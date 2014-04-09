//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: large_vector.h
// 
// Declares the native API for fast implementation of large vector operations.
//----------------------------------------------------------------------------

#pragma once

// Note the namespace is in C++ style (lowercase with _ between words). This
// is to explicitly force the notion that the structures in this namespace
// are native, as opposed to running on the CLR.
namespace sample_library {
	class large_vector {
	public:
		/// Performs a vector addition and saves the result to the result array.
		static void add(
			const float* vec_a, const float* vec_b
			, float* result, unsigned int cnt);

		/// Performs a vector addition and saves the result to the result array.
		static void add(
			const int* vec_a, const int* vec_b, int* result
			, unsigned int cnt);

	};
} // namespace sample_library
