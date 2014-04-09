#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : StructuredBuffer.h
 * File Description : Presents structured buffer
 */
#include <wrl/client.h>
#include <d3d11.h>

using namespace Microsoft::WRL;

template<typename T>
ref class StructuredBuffer sealed
{
internal:	
	std::vector<T> Data;
	
	StructuredBuffer(){}

	void Load(bool Read_Write)
	{
		using namespace DX;

		auto t = sizeof(T);
		auto NumElements = Data.size();

		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));

		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = t * NumElements;
		bufferDesc.BindFlags = Read_Write ? D3D11_BIND_UNORDERED_ACCESS : D3D11_BIND_SHADER_RESOURCE;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.StructureByteStride = t;
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

		D3D11_SUBRESOURCE_DATA subResourceData;
		subResourceData.pSysMem = &Data[0];

		auto hr = GDevice.d3dDevice->CreateBuffer(&bufferDesc, &subResourceData, this->buffer.GetAddressOf());
		ThrowIfFailed(hr);

		if (Read_Write)
		{
			// Create a memory in RAM to read the results back from VRAM
			bufferDesc.Usage = D3D11_USAGE_STAGING;
			bufferDesc.BindFlags = 0;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			hr = GDevice.d3dDevice->CreateBuffer(&bufferDesc, 0, this->CallBackBuffer.GetAddressOf());
			ThrowIfFailed(hr);

			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.Flags = 0;
			uavDesc.Buffer.NumElements = NumElements;

			hr = GDevice.d3dDevice->CreateUnorderedAccessView(this->buffer.Get(), &uavDesc, this->uav.GetAddressOf());
			ThrowIfFailed(hr);
		}
		else
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
			srvDesc.BufferEx.FirstElement = 0;
			srvDesc.BufferEx.Flags = 0;
			srvDesc.BufferEx.NumElements = NumElements;

			hr = GDevice.d3dDevice->CreateShaderResourceView(this->buffer.Get(), &srvDesc, this->srv.GetAddressOf());
			ThrowIfFailed(hr);
		}
	}
	
	// Looks up the underlying D3D constant buffer.
	property ID3D11Buffer* Buffer
	{
		ID3D11Buffer* get()
		{
			return this->buffer.Get();
		}
	}

	property ID3D11ShaderResourceView** SRV
	{
		ID3D11ShaderResourceView** get()
		{
			return this->srv.GetAddressOf();
		}
	}

	property ID3D11UnorderedAccessView** UAV
	{
		ID3D11UnorderedAccessView** get()
		{
			return this->uav.GetAddressOf();
		}
	}
	
	void Synchronize() 
	{
		GDevice.d3dContext->CopyResource(this->CallBackBuffer.Get(), this->buffer.Get());

		// Map the data for reading.
		D3D11_MAPPED_SUBRESOURCE mappedData; 
		GDevice.d3dContext->Map(this->CallBackBuffer.Get(), 0, D3D11_MAP_READ, 0, &mappedData);
		{
			auto elementsSize = this->Data.size();
			T* dataView = reinterpret_cast<T*>(mappedData.pData);
			if (dataView != nullptr)
			{
				for (int i = 0; i< elementsSize; ++i)
				{
					this->Data.at(i) = dataView[i];
				}
			}
			GDevice.d3dContext->Unmap(this->CallBackBuffer.Get(), 0);
		}
	}

	void Release()
	{
		this->Data.clear();
	}

private:
	ComPtr<ID3D11Buffer> buffer;
	ComPtr<ID3D11ShaderResourceView> srv;
	ComPtr<ID3D11UnorderedAccessView> uav;
	ComPtr<ID3D11Buffer> CallBackBuffer;
};