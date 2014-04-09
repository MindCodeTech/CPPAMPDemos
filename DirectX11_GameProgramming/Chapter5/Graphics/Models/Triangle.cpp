/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Triangle.cpp
 * File Description : 
 */
#include "pch.h"
#include "Triangle.h"
#include "Graphics/VertexTypes.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace DX;

Triangle::Triangle() : stride(sizeof(VertexTypes::VertexPositionNormalTexture)), offset(0)
{
}

void Triangle::Load()
{
#pragma region Create Vertex Buffer

		const VertexTypes::VertexPositionNormalTexture Vertices[] = 
		{
			{ XMFLOAT3( -1.0f, -1.0f, 0.0f) ,  XMFLOAT3( 0.0f,  1.0f, 0.0f) , XMFLOAT2( 1.0f, 0.0f ) },
			{ XMFLOAT3(  0.0f,  1.0f, 0.0f) ,  XMFLOAT3( 0.0f,  1.0f, 0.0f) , XMFLOAT2( 0.0f, 1.0f ) },
			{ XMFLOAT3(  1.0f, -1.0f, 0.0f) ,  XMFLOAT3( 0.0f,  1.0f, 0.0f) , XMFLOAT2( 0.0f, 0.0f ) },
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
		auto hr = GDevice.d3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &this->vertexBuffer);
		ThrowIfFailed(hr);

#pragma endregion

#pragma region Create Index Buffer

		const unsigned short Indices[] = {  0, 1, 2 };
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
	Shader::LoadShader("TriHullShader.cso", ShaderType::HullShader, VertexDeclaration::NOP, this->shader);
	Shader::LoadShader("TriDomainShader.cso", ShaderType::DomainShader, VertexDeclaration::NOP, this->shader);

	this->cBuffer.Load();
}

void Triangle::Render()
{
	auto world = XMMatrixIdentity();
	auto view = Camera->View;
	auto projection = Camera->Projection;

	this->cBuffer.Const.WorldViewProjection = XMMatrixTranspose(world * view * projection);

	this->cBuffer.Update();
	this->shader->SetConstantBuffer(0, 1, this->cBuffer.Buffer);
	this->shader->Apply();
	{
		GDevice.d3dContext->IASetVertexBuffers(0, 1, this->vertexBuffer.GetAddressOf(), &this->stride, &this->offset );
		GDevice.d3dContext->IASetIndexBuffer( this->indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0 );
		GDevice.d3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		GDevice.d3dContext->DrawIndexed(this->indicesSize, 0, 0 );
	}
}

float Triangle::GetTessEdge()
{
	return this->cBuffer.Const.TessEdge;
}

float Triangle::GetTessInside()
{
	return this->cBuffer.Const.TessInside;
}

void Triangle::SetTessEdge(float _value)
{
	this->cBuffer.Const.TessEdge = _value;
}

void Triangle::SetTessInside(float _value)
{
	this->cBuffer.Const.TessInside = _value;
}