//--------------------------------------------------------------------------------------
// File: example.cpp
//
// Show how to invoke CURAND and CUBLAS from C++ AMP
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

#include "..\include\amp_cuda.h"
#include <amp.h>
#include <curand.h>
#include <iostream>
#include <fstream>
#include <random>
#include <cublas_v2.h>
#include "Example.h"

using namespace concurrency;

// Use curand to generate uniform distribution to an concurrency::array
void use_curand(accelerator_view av)
{
    std::cout << "Use CURAND" << std::endl;
    const int N = 64 * 1024;
    const int seed = 2012;

    array<float> gpu_generated_rand_numbers(N, av);

    {
        amp_cuda::scoped_device device(av, true);
        amp_cuda::scoped_buffer buffer(gpu_generated_rand_numbers, amp_cuda::scoped_buffer_write_discard_kind);

        curandGenerator_t gpu_rand_generator;
        checkCurandErrors(curandCreateGenerator(&gpu_rand_generator, CURAND_RNG_PSEUDO_MTGP32));
        checkCurandErrors(curandSetPseudoRandomGeneratorSeed(gpu_rand_generator, seed));
        checkCurandErrors(curandGenerateUniform(gpu_rand_generator, buffer.cuda_ptr<float>(), N));
    }

    std::vector<float> results = gpu_generated_rand_numbers; // copy to host container results

    std::ofstream output("rand.txt");
    output.unsetf(std::ios::floatfield);
    output.precision(10);
    for(size_t i = 0; i < results.size(); i++)
    {
        output << results[i] << std::endl;
    }
    output.close();
    std::cout << "Success!" << std::endl;
}

// host implementation of simple_sgemm
void simple_sgemm(int n, float alpha, const std::vector<float> & A, const std::vector<float> & B,
                  float beta, std::vector<float> & C)
{
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            float prod = 0;
            for (int k = 0; k < n; ++k) {
                prod += A[k * n + i] * B[j * n + k];
            }
            C[j * n + i] = alpha * prod + beta * C[j * n + i];
        }
    }
}

// Use cublasSgemm, where use three matrices that are from concurrency::array
void use_cublas(accelerator_view av)
{
    std::cout << "Use CUBLAS" << std::endl;
    const int N = 256;
    const int n2 = N * N;
    std::mt19937 mersenne_twister_engine;
    std::uniform_real_distribution<float> uni;
    
    float alpha = 1.0f;
    float beta = 0.0f;

    std::vector<float> h_A(n2);
    std::vector<float> h_B(n2);
    std::vector<float> h_C(n2);
    std::vector<float> h_C_ref(n2);
    
    // Fill the matrices with test data
    for (int i = 0; i < n2; i++) {
        h_A[i] = uni(mersenne_twister_engine);
        h_B[i] = uni(mersenne_twister_engine);
        h_C[i] = uni(mersenne_twister_engine);
        h_C_ref[i] = h_C[i];
    }

    // cpu
    simple_sgemm(N, alpha, h_A, h_B, beta, h_C_ref);

    const array<float, 2> d_A(N, N, h_A.begin(), h_A.end(), av);
    const array<float, 2> d_B(N, N, h_B.begin(), h_B.end(), av);
    array<float, 2> d_C(N, N, h_C.begin(), h_C.end(), av);

    {
        amp_cuda::scoped_device device(av, true);
        amp_cuda::scoped_buffer buf_A(d_A);
        amp_cuda::scoped_buffer buf_B(d_B);
        amp_cuda::scoped_buffer buf_C(d_C /*, amp_cuda::scoped_buffer_read_write_kind */);

        cublasHandle_t handle;
        checkCublasErrors(cublasCreate(&handle));

        checkCublasErrors(cublasSgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, N, N, N, &alpha, 
                                      buf_A.cuda_ptr<float>(), N, buf_B.cuda_ptr<float>(), N, 
                                      &beta, buf_C.cuda_ptr<float>(), N));

        checkCublasErrors(cublasDestroy(handle));
    }

    h_C = d_C; // copy data back to host

    // Check result against reference
    size_t cnt = 0;
    for (int i = 0; i < n2; ++i) {
        float diff = h_C_ref[i] - h_C[i];
        if (fabs(diff) > 1e-3) 
        {
            std::cout << "i = " << i << ", Fail: Got: " << h_C[i] << ", Expected: " << h_C_ref[i] << std::endl;
            cnt++;
        }
        if (cnt > 16) 
        {
            exit(-1);
        }
    }
    if (cnt == 0) 
    {
        std::cout << "Success!" << std::endl;
    }
}

int main()
{
    auto accls = accelerator::get_all();
    auto iter = std::find_if(accls.begin(), accls.end(), [](const accelerator & acc) {	
        return amp_cuda::is_accelerator_cuda_capable(acc);
    });
    if (iter != accls.end())
    {
        std::wcout << "CUDA Capable Device: " << iter->description << std::endl;
        use_curand(iter->default_view);
        use_cublas(iter->default_view);
    }
    else 
    {
        std::wcout << "No CUDA Capable Device or Driver!"<< std::endl;
    }
    return 0;
}