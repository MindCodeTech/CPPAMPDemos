//--------------------------------------------------------------------------------------
// File: test.cpp
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
/// <tags>P1</tags>
/// <summary>Create an Array on GPU, write through the underlying data on the GPU, call refresh</summary>

#include <amptest.h>
#include <amptest_main.h>
#include <vector>
#include <algorithm>

using namespace Concurrency;
using namespace Concurrency::Test;

runall_result test_main()
{
    accelerator acc = require_device();
	
    if(acc.supports_cpu_shared_memory)
    {
        acc.set_default_cpu_access_type(ACCESS_TYPE);
    }
    
    array<int, 1> a(extent<1>(10));
    array_view<int, 1> av(a);
    
    Log() << "Writing on the GPU" << std::endl;
    parallel_for_each(extent<1>(1), [=, &a](index<1>) __GPU {
        av.data()[0] = 17;
    });
    
    av.refresh();
    
    Log() << "Result is: " << av[0] << "Expected: 17" << std::endl;
    return av[0] == 17 ? runall_pass : runall_fail;
}