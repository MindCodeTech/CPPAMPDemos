//////////////////////////////////////////////////////////////////////////////
////
//// Copyright (c) Microsoft Corporation. All rights reserved
////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: MatrixOperationsCppAmp.cs
//----------------------------------------------------------------------------

#include <amp.h>
using namespace concurrency;

extern "C" __declspec ( dllexport ) char* _stdcall MultiplyMatrix(float* A, float* B, float*  C, int M, int N, int W)
{
	static const int tile_size = 16;

	concurrency::extent<2> eA(M, N), eB(N, W), eC(M, W);
	concurrency::extent<2> g(((M+tile_size-1)/tile_size)*tile_size, ((W+tile_size-1)/tile_size)*tile_size);

	array<float, 2> mA (eA, A), mB (eB, B), mC (eC);

	try
	{
		parallel_for_each(g.tile< tile_size, tile_size >(), 
			[=, &mA, &mB, &mC] (tiled_index< tile_size, tile_size> t_idx) restrict(amp)  {
				float temp = 0;
				index<2> locIdx = t_idx.local;
				index<2> globIdx = t_idx.global;  

				for (int i = 0; i < N; i += tile_size) 
				{
					tile_static float locB[tile_size][tile_size], locA[tile_size][tile_size];

					locA[locIdx[0]][locIdx[1]] = (globIdx[0] < M && i + locIdx[1] < N) ? mA(globIdx[0], i + locIdx[1]) : 0.0f;
					locB[locIdx[0]][locIdx[1]] = (i + locIdx[0] < N && globIdx[1] < W) ? mB(i + locIdx[0], globIdx[1]) : 0.0f;

					t_idx.barrier.wait();

					for (int k = 0; k < tile_size; k++)
					{
						temp += locA[locIdx[0]][k] * locB[k][locIdx[1]];            
					}

					t_idx.barrier.wait();
				}

				if (t_idx.tile_dim1 < M && t_idx.tile_dim0 < W) 
				{
					mC[t_idx] = temp;
				}
		} );
		copy(mC, stdext::checked_array_iterator<float*>(C, eC.size()));
	}
	catch(std::exception& ex)
	{
		std::string ex_msg(ex.what());

		// Using CoTaskMemAlloc() to allocate memory so that it can
		// be freed when CLR interop layer calls CoTaskMemFree
		char* msg = (char*)CoTaskMemAlloc( ex_msg.length() + 1);
		ex_msg._Copy_s(msg, ex_msg.length(), ex_msg.length());

		// Null-terminate the msg
		msg[ex_msg.length()] = '\0';

		return msg;
	}
	
	return NULL;
}