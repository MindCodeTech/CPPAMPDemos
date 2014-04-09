//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: CLRConsoleApp.cpp
// 
// Defines the entry point of this console application. This shows how to
// make calls into our sample native dll containing C++ AMP code from a
// C++/CLI application.
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <algorithm>

#include "sample_amp_library.h"

using namespace System;
using namespace sample_amp_library;

/// <summary>
/// Performs addition on a large vector.
/// Templated managed wrapper function which calls into the native implementation.
/// This functions handles marshalling memory between the managed and native
/// memory spaces.
/// </summary>
template<typename T>
array<T>^ large_vector_add(array<T>^ vecA, array<T>^ vecB) {
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
		my_math::large_vector_add(vecA_ptr, vecB_ptr, result_ptr, num_elements);

		// The arrays will get unpinned when we exit from the scope.
	}

	return result;
}

bool test_large_vector_add(int num_elements) {
	Random rng;

	// Create the inputs
	Console::WriteLine(L"Generating inputs...");
	array<int>^ vecA = gcnew array<int>(num_elements);
	array<int>^ vecB = gcnew array<int>(num_elements);
	for (int i = 0; i < num_elements; i++) {
		vecA[i] = rng.Next();
		vecB[i] = rng.Next();
	}
	
	// Create array to store the results from our library and set to a sentinal value
	Console::WriteLine(L"Computing using our sample library using C++ AMP...");
	array<int>^ actual_result = large_vector_add(vecA, vecB);

	// Compute the expected result on the CPU component by component
	Console::WriteLine(L"Computing on the CPU...");
	array<int>^ expected_result = gcnew array<int>(num_elements);
	for (int i = 0; i < num_elements; i++) {
		expected_result[i] = vecA[i] + vecB[i];
	}

	// And check our results.
	Console::WriteLine(L"Verifying result...");
	static const int max_results_to_log = 10;
	int wrong_result_cnt = 0;
	for (int i = 0; i < num_elements; i++) {
		if(actual_result[i] != expected_result[i]) {
			wrong_result_cnt++;
			if(wrong_result_cnt <= max_results_to_log) {
				Console::WriteLine(L"wrong result for vector component {0}. expected = {1}, actual = {2}", i, expected_result[i], actual_result[i]);
			}
		}
	}

	if(wrong_result_cnt == 0) {
		Console::WriteLine(L"All results were correct.");
	} else {
		Console::WriteLine(L"A total of {0} elements produced the wrong result.", wrong_result_cnt);
	}

	return wrong_result_cnt == 0;
}

int main(array<String^>^ args) {
	bool passed = true;
	try {

		passed &= test_large_vector_add(65536 / sizeof(int)); // Use 64KB per vector

	} catch(Exception^ ex) {
		Console::Error->WriteLine("Unhandled {0} exception caught: {1}", ex->GetType()->Name, ex->Message);
		passed = false;
	}

	return passed ? 0 : 1;
}
