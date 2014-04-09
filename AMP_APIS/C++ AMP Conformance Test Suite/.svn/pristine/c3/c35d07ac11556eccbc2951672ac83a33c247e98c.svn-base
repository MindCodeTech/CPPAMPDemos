// Copyright (c) Microsoft
// All rights reserved
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.

/// <summary>Test D3D interop API - This test also verifies if any interop causes any leak</summary>

#include <amptest.h>
#include <amptest_main.h>
#include <amptest/amp.interop.h>
#include "d3d_helper.h"

using namespace Concurrency;
using namespace Concurrency::direct3d;
using namespace Concurrency::Test;
using std::vector;

void MatrixMultiplication_gpu(
    index<2> idx,
    array<float, 2> & mC,
    const array<float, 2> & mA,
    const array<float, 2> & mB)
    restrict(amp)
{
    float result = 0.0f;

    for(int i = 0; i < mA.get_extent()[1]; ++i)
    {
        index<2> idxA(idx[0], i);
        index<2> idxB(i, idx[1]);

        result += mA[idxA] * mB[idxB];
    }

    mC[idx] = result;
}

void MatrixMultiplication_cpu(
    const vector<float>& pA,
    const vector<float>& pB,
    vector<float>& pC,
    const unsigned int M,
    const unsigned int W,
    const unsigned int N)
{
    // Compute mxm on CPU
    for(unsigned int i = 0; i < M; ++i)
    {
        for(unsigned int j = 0; j < N; ++j)
        {
            float result = 0.0f;

            for(unsigned int k = 0; k < W; ++k)
            {
                result += pA[i * W + k] * pB[k * N + j];
            }

            pC[i * N + j] = result;
        }
    }
}

const unsigned int M = 4;
const unsigned int W = 4;
const unsigned int N = 4;

runall_result test_d3d_device(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, accelerator_view av)
{
    extent<2> eA(M, W);
    extent<2> eB(W, N);
    extent<2> eC(M, N);

    vector<float> pA(eA.size());
    Fill(pA);

    D3D11_BUFFER_DESC bufferDescription;
    ZeroMemory( &bufferDescription, sizeof( D3D11_BUFFER_DESC ) );
    bufferDescription.ByteWidth = eA.size() * sizeof(float);
    bufferDescription.Usage = D3D11_USAGE_DEFAULT;
    bufferDescription.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    bufferDescription.CPUAccessFlags = 0;
    bufferDescription.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

    ID3D11Buffer *pBufferA = NULL;
    if (CreateD3DBuffer(pDevice, pContext, &bufferDescription, pA.data(), &pBufferA) != S_OK)
    {
        Log(LogType::Error) << "Failed to create D3D buffer." << std::endl;
        return runall_skip;
    }

    vector<float> pB(eB.size());
    Fill(pB);

    bufferDescription.ByteWidth = eB.size() * sizeof(float);
    ID3D11Buffer *pBufferB = NULL;
    if (CreateD3DBuffer(pDevice, pContext, &bufferDescription, pB.data(), &pBufferB) != S_OK)
    {
        Log(LogType::Error) << "Failed to create D3D buffer." << std::endl;
        return runall_skip;
    }

    bufferDescription.ByteWidth = eC.size() * sizeof(float);
    ID3D11Buffer *pBufferC = NULL;
    if (CreateD3DBuffer(pDevice, pContext, &bufferDescription, NULL, &pBufferC) != S_OK)
    {
        Log(LogType::Error) << "Failed to create D3D buffer." << std::endl;
        return runall_skip;
    }
    ID3D11Buffer *pBufferCcopy = NULL;

    // Scoping to force accelerator_view and array clean up
    { 
        Log(LogType::Info) << "Test on device :" << av.get_accelerator().get_description() << std::endl;

        vector<float> pResultGPU(eC.size());

        array<float, 2> fA = make_array<float, 2>(eA, av, pBufferA);
        if (!objects_same(get_buffer(fA), pBufferA) )
        {
            Log(LogType::Error) << "The D3D buffer returned by get_buffer is not same as the original D3D buffer"
                " used to create the array." << std::endl;
            return runall_fail;
        }
        AMP_RELEASE(pBufferA); // Compensate for get_buffer on array

        array<float, 2> fB = make_array<float, 2>(eB, av, pBufferB);
        if (!objects_same(get_buffer(fB), pBufferB) )
        {
            Log(LogType::Error) << "The D3D buffer returned by get_buffer is not same as the original D3D buffer "
                "used to create the array." << std::endl;
            return runall_fail;
        }
        AMP_RELEASE(pBufferB); // Compensate for get_buffer on array

        array<float, 2> fC = make_array<float, 2>(eC, av, pBufferC);
        if (!objects_same(get_buffer(fC), pBufferC) )
        {
            Log(LogType::Error) << "The D3D buffer returned by get_buffer is not same as the original D3D buffer "
                "used to create the array." << std::endl;
            return runall_fail;
        }
        AMP_RELEASE(pBufferC); // Compensate for get_buffer on array

        parallel_for_each(eC, [&](index<2> idx)  restrict(amp)
        {
            MatrixMultiplication_gpu(idx, fC, fA, fB);
        });

        av.flush();

        HRESULT hr = get_buffer(fC)->QueryInterface( __uuidof(ID3D11Buffer),
            reinterpret_cast<void**>(&pBufferCcopy));
        if ((hr != S_OK) || (pBufferCcopy == nullptr))
        {
            Log(LogType::Error) << "Failed to query interface of D3D11 buffer created by an array. hr = 0x" << std::hex << hr << std::endl;
            return runall_fail;
        }
        AMP_RELEASE(pBufferC); // Compensate for AddRef in get_buffer
        AMP_RELEASE(pBufferCcopy); // Compensate for AddRef in QueryInterface

        hr = CopyOut(pDevice, pContext, pBufferCcopy, pResultGPU.data(), pResultGPU.size() * sizeof(float));
        if (hr != S_OK)
        {
            Log(LogType::Error) << "Failed to copy data out of D3D buffer created by an array. hr = 0x" << std::hex << hr << std::endl;
            return runall_fail;
        }

        vector<float> pResultCPU(eC.size());
        MatrixMultiplication_cpu(pA, pB, pResultCPU, M, W, N);

        if (!Verify(pResultGPU, pResultCPU))
            return runall_fail;
    }

    // Cleanup
    Log(LogType::Info) << "Test verifies reference count for all D3D created objects." << std::endl;

    AMP_RELEASE_VERIFY(pBufferA);
    AMP_RELEASE_VERIFY(pBufferB);
    AMP_RELEASE_VERIFY(pBufferC);

    return runall_pass;
}

