//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: sample_amp_library.h
// 
// Declares the public API for our sample library.
//----------------------------------------------------------------------------

#pragma once

// SAMPLEAMPLIBRARY_EXPORTS is defined in the project settings and is used in
// these dual-purpose header files to help distinguish between when this header
// is included in the library's implementation or being consumed by another
// project.
#ifdef SAMPLEAMPLIBRARY_EXPORTS
#define SAMPLEAMPLIBRARY_API __declspec(dllexport)
#else
#define SAMPLEAMPLIBRARY_API
#endif

namespace sample_amp_library {
	class my_math {
	public:
		/// Performs a vector addition and saves the result to the result array.
		SAMPLEAMPLIBRARY_API static void large_vector_add(
			const float* vec_a, const float* vec_b
			, float* result, unsigned int cnt);

		/// Performs a vector addition and saves the result to the result array.
		SAMPLEAMPLIBRARY_API static void large_vector_add(
			const int* vec_a, const int* vec_b, int* result
			, unsigned int cnt);

	};
}

