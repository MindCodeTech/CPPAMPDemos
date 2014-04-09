#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : CBuffer.h
 * File Description : This header creates a constant buffer
 */
#include <wrl/client.h>
#include <d3d11.h>
#include <DirectXMath.h>

using namespace Microsoft::WRL;

template<typename T>
ref class CBuffer sealed
{
internal:	
	T Const;

	CBuffer(){}
	
	void Load()
	{
		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.ByteWidth = sizeof(T);
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		auto hr = DX::GDevice.d3dDevice->CreateBuffer(&bufferDesc, nullptr, &this->buffer);
		ThrowIfFailed(hr);
	}

	// Writes new data into the constant buffer.
	void SetData(T& value)
	{
		this->Const.reset(&value);
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		auto hr = DX::GDevice.d3dContext->Map(buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		ThrowIfFailed(hr);
		*(T*)mappedResource.pData = value;
		DX::GDevice.d3dContext->Unmap(buffer.get(), 0);
	}

	// Looks up the underlying D3D constant buffer.
	property ID3D11Buffer** Buffer
	{
		ID3D11Buffer** get()
		{
			return this->buffer.GetAddressOf();
		}
	}

	void Update()
	{
		DX::GDevice.d3dContext->UpdateSubresource(this->buffer.Get(), 0, nullptr, &this->Const, 0, 0 );
	}

	void Release()
	{
		this->buffer->Release();
	}

private:
	ComPtr<ID3D11Buffer> buffer;
};