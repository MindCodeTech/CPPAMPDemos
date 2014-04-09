//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: LargeVector.h
// 
// Declares the managed API of large vector operations.
//----------------------------------------------------------------------------

#pragma once

using namespace System;

namespace SampleLibrary {
	/// <summary>
	/// Provides functions for treating arrays as vectors and performing math on them.
	/// </summar>
	public ref class LargeVector {
	public:
		/// Performs a vector addition and returns the result.
		static array<float>^ Add(array<float>^ vecA, array<float>^ vecB);

		/// Performs a vector addition and returns the result.
		static array<int>^ Add(array<int>^ vecA, array<int>^ vecB);

	};
} // namespace SampleLibrary
