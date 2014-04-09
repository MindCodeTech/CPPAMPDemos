//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: sample_amp_library.cpp
// 
// Defines the APIs for our sample library.
// 
//----------------------------------------------------------------------------


#include "sample_amp_library.h"
#include <amp.h>

using namespace concurrency;

namespace sample_amp_library {
	namespace details {

		/// Performs a vector addition and saves the result to the result array.
		template <typename T>
		void large_vector_add(
			const T* vec_a, const T* vec_b, T* result, unsigned int cnt
			) {
			// Get the accelerator we want to execute on
			accelerator_view av = accelerator().default_view;

			extent<1> data_sz(cnt);

			// Create a view over the data on the CPU
			array_view<const T,1> vec_av(data_sz, &vec_a[0]);
			array_view<const T,1> vec_bv(data_sz, &vec_b[0]);
			array_view<T,1> resultv(data_sz, &result[0]);

			// Run code on the GPU
			// Specify av to explicitly indicate which accelerator to run on.
			parallel_for_each(av, resultv.extent
				, [=](index<1> idx) restrict(amp) {
					const T operandA = vec_av[idx];
					const T operandB = vec_bv[idx];

					resultv[idx] = operandA + operandB;
			});

			// Copy data from GPU to CPU, only needed for output
			resultv.synchronize();
		}

	}

	void my_math::large_vector_add(
		const float* vec_a, const float* vec_b, float* result, unsigned int cnt) {
		details::large_vector_add<float>(vec_a, vec_b, result, cnt);
	}

	void my_math::large_vector_add(
		const int* vec_a, const int* vec_b, int* result, unsigned int cnt) {
		details::large_vector_add<int>(vec_a, vec_b, result, cnt);
	}

}	// namespace SampleAMPLibrary
