/*
	FOR GETTING MORE INFORMATION ABOUT THIS CODE PLEASE CHECK http://directx11-1-gameprogramming.azurewebsites.net/
	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
	ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
	PARTICULAR PURPOSE.
	Copyright (c) Microsoft Corporation. All rights reserved

	File Name        : Texture.cpp
	Generated by     : Pooya Eimandar (http://Pooya-Eimandar.com/)
	File Description :
*/

#include "pch.h"
#include "FrameWork\DXHelper.h"
#include "..\DDSTextureLoader.h"
#include "..\Texture.h"

using namespace Concurrency;
using namespace Platform;
using namespace Microsoft::WRL;

Texture::Texture()
{
}

void Texture::Load(Graphics3D G3D, String^ filename)
{
	const Platform::Array<byte>^ textureData = DXHelper::ReadFile(filename);
	{
		auto device = G3D.device.Get();

		CreateDDSTextureFromMemory(
			device,
			textureData->Data,
			textureData->Length,
			nullptr,
			&this->textureView);

		CreateSampler(device);
	};
}

task<void> Texture::LoadAsync(Graphics3D G3D, String^ filename)
{
	return DXHelper::ReadFileAsync(filename).then([=](const Platform::Array<byte>^ textureData)
	{
		auto device = G3D.device.Get();

		CreateDDSTextureFromMemory(
			device,
			textureData->Data,
			textureData->Length,
			nullptr,
			&this->textureView);
		
		CreateSampler(device);
	});
}

void Texture::CreateSampler(ID3D11Device1* device)
{
	// Once the texture view is created, create a sampler.  This defines how the color
	// for a particular texture coordinate is determined using the relevant texture data.
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

	// The sampler does not use anisotropic filtering, so this parameter is ignored.
	samplerDesc.MaxAnisotropy = 0;

	// Specify how texture coordinates outside of the range 0..1 are resolved.
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	// Use no special MIP clamping or bias.
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Don't use a comparison function.
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

	// Border address mode is not used, so this parameter is ignored.
	samplerDesc.BorderColor[0] = 0.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 0.0f;
	samplerDesc.BorderColor[3] = 0.0f;

	
	auto hr = device->CreateSamplerState(&samplerDesc, &sampler);
	DXHelper::ThrowIfFailed(hr);
}
