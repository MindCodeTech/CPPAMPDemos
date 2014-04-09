// Copyright (c) Microsoft
// All rights reserved
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.

// Test wrapper for DirectX API's

#pragma once

#include <d3d11.h>

#pragma comment(lib, "d3d11")

HRESULT CreateD3DDevice(
    UINT createDeviceFlags,
    bool reference,
    ID3D11Device **ppDevice,
    ID3D11DeviceContext **ppContext)
{
    if ((ppDevice == NULL) || (ppContext == NULL)) return E_FAIL;

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    D3D_DRIVER_TYPE driverType;
    if (reference)
    {
        driverType = D3D_DRIVER_TYPE_REFERENCE;
    }
    else
    {
        driverType = D3D_DRIVER_TYPE_HARDWARE;
    }

    D3D_FEATURE_LEVEL featureLevel;

    return D3D11CreateDevice(
        NULL,
        driverType,
        NULL,
        createDeviceFlags,
        featureLevels,
        1,
        D3D11_SDK_VERSION,
        ppDevice,
        &featureLevel,
        ppContext);

}

HRESULT CreateD3DBuffer(
    ID3D11Device *pDevice,
    ID3D11DeviceContext *pContext,
    const D3D11_BUFFER_DESC *bufferDescription,
    float* pData,
    ID3D11Buffer **ppBuffer)
{
    if ( (bufferDescription == NULL) 	|| 
        (pDevice == NULL) 			|| 
        (pContext == NULL) 			||
        (ppBuffer == NULL))
        return E_FAIL;

    if (pDevice->CreateBuffer(bufferDescription, NULL, ppBuffer) != S_OK) return E_FAIL;

    if (NULL != pData)
    {
        D3D11_BUFFER_DESC stagingBufferDescription;
        ZeroMemory( &stagingBufferDescription, sizeof( D3D11_BUFFER_DESC ) );
        stagingBufferDescription.ByteWidth = bufferDescription->ByteWidth;
        stagingBufferDescription.Usage = D3D11_USAGE_STAGING;
        stagingBufferDescription.BindFlags = 0;
        stagingBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
        stagingBufferDescription.MiscFlags = 0;

        ID3D11Buffer *pStagingBuffer = NULL;
        if ( pDevice->CreateBuffer(&stagingBufferDescription, NULL, &pStagingBuffer) 
            != S_OK) 
            return E_FAIL;
        D3D11_MAPPED_SUBRESOURCE dOutBuf;
        if ( pContext->Map(pStagingBuffer, 0, D3D11_MAP_WRITE, 0, &dOutBuf) 
            != S_OK) 
            return E_FAIL;

        memcpy(dOutBuf.pData, pData, bufferDescription->ByteWidth);
        pContext->Unmap(pStagingBuffer, 0);

        D3D11_BOX box;
        box.left = 0;
        box.top = 0;
        box.front = 0;
        box.right = bufferDescription->ByteWidth;
        box.bottom = 1;
        box.back = 1;

        pContext->CopySubresourceRegion(*ppBuffer, 0, 0, 0, 0, pStagingBuffer, 0, &box);
        pStagingBuffer->Release();
    }

    return S_OK;
}

HRESULT CopyOut(
    ID3D11Device *pDevice,
    ID3D11DeviceContext *pContext,
    ID3D11Buffer *pBuffer,
    void *pData,
    size_t szData)
{
    HRESULT hr = 0;

    if ( (pDevice == NULL) 	||
        (pContext == NULL) 	||
        (pBuffer == NULL) 	||
        (pData == NULL)) 
        return E_FAIL;

    D3D11_BUFFER_DESC bufferDescription;
    pBuffer->GetDesc(&bufferDescription);

    D3D11_BUFFER_DESC stagingBufferDescription;
    ZeroMemory( &stagingBufferDescription, sizeof( D3D11_BUFFER_DESC ) );
    stagingBufferDescription.ByteWidth = bufferDescription.ByteWidth;
    stagingBufferDescription.Usage = D3D11_USAGE_STAGING;
    stagingBufferDescription.BindFlags = 0;
    stagingBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
    stagingBufferDescription.MiscFlags = 0;

    ID3D11Buffer *pStagingBuffer = NULL;
    if ( (hr = pDevice->CreateBuffer(&stagingBufferDescription, NULL, &pStagingBuffer))
        != S_OK)
        return hr;

    D3D11_BOX box;
    box.left = 0;
    box.top = 0;
    box.front = 0;
    box.right = bufferDescription.ByteWidth;
    box.bottom = 1;
    box.back = 1;
    pContext->CopySubresourceRegion(pStagingBuffer, 0, 0, 0, 0, pBuffer, 0, &box);

    D3D11_MAPPED_SUBRESOURCE dOutBuf;

    if ( (hr = pContext->Map(pStagingBuffer, 0, D3D11_MAP_WRITE, 0, &dOutBuf))
        != S_OK)
        return hr;

    memcpy_s(pData, szData, dOutBuf.pData, bufferDescription.ByteWidth);
    pContext->Unmap(pStagingBuffer, 0);

    pStagingBuffer->Release();

    return S_OK;
}
