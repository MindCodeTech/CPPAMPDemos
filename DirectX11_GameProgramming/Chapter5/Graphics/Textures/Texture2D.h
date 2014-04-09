#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Texture2D.h
 * File Description : Load 2D texture
 */
#include "DDSTextureLoader.h"

using namespace Microsoft::WRL;

ref class Texture2D
{
internal:
	Texture2D();
	void Load(ComPtr<ID3D11Resource> resource, _In_ ID3D11ShaderResourceView* shaderResourceView);

	static void LoadTexture(Platform::String^ path, _Inout_ Texture2D^ &texture)
	{
		const Platform::Array<byte>^ textureData = DX::ReadFile(path);
		{
			ComPtr<ID3D11Resource> resource;
			ID3D11ShaderResourceView* srv = nullptr; 
			auto device = DX::GDevice.d3dDevice.Get();

			CreateDDSTextureFromMemory(
				device,
				textureData->Data,
				textureData->Length,
				&resource,
				&srv,
				0,
				DXGI_FORMAT_B8G8R8A8_UNORM,
				D3D11_BIND_SHADER_RESOURCE);

			texture->Load(resource, srv);
		}
	}

	static void LoadTexture(Platform::String^ path, size_t maxSize, DXGI_FORMAT format, UINT bindFlags, _Inout_ Texture2D^ &texture)
	{
		const Platform::Array<byte>^ textureData = DX::ReadFile(path);
		{
			ComPtr<ID3D11Resource> resource;
			ID3D11ShaderResourceView* srv = nullptr; 
			auto device = DX::GDevice.d3dDevice.Get();

			CreateDDSTextureFromMemory(
				device,
				textureData->Data,
				textureData->Length,
				&resource,
				&srv,
				maxSize,
				format,
				bindFlags);

			texture->Load(resource, srv);
		}
	}

	static Concurrency::task<void> LoadTextureAsync(Platform::String^ path, _Inout_ Texture2D^ &texture)
	{
		return DX::ReadFileAsync(path)
			.then([texture](const Platform::Array<byte>^ textureData)
		{
			ComPtr<ID3D11Resource> resource;
			ID3D11ShaderResourceView* srv = nullptr;
			auto device = DX::GDevice.d3dDevice.Get();

			CreateDDSTextureFromMemory(
				device,
				textureData->Data,
				textureData->Length,
				&resource,
				&srv,
				0,
				DXGI_FORMAT_B8G8R8A8_UNORM,
				D3D11_BIND_SHADER_RESOURCE);

			texture->Load(resource, srv);
		});
	}

	static Concurrency::task<void> LoadTextureAsync(Platform::String^ path, size_t maxSize, DXGI_FORMAT format, UINT bindFlags, _Inout_ Texture2D^ &texture)
	{
		return DX::ReadFileAsync(path)
			.then([maxSize, format, bindFlags, texture](const Platform::Array<byte>^ textureData)
		{
			ComPtr<ID3D11Resource> resource;
			ID3D11ShaderResourceView* srv = nullptr;
			auto device = DX::GDevice.d3dDevice.Get();

			CreateDDSTextureFromMemory(
				device,
				textureData->Data,
				textureData->Length,
				&resource,
				&srv,
				maxSize,
				format,
				bindFlags);

			texture->Load(resource, srv);
		});
	}

	static void CreateSampler(const D3D11_FILTER filter, const D3D11_TEXTURE_ADDRESS_MODE Address[3])
	{
		// Once the texture view is created, create a sampler.  This defines how the color
		// for a particular texture coordinate is determined using the relevant texture data.
		D3D11_SAMPLER_DESC samplerDesc;
		ZeroMemory(&samplerDesc, sizeof(samplerDesc));

		samplerDesc.Filter = filter;

		// The sampler does not use anisotropic filtering, so this parameter is ignored.
		samplerDesc.MaxAnisotropy = 0;

		// Specify how texture coordinates outside of the range 0..1 are resolved.
		samplerDesc.AddressU = Address[0];
		samplerDesc.AddressV = Address[1];
		samplerDesc.AddressW = Address[2];

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

		using namespace DX;
		using namespace Microsoft::WRL;
		
		ComPtr<ID3D11SamplerState> sampler;
		auto hr = GDevice.d3dDevice->CreateSamplerState(&samplerDesc, &sampler);
		ThrowIfFailed(hr);
		GDevice.Samplers.push_back(sampler);
	}

	property ID3D11ShaderResourceView** SRV
	{
		ID3D11ShaderResourceView** get() 
		{ 
			return this->srv.GetAddressOf(); 
		}
	}

	property ID3D11Texture2D* Texture
	{
		ID3D11Texture2D* get() 
		{ 
			return this->texture2D.Get(); 
		}
	}

	property int Height
	{
		int get() 
		{ 
			return this->texDesc.Height; 
		}
	}
	property int Width
	{
		int get() 
		{ 
			return this->texDesc.Width; 
		}
	}
private:
	ComPtr<ID3D11Texture2D> texture2D;
	ComPtr<ID3D11ShaderResourceView> srv;
	D3D11_TEXTURE2D_DESC texDesc;
};