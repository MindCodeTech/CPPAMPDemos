//RUN: false
//XFAIL: *
//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: main.cpp
// 
// This file contains driver code to perform matrix multiplication
//----------------------------------------------------------------------------

#include <iostream>
#include <vector>
#include <random>
#include <amp.h>
#include "dist_mxm.h"
#include <string>

using namespace concurrency;

void rand_init	(float* f_arr, unsigned int sz) {
    std::mt19937 engine(544);
    std::normal_distribution<float> dist;

    for(size_t i=0; i < sz; i++) {
        f_arr[i]=dist(engine);
    }
}

void simple_init(float* f_arr, unsigned int sz) {
    for(size_t i=0; i < sz; i++) {
        f_arr[i]= float(i);
    }
}

void zero_init(float* f_arr, unsigned int sz) {
    for(size_t i=0; i < sz; i++) {
        f_arr[i] = 0;
    }
}


void main() {
    const int MATRIX_SZ = 1024;
    const int STREAM_WIDTH = 512;

    const unsigned int M = MATRIX_SZ;
    const unsigned int N = MATRIX_SZ;
    const unsigned int W = MATRIX_SZ;

    float *A = new float[M*N];
    float *B = new float[N*W];
    float *C = new float[M*W];

    rand_init(A, M*N);	// initialize with random numbers
    rand_init(B, N*W);  // initialize with random numbers
    zero_init(C, M*W);   // initialize with zero

    std::vector<accelerator> accelerators = accelerator::get_all();
    std::vector<accelerator_view> accelerator_views;
    std::for_each(accelerators.begin(), accelerators.end(), [&] (accelerator& accl) {
        if(!accl.is_emulated) {		// if not emulated
            std::wstring wdescr = std::wstring(accl.description);
            std::wcout << accl.description << std::endl;
            accelerator_views.push_back(accl.default_view);	// get the default accelerator view
        }
    });

    mxm_amp_distribute(accelerator_views, STREAM_WIDTH, A, B, C, M, N, W);
}
