//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: LargeMatrixMultiplication.cpp
// 
// Implement C++ AMP based matrix multiplication for large square matrices. It implements four different chunking 
// strategies. Each stragey has different overhead to stream the data to the accelerator.
// 
//   1) Strategy 1: Chunking the matrices along the dimensions I, J and K. A, B, and C all have the same chunk size;
//                  Streaming the matrices in the loop order ijk.
//   2) Strategy 2: Chunking the matrices along the dimensions I and J. A and B have the same chunk size. 
//                  Streaming A, B and C in the loop order ij.
//   3) Strategy 3: A modification of the Strategy 2. It streams a large chunk of C to reduce the number of streamings
//                  for C with the tradeoff not fully utilizing the accelerator memory allocated for C during each iteration
//                  of the computation.
//   4) Strategy 4: A modification of the Strategy 2. It streams A, B, and C in the loop order of ji, not ij. This is 
//                  to utilize the memory layout of the matrices on the CPU assuming they are all row-major. 
//                  This will avoid the overhead to pack the chunk of A before it is streamed to the accelerator.
//----------------------------------------------------------------------------

#include <amp.h>
#include <assert.h>
#include <memory.h>
#include <iostream>
#include <iomanip>
#include "timer.h"

using namespace concurrency;

static const int TILE_SIZE             = 32;            // The shared memory tile size for matrix multiplication 
static const int DEFAULT_ACCL_MEM_SIZE = 64*1024*1024;  // Assume the default accelerator memory size

template<typename T, bool is_writeonly=false> 
struct array_view_type_trait
{ 
    inline static void addition_or_assignment(T& x, const T& y) restrict(amp) { x += y;}
};

template<typename T> 
struct array_view_type_trait<T, true>
{ 
    inline static void addition_or_assignment(T& x, const T& y) restrict(amp) { x = y;}
};

// Align the size to the multiple of the TILE_SIZE 
inline int aligned_tile_size(int size)
{
    return (size <= TILE_SIZE) ? size : static_cast<int>((size/TILE_SIZE) * TILE_SIZE);
}

// pack_array_chunk: 
//   Extracts a chunk from a 2D array stored in vector src, with the chunk's origin and its extent specified. 
//
template<typename T>
void pack_array_chunk(extent<2>& src_ext, const std::vector<T>& src, index<2>& chunk_origin, extent<2>& chunk_ext, std::vector<T>& chunk)
{
	auto M = src_ext[0];
	auto N = src_ext[1];
    auto chunk_M = chunk_ext[0];
    auto chunk_N = chunk_ext[1];
    auto chunk_offset = chunk_origin[0]*N + chunk_origin[1];

	assert(src.size() >= M*N);
	assert(chunk_origin[0] >= 0 && chunk_origin[1] >= 0);
	assert((chunk_origin[0] + chunk_M - 1) * N + (chunk_origin[1] + chunk_N) <= src.size());

	for (int i = 0; i < chunk_M; ++i)
	{
        memcpy_s(&chunk[i*chunk_N], chunk_N*sizeof(T), &src[i*N+chunk_offset], chunk_N*sizeof(T));
	}
}

// unpack_array_chunk:
//   Updates a 2D array stored in vector dst with a 2D chunk data, where the chunk's origin and its extent are specified. 
//
template<typename T>
void unpack_array_chunk(index<2>& chunk_origin, extent<2>& chunk_ext, std::vector<T>& chunk, extent<2>& dst_extent, std::vector<T>& dst)
{
	int M = dst_extent[0];
	int N = dst_extent[1];
    auto chunk_M = chunk_ext[0];
    auto chunk_N = chunk_ext[1];
    auto chunk_offset = chunk_origin[0]*N + chunk_origin[1];

	assert(dst.size() >= M*N);
	assert(chunk_origin[0] >= 0 && chunk_origin[1] >= 0);
	assert((chunk_origin[0] + chunk_M - 1) * N + (chunk_origin[1] + chunk_N) <= dst.size());

	for (int i = 0; i < chunk_M; ++i)
	{
        memcpy_s(&dst[i*N+chunk_offset], chunk_N*sizeof(T), &chunk[i*chunk_N], chunk_N*sizeof(T));
	}
}

