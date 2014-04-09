//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: LargeVector.cpp
// 
// Defines the managed implementations of large vector operations.
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "LargeVector.h"
#include "large_vector.h"

using namespace System;
using namespace sample_library;

namespace SampleLibrary {
	namespace details {

		template <typename T>
		array<T>^ LargeVector_Add(array<T>^ vecA, array<T>^ vecB) {
			if(vecA == nullptr || vecB == nullptr)
				throw gcnew ArgumentNullException();
			if(vecA->Length != vecB->Length)
				throw gcnew ArgumentException("The length of both vectors should be equal.");

			// Create a managed array for the results
			int num_elements = vecA->Length;
			array<T>^ result = gcnew array<T>(num_elements);

			{
				// Need pinned pointers to pass to the native layer so the arrays don't
				// get moved by the GC
				// See: http://msdn.microsoft.com/en-us/library/1dz8byfh(v=VS.110).aspx
				pin_ptr<const T> vecA_ptr = &vecA[0];
				pin_ptr<const T> vecB_ptr = &vecB[0];
				pin_ptr<T> result_ptr = &result[0];

				// Use the native c++ library which uses C++ AMP
				large_vector::add(vecA_ptr, vecB_ptr, result_ptr, num_elements);

				// The arrays will get unpinned when we exit from the scope.
			}

			return result;
		}

	}	// namespace details

	array<float>^ LargeVector::Add(array<float>^ vecA, array<float>^ vecB) {
		return details::LargeVector_Add<float>(vecA, vecB);
	}

	array<int>^ LargeVector::Add(array<int>^ vecA, array<int>^ vecB) {
		return details::LargeVector_Add<int>(vecA, vecB);
	}

} // namespace SampleLibrary