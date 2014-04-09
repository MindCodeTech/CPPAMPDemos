//--------------------------------------------------------------------------------------
// File: Utils.h
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this
// file except in compliance with the License.  You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR
// CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
//
// See the Apache Version 2.0 License for specific language governing permissions
// and limitations under the License.
//
//--------------------------------------------------------------------------------------
//
#pragma once
#include <amptest.h>
#include <iostream>

template <int _Rank>
runall_result expect_exception(const concurrency::accelerator_view& av, const concurrency::extent<_Rank>& ext, const std::string& expectedMessage)
{
	using namespace concurrency;
	using namespace concurrency::Test;

	try
	{
		int x;
		parallel_for_each(av, ext, [=](index<_Rank>) restrict(amp) { int y = x; (void)y; });
	}
	catch(invalid_compute_domain e)
	{
		if(e.what() == expectedMessage)
			return runall_pass;
		else
			throw; // Propagate the unexpected exception
	}

	return runall_fail;
}