runall_result test1()
{
    runall_result result;
    HRESULT hr = E_FAIL;
    ID3D11Device *pDevice = NULL;
    ID3D11DeviceContext *pContext = NULL;

    {
        hr = CreateD3DDevice(0, false, &pDevice, &pContext);

        if (hr != S_OK)
        {
            Log(LogType::Error) << "Failed to create D3D device." << std::endl;
            return runall_skip;
        }

        accelerator_view av = create_accelerator_view(pDevice);
        if (!objects_same(get_device(av), pDevice) )
        {
            Log(LogType::Error) << "The D3D device returned by get_device is not same as the original D3D device"
                " used to create the accelerator_view." << std::endl;
            return runall_fail;
        }
        AMP_RELEASE(pDevice); // Compensating for AddRef in get_device call

        result = test_d3d_device( pDevice, pContext, av);
    }

    AMP_RELEASE_VERIFY(pContext);
    AMP_RELEASE_VERIFY(pDevice);

    return result;
}

runall_result test2()
{
    runall_result result;
    HRESULT hr = E_FAIL;
    ID3D11Device *pDevice = NULL;
    ID3D11DeviceContext *pContext = NULL;

    {
        accelerator_view rv = accelerator(accelerator::default_accelerator).get_default_view();

        hr = get_device(rv)->QueryInterface( __uuidof(ID3D11Device), 
            reinterpret_cast<void**>(&pDevice));

        if ((hr != S_OK) || (pDevice == NULL))
        {
            Log(LogType::Error) << "Failed to get the D3D device underlying the accelerator_view. hr = 0x" << std::hex << hr << std::endl;
            return runall_fail;
        }
        AMP_RELEASE(pDevice); // Compensating for AddRef in get_device call

        pDevice->GetImmediateContext(&pContext);

        result = test_d3d_device( pDevice, pContext, accelerator(accelerator::default_accelerator).get_default_view() );
    }

    AMP_RELEASE(pContext);

    AMP_RELEASE(pDevice);

    return result;
}

runall_result test_main()
{
    runall_result result;

    result &= REPORT_RESULT(test1());
    result &= REPORT_RESULT(test2());

    return result;
}