//
// tiled_matrix_multiply:
//   Computes matrix multiplication:
//     C += A * B if is_c_write_only is true or
//     C  = A * B if is_c_write_only is false
//
template<typename T, bool is_c_write_only>
void tiled_matrix_multiply(array_view<const T, 2> av_a, array_view<const T, 2> av_b,  array_view<T, 2> av_c) 
{
	parallel_for_each(av_c.extent.tile<TILE_SIZE,TILE_SIZE>(), [=] (tiled_index<TILE_SIZE,TILE_SIZE> tidx) restrict(amp)
	{
		T temp_c = 0;

	    index<2> localIdx = tidx.local;
	    index<2> globalIdx = tidx.global;

	    for (int i = 0; i < av_a.extent[1]; i += TILE_SIZE)
	    {
		    tile_static T localB[TILE_SIZE][TILE_SIZE];
		    tile_static T localA[TILE_SIZE][TILE_SIZE];

		    localA[localIdx[0]][localIdx[1]] = av_a(globalIdx[0], i + localIdx[1]);
		    localB[localIdx[0]][localIdx[1]] = av_b(i + localIdx[0], globalIdx[1]);
        
		    tidx.barrier.wait_with_tile_static_memory_fence();
        
		    for (int k = 0; k < TILE_SIZE; k++)
		    {
			    temp_c += localA[localIdx[0]][k] * localB[k][localIdx[1]];
		    }
       
		    tidx.barrier.wait_with_tile_static_memory_fence();
	    }

        array_view_type_trait<T, is_c_write_only>::addition_or_assignment(av_c[tidx], temp_c);
    });
}

// matrix_multiply_strategy1:
//   Chunking the matrices along the dimensions I, J and K. The chunks of A, B and C are all square tiles with 
//   the same chunk size. The chunks are streamed to the accelerator in the loop order of ijk.
//
//   The tile_size of the chunks is computed from the following:
//        accel_mem_size = 3*tile_size*tile_size;
//
template<typename T>
void matrix_multiply_strategy1(int N, int accl_mem_size, const std::vector<T>& A, const std::vector<T>& B, std::vector<T>& C)
{
    auto tile_size = aligned_tile_size(static_cast<int>(sqrt(accl_mem_size / (3 * sizeof(T)))) );

	std::vector<T> chunk_a(tile_size*tile_size);
	std::vector<T> chunk_b(tile_size*tile_size);
	std::vector<T> chunk_c(tile_size*tile_size);

    for (int i = 0; i < N; i += tile_size)
    {
        auto tmp_m = min(tile_size, (N - i));

	    for (int j = 0; j < N; j += tile_size)
        {
            // Stream the chunk of C
            auto tmp_n = min(tile_size, (N - j));
			array_view<T, 2> av_c(tmp_m, tmp_n, chunk_c);

			// a little bit faster compared to reset the chunk C at the CPU, then copy it to GPU			
			parallel_for_each(av_c.extent, [=](index<2> idx) restrict(amp)
			{
				av_c[idx] = 0;
			});

		    for (int k = 0; k < N; k += tile_size)
            {
                // Pack the chunks of A and B, and stream them to the accelerator
                auto tmp_k = min(tile_size, (N - k));
				pack_array_chunk(extent<2>(N, N), A, index<2>(i, k), extent<2>(tmp_m, tmp_k), chunk_a);
				pack_array_chunk(extent<2>(N, N), B, index<2>(k, j), extent<2>(tmp_k, tmp_n), chunk_b);
				array_view<const T, 2> av_a(tmp_m, tmp_k, chunk_a);
				array_view<const T, 2> av_b(tmp_k, tmp_n, chunk_b);

                tiled_matrix_multiply<T, false>(av_a, av_b, av_c);
			}

			// Copyback the chunk of C and unpack it to C
			av_c.synchronize();
			unpack_array_chunk(index<2>(i, j), extent<2>(tmp_m, tmp_n), chunk_c, extent<2>(N, N), C); 
        }
    }
}

