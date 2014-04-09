/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Quad.cpp
 * File Description : 
 */
#include "pch.h"
#include "Quad.h"
#include "Graphics/VertexTypes.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace DX;

Quad::Quad() : stride(sizeof(VertexTypes::VertexPositionNormalTexture)), offset(0),
	needToUpdate(true), postProcessType(true), needToChangeState(nullptr)
{
}

void Quad::Load()
{
#pragma region Create Vertex Buffer

	const VertexTypes::VertexPositionNormalTexture Vertices[] = 
	{
		{ XMFLOAT3( 1.0f,  1.0f, 0.0f) , XMFLOAT3( 0.0f,  1.0f, 0.0f), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f) , XMFLOAT3( 0.0f,  1.0f, 0.0f), XMFLOAT2( 0.0f, 1.0f ) },
		{ XMFLOAT3(-1.0f,  1.0f, 0.0f) , XMFLOAT3( 0.0f,  1.0f, 0.0f), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, 0.0f) , XMFLOAT3( 0.0f,  1.0f, 0.0f), XMFLOAT2( 1.0f, 1.0f ) },
	};

	D3D11_BUFFER_DESC vertexBufferDesc = {0};
	vertexBufferDesc.ByteWidth = sizeof(VertexTypes::VertexPositionNormalTexture) * ARRAYSIZE(Vertices);
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	vertexBufferData.pSysMem = Vertices;
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;

	ComPtr<ID3D11Buffer> vertexBuffer;	
	HRESULT hr = GDevice.d3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &this->vertexBuffer);
	ThrowIfFailed(hr);

#pragma endregion

#pragma region Create Index Buffer

	const unsigned short Indices[] = {  0, 1, 2, 0, 3, 1};
	this->indicesSize = ARRAYSIZE(Indices);

	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.ByteWidth = sizeof(unsigned short) * ARRAYSIZE(Indices);
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexBufferData;
	indexBufferData.pSysMem = Indices;
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;

	hr = GDevice.d3dDevice->CreateBuffer(&indexBufferDesc, &indexBufferData, &this->indexBuffer);
	ThrowIfFailed(hr);

#pragma endregion

	this->shader =ref new Shader();
	Shader::LoadShader("VertexShader.cso", ShaderType::VertexShader, VertexDeclaration::PositionNormalTexture, this->shader);
	Shader::LoadShader("PixelShader.cso", ShaderType::PixelShader, VertexDeclaration::NOP, this->shader);
	Shader::LoadShader(L"ComputeShader.cso", ShaderType::ComputeShader, VertexDeclaration::NOP, this->shader);

	this->ObjectInforVars.Load();

	this->texture2D = ref new Texture2D();
	Texture2D::LoadTexture(L"Assets/Textures/T0.dds", 8U,
		DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_SHADER_RESOURCE, this->texture2D);

	const int height = this->texture2D->Height; 
	const int width = this->texture2D->Width;

	//Create output texture
	ComPtr<ID3D11Texture2D> texture = nullptr;
	{
		D3D11_TEXTURE2D_DESC desc = {0}; 
		desc.Height = height; 
		desc.Width = width; 
		desc.MipLevels = 1; 
		desc.ArraySize = 1; 
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 
		desc.SampleDesc.Count = 1; 
		desc.SampleDesc.Quality = 0; 
		desc.Usage = D3D11_USAGE_DEFAULT; 
		//D3D11_BIND_SHADER_RESOURCE allows for reading from the texture 
		//and D3D11_BIND_UNORDERED_ACCESS allows for writing to the texture.
		desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE; 
		desc.CPUAccessFlags = 0; 
		desc.MiscFlags = 0;

		auto hr = GDevice.d3dDevice->CreateTexture2D( &desc, nullptr, &texture );
		ThrowIfFailed(hr);
	}

	InitializeAMP(texture);
	InitializeCS(texture);
}

void Quad::InitializeAMP(ComPtr<ID3D11Texture2D> texture)
{		
	auto hr = GDevice.d3dDevice->CreateShaderResourceView(texture.Get(), nullptr, &this->processedResourceView);
	ThrowIfFailed(hr);

	auto writeTexture = make_texture<unorm4, 2>( DX::accViewObj, texture.Get());
	this->amp_textureView = unique_ptr<writeonly_texture_view<unorm4, 2>>(new writeonly_texture_view<unorm4, 2>(writeTexture));
}

void Quad::InitializeCS(ComPtr<ID3D11Texture2D> texture)
{
	this->rwTexture = ref new RWTexture();
	this->rwTexture->Load(texture, XMFLOAT2(0, 0));
}

