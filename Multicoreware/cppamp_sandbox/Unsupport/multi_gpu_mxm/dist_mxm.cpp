//RUN: false
//XFAIL: *
//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: dist_mxm.cpp
// 
// Implement matrix multiplication using multiple GPU 
// This sample is implemented using C++ AMP
//----------------------------------------------------------------------------

#include <algorithm>
#include "dist_mxm.h"

using namespace concurrency;
static const unsigned int TILE_SIZE=16;

// Distributed C++ AMP Matrix Multiplication - chunk & stream
//
// Demonstrates partitioning matrix multiplication to run on multiple accelerators in a chunk and stream approach.
//		Loop until all chunks have been processed
//			In parallel, dispatch a stream_width height chunk of A & C to the available accelerators
//			Loop and dispatch vertical streams of B to resolve chunk of C with tiled matrix multiply
//
// A is an W x N matrix
// B is an N x M matrix
// C is an W x M matrix
//
// Constraints in this sample code: 
//		M, N, W must be a multiple of stream_width
//		stream_width must be a multiple of TILE_SIZE
//

// populate B matrix column stream
void populate_stream(float* column_stream_ptr, const float* const arr_in, unsigned int start_column, unsigned int stream_width, unsigned int arr_length, unsigned int arr_width) {
    for(unsigned int row = 0; row < arr_length; row++) {
        memcpy(column_stream_ptr + (row * stream_width), arr_in + (row * arr_width) + start_column, stream_width * sizeof(float));
    }
}

// matrix multiply, resolve C for this stream
void mxm_calc_stream(extent<2> extent_compute, unsigned int N, unsigned int start_column, array<float,2>const &matrix_a, array_view<float,2> const &matrix_b, array<float,2> &matrix_c) {
    parallel_for_each(extent_compute.tile<TILE_SIZE,TILE_SIZE>(), [=, &matrix_a, &matrix_c] (tiled_index<TILE_SIZE,TILE_SIZE> t_idx) restrict(amp) {
        float accum_c = 0;	// accumulate sum in local register to avoid extra round trips to global memory

        for (unsigned int i = 0; i < N; i += TILE_SIZE)
        {
            tile_static float local_B[TILE_SIZE][TILE_SIZE];	// tile memory for partial sum of matrix B
            tile_static float local_A[TILE_SIZE][TILE_SIZE];    // tile memory for partial sum of matrix A

            local_A[t_idx.local[0]][t_idx.local[1]] = matrix_a(t_idx.global[0], i + t_idx.local[1]);
            local_B[t_idx.local[0]][t_idx.local[1]] = matrix_b(i + t_idx.local[0], t_idx.global[1]);

            t_idx.barrier.wait();	// wait for all threads in the tile to pull their corresponding values into the tile_static memory
        
            for (unsigned int k = 0; k < TILE_SIZE; k++)
            {
                accum_c += local_A[t_idx.local[0]][k] * local_B[k][t_idx.local[1]];	// accumulate partial sum for current tile
            }
       
            t_idx.barrier.wait();	// wait for all threads in the tile
        }
        matrix_c(t_idx.global[0], t_idx.global[1] + start_column) = accum_c;	// done with this tile, update global memory
    });
}

// dispatch chunk of A & C
void dispatch_chunk(accelerator_view acc_vw, const float* A, const float* B, float* C, unsigned int M, unsigned int N, unsigned int W, unsigned int stream_width, unsigned int stream_offset, array<float,2> &staging_b) {
    extent<2> extent_A(stream_width, N);
    extent<2> extent_C(stream_width, M);

    extent<2> extent_compute(stream_width, stream_width);

    // chunk storage on GPU for A & C
    array<float, 2> matrix_a(extent_A, A + (stream_offset * N), acc_vw);
    array<float, 2> matrix_c(extent_C, C + (stream_offset * M), acc_vw);

    array_view<float,2> matrix_b(staging_b);	

    // stream B to GPU
    for(unsigned int start_column = 0; start_column < M; start_column += stream_width) {
            populate_stream(staging_b.data(), B, start_column, stream_width, N, M); 
            mxm_calc_stream(extent_compute, N, start_column, matrix_a, matrix_b, matrix_c);
    }
    copy(matrix_c, stdext::checked_array_iterator<float*>(C + (stream_offset * M), extent_C.size()));
}

void mxm_amp_distribute(std::vector<accelerator_view> acc_views, const unsigned int stream_width, const float*  A, const float* B, float*  C, const unsigned int M, const unsigned int N, const unsigned int W) {
    // Performs matrix multiplication C = A * B

    // throw runtime error if M, N and stream_width are not all multiples of TILE_SIZE
    if( ((M % stream_width) != 0) || ((N % stream_width) != 0) || ((W % stream_width) != 0) ) {
        throw std::runtime_error("M, N and W must all be multiples of stream_width");
    }

    if(acc_views.size() == 0) {
        throw std::runtime_error("acc_views is empty, nowhere to run");
    }

    // Create a vector of staging arrays for streams of matrix B, one for each accelerator
    std::vector<array<float, 2>> b_vect;
    b_vect.reserve(acc_views.size()); 
    accelerator cpu_accelerator = accelerator(accelerator::cpu_accelerator);

    std::for_each(acc_views.begin(), acc_views.end(), [&] (accelerator_view acc_vw) {
        b_vect.emplace_back(extent<2>(N, stream_width), cpu_accelerator.default_view, acc_vw); 
    });

    unsigned int w_start = 0;	// indicates where to start processing
    while(w_start < W) {
        parallel_for((size_t) 0, acc_views.size(), [&] (size_t accelerator_ix) {
            unsigned int stream_offset = w_start + (stream_width * accelerator_ix);
            if (stream_offset < W) {
                dispatch_chunk(acc_views[accelerator_ix], A, B, C, M, N, W, stream_width, stream_offset, b_vect[accelerator_ix]);
            }
        });
        w_start += (acc_views.size() * stream_width);
    }
}