//
// matrix_multiply_strategy2: 
//   Chunking the matrices along the dimensions I and J. The chunks of A and B are panel tiles, and the chunk of C
//   is a square tile. The chunks of A and B are streamed to the accelerator in the loop order of ij. 
//
//   The tile_size of the chunks is computed from the following:
//        accel_mem_size = A:tile_size*N + B:N*tile_size + C:tile_size*tile_size;
// 
template<typename T>
void matrix_multiply_strategy2(int N, int accl_mem_size, const std::vector<T>& A, const std::vector<T>& B, std::vector<T>& C)
{
	auto tile_size = aligned_tile_size(static_cast<int>(sqrt(accl_mem_size / sizeof(T) + N*N ) - N));

	std::vector<T> chunk_b(N*tile_size);
	std::vector<T> chunk_c(tile_size*tile_size);

	for (int i = 0; i < N; i += tile_size)
    {
        // Stream the chunk of A
        auto tmp_m = min(tile_size, (N - i));
        array_view<const T, 2> av_a(tmp_m, N, &A[i*N]);

        for (int j = 0; j < N; j += tile_size)
		{
            // Pack the chunk of B and stream it with the chunk of C to the accelerator
            auto tmp_n = min(tile_size, (N - j));
            pack_array_chunk(extent<2>(N, N), B, index<2>(0, j), extent<2>(N, tmp_n), chunk_b);
            array_view<const T, 2> av_b(N, tmp_n, chunk_b);
            array_view<T, 2> av_c(tmp_m, tmp_n, chunk_c);
			av_c.discard_data();

            tiled_matrix_multiply<T, true>(av_a, av_b, av_c);

			// Copyback the chunk of C and unpack it to C
			av_c.synchronize();
			unpack_array_chunk(index<2>(i, j), extent<2>(tmp_m, tmp_n), chunk_c, extent<2>(N, N), C); 
        }
    }
}

//
// matrix_multiply_strategy3: 
//   Chunking the matrices of A and C along the dimensions I, and chunking B along the dimension J. 
//   The chunks of A and B are streamed to the accelerator in the loop order of ij. 
//
//   The tile_size of the chunks is computed from the following:
//        accel_mem_size = A:tile_size*N + B:N*tile_size + C:tile_size*N;
// 
template<typename T>
void matrix_multiply_strategy3(int N, int accl_mem_size, const std::vector<T>& A, const std::vector<T>& B, std::vector<T>& C)
{
	auto tile_size = aligned_tile_size(static_cast<int>(accl_mem_size / (3*N*sizeof(T))));
	std::vector<T> chunk_b(N*tile_size);

	for (int i = 0; i < N; i += tile_size)
    {
        // Stream the chunks of A & C
        auto tmp_m = min(tile_size, (N - i));
        array_view<const T, 2> av_a(tmp_m, N, &A[i*N]);
        array_view<T, 2> av_c(tmp_m, N, &C[i*N]);
		av_c.discard_data();

        for (int j = 0; j < N; j += tile_size)
		{
            // Pack the chunk of B and stream it to the accelerator
            auto tmp_n = min(tile_size, (N - j));
            pack_array_chunk(extent<2>(N, N), B, index<2>(0, j), extent<2>(N, tmp_n), chunk_b);
            array_view<const T, 2> av_b(N, tmp_n, chunk_b);
			array_view<T, 2> av_c1 = av_c.section(index<2>(0, j), extent<2>(tmp_m, tmp_n));
			av_c1.discard_data();

            tiled_matrix_multiply<T, true>(av_a, av_b, av_c1);
        }
		// Copyback the chunk of C
		av_c.synchronize();
    }
}

//
// matrix_multiply_strategy4: 
//   Modification of the strategy 2 except the chunks of A and B are streamed to the accelerator 
//   in the loop order of ji. 
//
template<typename T>
void matrix_multiply_strategy4(int N, int accl_mem_size, const std::vector<T>& A, const std::vector<T>& B, std::vector<T>& C)
{
	auto tile_size = aligned_tile_size(static_cast<int>(sqrt(accl_mem_size / sizeof(T) + N*N ) - N));

	std::vector<T> chunk_b(N*tile_size);
	std::vector<T> chunk_c(tile_size*tile_size);

	for (int j = 0; j < N; j += tile_size)
    {
        // Pack the chunk of B and stream it to the accelerator
        auto tmp_n = min(tile_size, (N - j));
        pack_array_chunk(extent<2>(N, N), B, index<2>(0, j), extent<2>(N, tmp_n), chunk_b);
        array_view<const T, 2> av_b(N, tmp_n, chunk_b);

		for (int i = 0; i < N; i += tile_size)
        {
            // Stream the chunks of A and C
            auto tmp_m = min(tile_size, (N - i));
            array_view<const T, 2>      av_a(tmp_m, N, &A[i*N]);
            array_view<T, 2> av_c(tmp_m, tmp_n, chunk_c);
			av_c.discard_data();

            tiled_matrix_multiply<T, true>(av_a, av_b, av_c);

			// Copyback the chunk of C and unpack it to C
			av_c.synchronize();
			unpack_array_chunk(index<2>(i, j), extent<2>(tmp_m, tmp_n), chunk_c, extent<2>(N, N), C); 
        }
    }
}