void Quad::AMPPostProcess()
{
	auto hasNegative = this->negative;
	auto hasLighter = this->highLighter;
	auto hasOddEven = this->oddEven;

	auto input_tex = make_texture<unorm4, 2>( accViewObj, this->texture2D->Texture);
	auto processedView = *this->amp_textureView.get();

	parallel_for_each(input_tex.accelerator_view, processedView.extent, [ = , &input_tex ](index<2> idx) restrict(amp) 
	{
		using namespace fast_math;

		auto Negative = [=] (unorm4 _in) restrict(amp) -> unorm4
		{
			auto rgb = static_cast<unorm3>(1) - _in.rgb;
			return unorm4( rgb.r, rgb.g, rgb.b, 1.0f);
		};

		auto color = input_tex[idx];
		if (hasNegative)
		{			
			color = Negative(input_tex[idx]);
		}
		if (hasLighter)
		{			
			const float weights[3] = { 0.05f, 0.1f, 0.2f };
			for(int i = 0; i < 3; i++) 
			{
				color.rgb += color.rgb * unorm3(weights[i]);
			}
		}
		if (hasOddEven)
		{
			const float _CONST0 = 0.5f;
			const float _CONST1 = 10.0f;

			if (idx[0] % 2 != 0)//if is Odd
			{
				color.rgb += unorm3(sin(color.r * _CONST1) * _CONST0, sin(color.g * _CONST1) * _CONST0, sin(color.b * _CONST1) * _CONST0);
			}
			else
			{
				color.rgb += unorm3(cos(color.r * _CONST1) * _CONST0, cos(color.g * _CONST1) * _CONST0, cos(color.b * _CONST1) * _CONST0);
			}
		}

		processedView.set(idx, color);
	});
	this->ObjectInforVars.Update();
	this->shader->SetConstantBuffer(0, 1, this->ObjectInforVars.Buffer);	
}

void Quad::CSPostProcess()
{
	//Set type of post process
	this->ObjectInforVars.Const.PixelState.x =	this->negative ? 1.0f : 0.0f;
	this->ObjectInforVars.Const.PixelState.y =	this->highLighter  ? 1.0f : 0.0f;
	this->ObjectInforVars.Const.PixelState.z =	this->oddEven  ? 1.0f : 0.0f;
	this->ObjectInforVars.Update();
	this->shader->SetConstantBuffer(0, 1, this->ObjectInforVars.Buffer);	

	//Set Texture
	this->shader->BindSRV(ShaderType::ComputeShader, 0, 1, this->texture2D->SRV);

	//Set RWTexture2D
	this->shader->BindSRV(ShaderType::ComputeShader, 1, 1, this->rwTexture->SRV);
	this->shader->BindUAV(0, 1, this->rwTexture->UAV);

	UINT groupThreadsX = static_cast<UINT>(ceilf(this->texture2D->Width / 256.0f));
	this->shader->Dispatch(groupThreadsX, this->texture2D->Height, 1);
	// Unbind the input texture from the CS for good housekeeping.
	this->shader->UnbindSRV(ShaderType::ComputeShader, 1, 1);
	// Unbind output from compute shader, a resource cannot be both an output and input at the same time.
	this->shader->UnbindUAV(0, 1);
}

void Quad::Render()
{
	if (this->needToChangeState != nullptr)
	{
		this->postProcessType = this->needToChangeState;
		this->needToUpdate = true;
		this->needToChangeState = nullptr;
	}
	//Update constant buffer
	auto world = XMMatrixIdentity();
	auto view = Camera->View;
	auto projection = Camera->Projection;
	this->ObjectInforVars.Const.WorldViewProj = XMMatrixTranspose(world * view * projection);
	
	//Set Sampler State
	auto sampler = GDevice.Samplers.at(0);
	this->shader->SetSampler(0, 1, sampler.GetAddressOf());

	if (this->needToUpdate)
	{
		this->needToUpdate = false;
		if (this->postProcessType)
		{
			//Use Compute Shader
			CSPostProcess();
		}
		else
		{
			//Use C++AMP
			AMPPostProcess();
		}
	}

	//Should update in each frame
	if ( this->postProcessType)
	{
		//Use Compute Shader
		this->shader->BindSRV(ShaderType::PixelShader, 0, 1, this->rwTexture->SRV);
	}
	else
	{
		//Use C++AMP
		this->shader->BindSRV(ShaderType::PixelShader ,0, 1, this->processedResourceView.GetAddressOf());
	}
	
	this->shader->Apply();
	{
		GDevice.d3dContext->IASetVertexBuffers(0, 1, this->vertexBuffer.GetAddressOf(), &this->stride, &this->offset );
		GDevice.d3dContext->IASetIndexBuffer( this->indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0 );
		GDevice.d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		GDevice.d3dContext->DrawIndexed(this->indicesSize, 0, 0 );
		this->shader->EndApply();
	}
}