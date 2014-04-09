/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : RWTexture.cpp
 * File Description : 
 */
#include "pch.h"
#include "RWTexture.h"

using namespace Microsoft::WRL;

RWTexture::RWTexture()
{

}

void RWTexture::Load(DirectX::XMFLOAT2 TextureSize)
{
	Load(nullptr, TextureSize);
}

void RWTexture::Load(ComPtr<ID3D11Texture2D> texture2D, DirectX::XMFLOAT2 TextureSize)
{
	using namespace DX;

	HRESULT hr = S_FALSE;

	//Create texture2D
	if (texture2D == nullptr)
	{
		D3D11_TEXTURE2D_DESC textureDesc;
		textureDesc.Width     = TextureSize.x;
		textureDesc.Height    = TextureSize.y;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.SampleDesc.Count   = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage     = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags      = 0;

		hr = GDevice.d3dDevice->CreateTexture2D(&textureDesc, 0, texture2D.GetAddressOf());
		ThrowIfFailed(hr);
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	hr = GDevice.d3dDevice->CreateShaderResourceView(texture2D.Get(), &srvDesc, &this->srv);
	ThrowIfFailed(hr);

	//Create UAV
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	hr = GDevice.d3dDevice->CreateUnorderedAccessView(texture2D.Get(), &uavDesc, &this->uav);
	ThrowIfFailed(hr);
}