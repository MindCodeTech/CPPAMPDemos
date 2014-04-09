//--------------------------------------------------------------------------------------
// File: Example.h
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

#pragma once
#include <curand.h>
#include <cublas_v2.h>
#include <stdio.h>
#include <stdlib.h>
#include "..\include\amp_cuda.h"

// This will output the proper CUDA error strings in the event that a CUDA host call returns an error
#define checkCurandErrors(err)           __checkErrors (err, CURAND_STATUS_SUCCESS, "CURAND", __FILE__, __LINE__)
#define checkCublasErrors(err)           __checkErrors (err, CUBLAS_STATUS_SUCCESS, "CUBLAS", __FILE__, __LINE__)

inline void __checkErrors(int err, int success_code, const char * module_name, const char *file, const int line)
{
    if( success_code != err) 
    {
        std::stringstream msg;
        msg << file << "(" << line << "): " << module_name << " error " << err << std::endl; 
        throw amp_cuda::interop_exception(msg.str().c_str());
    }
}