template<typename T>
bool verify(int N, std::vector<T> &results)
{
	for (int i = 0; i < N; ++i)
    {
		for (int j = 0; j < N; ++j)
		{
            auto expected = static_cast<T>(j);
            auto actural  = results[i*N+j];

			if (fabs(actural - expected) / fabs(actural + expected) > 0.0001) 
			{
                std::cout << "mistach: result[" << i <<"][" << j << "]:  expteced=" << expected << "  actural=" << actural << std::endl;
				return false;
			}
		}
    }

	return true;
}

template<typename T, typename matrix_multiply_func>
void run_test(int N, std::vector<int> &accl_mem_sizes, matrix_multiply_func& matrix_multiply, const char* matrix_multiply_name)
{
	gpu_timer timer;

	if ((N % TILE_SIZE) != 0)
		throw std::exception("Matrix dimension needs to be aligned with the tile size"); 

    // Test the strategy with various available accelerator memroy size
	for (unsigned int i = 0; i < accl_mem_sizes.size(); ++i) 
	{
		auto accl_mem_size = accl_mem_sizes[i];
        auto total_size = 3*N*N*sizeof(T);  // total three matrix size

		if (accl_mem_size >= total_size)
			throw std::exception("Matrix too small, cannot test chuncking strategies");

	    // Minimum memory requirement: panel A & B (2*TILE_SIZE*N), C tile (TILE_SIZE*TILE_SIZE)
        auto miminum_mem_size = (2*TILE_SIZE*N + TILE_SIZE*TILE_SIZE) * sizeof(T);
        if (accl_mem_size < miminum_mem_size)
		    throw std::exception("Not enough accelerator memory");

        // Initialize the matrices
		std::vector<T> A(N*N);
		std::vector<T> B(N*N);
		std::vector<T> C(N*N);

        std::fill(A.begin(), A.end(), static_cast<T>(1));
        std::fill(B.begin(), B.end(), static_cast<T>(0));
        std::fill(C.begin(), C.end(), static_cast<T>(0));

		for (int i = 0; i < N; i++)
			for (int j = 0; j < N; j++)
				if (i == j) B[i*N+j] = static_cast<T>(i);

        auto data_size_MB = 3*N*N*sizeof(T)/(1024*1024);
        auto accl_mem_size_MB = accl_mem_size/(1024*1024);
        std::cout << matrix_multiply_name << ":  N=" << N << "  data_size=" << data_size_MB << "MB  available_device_memory=" << accl_mem_size_MB << "MB  ";

		// warmup
		matrix_multiply(1024, DEFAULT_ACCL_MEM_SIZE, A, B, C);

		// run and timing
		timer.start();
		matrix_multiply(N, accl_mem_size, A, B, C);
		timer.stop();

		std::cout << std::setprecision(8) << " elapsed_time=" << timer.read() << "(sec.)\n";

#ifdef __DEBUG
		if (!verify(N, C))
			throw std::exception(" failed\n");
		else 
			std::cout << " passed.\n";
#endif
	}
}

int main()
{
	accelerator default_device;
	std::wcout << L"Using device : " << default_device.get_description() << std::endl;
	if (default_device == accelerator(accelerator::direct3d_ref))
		std::cout << "WARNING!! Running workload on emulator will slow its performance" << std::endl;

	const int N = 10240;                 // each matrix 1024*1024*1024*sizeof(float) = 400MB
	std::vector<int> accl_mem_sizes(10); // available accelerator memory sizes

	for (unsigned int i = 0; i < accl_mem_sizes.size(); ++i) 
		accl_mem_sizes[i] = DEFAULT_ACCL_MEM_SIZE + i*20*1024*1024;
	
	try 
	{
		run_test<float>(N, accl_mem_sizes, matrix_multiply_strategy1<float>, "matrix_multiply_strategy1");
		run_test<float>(N, accl_mem_sizes, matrix_multiply_strategy2<float>, "matrix_multiply_strategy2");
		run_test<float>(N, accl_mem_sizes, matrix_multiply_strategy3<float>, "matrix_multiply_strategy3");
		run_test<float>(N, accl_mem_sizes, matrix_multiply_strategy4<float>, "matrix_multiply_strategy4");
	} 
	catch (const std::exception &e)
	{
		std::cout << "Error: " << e.what() << std::endl;
	}

	return 0;
}
