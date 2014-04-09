//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: large_vector.cpp
// 
// Defines the implementations of the native large vector operations.
// 
// This file can NOT be compiled using /clr as it uses C++ AMP.
//----------------------------------------------------------------------------

// Since this file will be compiled with different compiler options than the
// rest of the files in the project, we cannot use the same common stdafx.h.
//#include "stdafx.h"

#include "large_vector.h"
#include <amp.h>

using namespace concurrency;

namespace sample_library {
	namespace details {

		/// Performs a vector addition and saves the result to the result array.
		template <typename T>
		void large_vector_add(
			const T* vec_a, const T* vec_b, T* result, unsigned int cnt
			) {
				extent<1> data_sz(cnt);

				// Create a view over the data on the CPU
				array_view<const T,1> vec_av(data_sz, &vec_a[0]);
				array_view<const T,1> vec_bv(data_sz, &vec_b[0]);
				array_view<T,1> resultv(data_sz, &result[0]);

				// Run code on the GPU
				parallel_for_each(resultv.extent
					, [=](index<1> idx) restrict(amp) {
						resultv[idx] = vec_av[idx] + vec_bv[idx];
				});

				// Copy data from GPU to CPU, only needed for output
				resultv.synchronize();
		}

	}

	void large_vector::add(
		const float* vec_a, const float* vec_b, float* result, unsigned int cnt) {
			details::large_vector_add<float>(vec_a, vec_b, result, cnt);
	}

	void large_vector::add(
		const int* vec_a, const int* vec_b, int* result, unsigned int cnt) {
			details::large_vector_add<int>(vec_a, vec_b, result, cnt);
	}

} // namespace sample_library
