/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Shader.cpp
 * File Description : 
 */
#include "pch.h"
#include "Shader.h"
#include "Graphics/VertexTypes.h"

using namespace DirectX; 
using namespace Concurrency;
using namespace Platform;
using namespace DX;

Shader::Shader()
{
}

void Shader::CreateComputeShader(const void* bytes, SIZE_T Length)
{
	auto hr = GDevice.d3dDevice->CreateComputeShader(bytes, Length, nullptr, &this->cShader);
	ThrowIfFailed(hr);
}

void Shader::CreateVertexShader(const void* bytes, SIZE_T Length, VertexDeclaration VDeclaration)
{
	auto hr = GDevice.d3dDevice->CreateVertexShader(bytes, Length, nullptr, &this->vShader);
	ThrowIfFailed(hr);
	hr = CreateInputLayout(VDeclaration, bytes, Length);
	ThrowIfFailed(hr);
}

void Shader::CreatePixelShader(const void* bytes, SIZE_T Length)
{
	auto hr = GDevice.d3dDevice->CreatePixelShader(bytes, Length, nullptr, &this->pShader);
	ThrowIfFailed(hr);
}

void Shader::CreateHullShader(const void* bytes, SIZE_T Length)
{
	auto hr = GDevice.d3dDevice->CreateHullShader(bytes, Length, nullptr, &this->hShader);
	ThrowIfFailed(hr);
}

void Shader::CreateDomainShader(const void* bytes, SIZE_T Length)
{
	auto hr = GDevice.d3dDevice->CreateDomainShader(bytes, Length, nullptr, &this->dShader);
	ThrowIfFailed(hr);
}

void Shader::CreateGeometryShader(const void* bytes, SIZE_T Length)
{
	auto hr = GDevice.d3dDevice->CreateGeometryShader(bytes, Length, nullptr, &this->gShader);
	ThrowIfFailed(hr);
}

void Shader::SetComputeShader(_In_  ID3D11ComputeShader* CS)
{
	this->cShader = CS;
}

void Shader::SetVertexShader(_In_  ID3D11VertexShader* VS)
{
	this->vShader = VS;
}

void Shader::SetPixelShader(_In_  ID3D11PixelShader* PS)
{
	this->pShader = PS;
}

void Shader::SetHullShader(_In_ ID3D11HullShader* HS)
{
	this->hShader = HS;
}

void Shader::SetDomainShader(_In_ ID3D11DomainShader* DS)
{
	this->dShader = DS;
}

void Shader::SetGeometryShader(_In_ ID3D11GeometryShader* GS)
{
	this->gShader = GS;
}

void Shader::SetConstantBuffer(UINT startSlot, UINT numBuffer, _In_ ID3D11Buffer** CBuffer)
{
	if (vShader != nullptr)	GDevice.d3dContext->VSSetConstantBuffers(startSlot, numBuffer, CBuffer);
	if (pShader != nullptr) GDevice.d3dContext->PSSetConstantBuffers(startSlot, numBuffer, CBuffer);
	if (hShader != nullptr) GDevice.d3dContext->HSSetConstantBuffers(startSlot, numBuffer, CBuffer);
	if (dShader != nullptr) GDevice.d3dContext->DSSetConstantBuffers(startSlot, numBuffer, CBuffer);
	if (cShader != nullptr) GDevice.d3dContext->CSSetConstantBuffers(startSlot, numBuffer, CBuffer);
}

void Shader::BindSRV( ShaderType shaderType, UINT startSlot, UINT numBuffer, _In_ ID3D11ShaderResourceView** SRV)
{
	switch (shaderType)
	{
	case ShaderType::VertexShader:
		GDevice.d3dContext->VSSetShaderResources(startSlot, numBuffer, SRV);
		break;
	case ShaderType::PixelShader:
		GDevice.d3dContext->PSSetShaderResources(startSlot, numBuffer, SRV);
		break;
	case ShaderType::GeometryShader:
		GDevice.d3dContext->GSSetShaderResources(startSlot, numBuffer, SRV);
		break;
	case ShaderType::DomainShader:
		GDevice.d3dContext->DSSetShaderResources(startSlot, numBuffer, SRV);
		break;
	case ShaderType::HullShader:
		GDevice.d3dContext->HSSetShaderResources(startSlot, numBuffer, SRV);
		break;
	case ShaderType::ComputeShader:
		GDevice.d3dContext->CSSetShaderResources(startSlot, numBuffer, SRV);
		break;
	default:
		throw ref new Exception(0, "Unknown shader type");
	}
}

void Shader::BindUAV( UINT startSlot, UINT numBuffer, _In_ ID3D11UnorderedAccessView** UAV)
{
	GDevice.d3dContext->CSSetUnorderedAccessViews(startSlot, numBuffer, UAV, 0);
}

void Shader::UnbindSRV( ShaderType shaderType, UINT startSlot, UINT numBuffer)
{
	ID3D11ShaderResourceView* srv = nullptr;
	BindSRV(shaderType, startSlot, numBuffer, &srv);
}

void Shader::UnbindUAV( UINT startSlot, UINT numBuffer)
{
	ID3D11UnorderedAccessView* uav = nullptr;
	BindUAV(startSlot, numBuffer, &uav);
}

void Shader::SetInputLayout(ID3D11InputLayout* value)
{
	this->inputLayout = value;
}

void Shader::SetTexture2D(UINT StartSLot, UINT NumViews, Texture2D^ texture2D)
{
	if (GDevice.Samplers.size() != 0)
	{
		auto sampler = GDevice.Samplers.at(0).GetAddressOf();

		if (pShader != nullptr)
		{
			GDevice.d3dContext->PSSetShaderResources(StartSLot, NumViews, texture2D->SRV);
			GDevice.d3dContext->PSSetSamplers(StartSLot, NumViews, sampler);
		}

		if (dShader != nullptr)
		{
			GDevice.d3dContext->DSSetShaderResources(StartSLot, NumViews, texture2D->SRV);
			GDevice.d3dContext->DSSetSamplers(StartSLot, NumViews, sampler);
		}

		if (cShader != nullptr)
		{
			GDevice.d3dContext->CSSetShaderResources(StartSLot, NumViews, texture2D->SRV);
			GDevice.d3dContext->CSSetSamplers(StartSLot, NumViews, sampler);
		}
	}
}

void Shader::SetSampler( UINT StartSLot, UINT NumViews, ID3D11SamplerState** Sampler )
{
	GDevice.d3dContext->PSSetSamplers(StartSLot, NumViews, Sampler);
}

void Shader::Apply()
{
	GDevice.d3dContext->IASetInputLayout(this->inputLayout.Get());
	GDevice.d3dContext->VSSetShader(this->vShader != nullptr ? this->vShader.Get() : nullptr, nullptr, 0);
	GDevice.d3dContext->HSSetShader(this->hShader != nullptr ? this->hShader.Get() : nullptr, nullptr, 0);
	GDevice.d3dContext->DSSetShader(this->dShader != nullptr ? this->dShader.Get() : nullptr, nullptr, 0);
	GDevice.d3dContext->GSSetShader(this->gShader != nullptr ? this->gShader.Get() : nullptr, nullptr, 0);
	GDevice.d3dContext->PSSetShader(this->pShader != nullptr ? this->pShader.Get() : nullptr, nullptr, 0);
}

void Shader::ApplyWithDispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
	GDevice.d3dContext->IASetInputLayout(this->inputLayout.Get());
	GDevice.d3dContext->VSSetShader(this->vShader != nullptr ? this->vShader.Get() : nullptr, nullptr, 0);
	GDevice.d3dContext->HSSetShader(this->hShader != nullptr ? this->hShader.Get() : nullptr, nullptr, 0);
	GDevice.d3dContext->DSSetShader(this->dShader != nullptr ? this->dShader.Get() : nullptr, nullptr, 0);
	GDevice.d3dContext->GSSetShader(this->gShader != nullptr ? this->gShader.Get() : nullptr, nullptr, 0);
	GDevice.d3dContext->PSSetShader(this->pShader != nullptr ? this->pShader.Get() : nullptr, nullptr, 0);
	Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void Shader::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
	if (this->cShader != nullptr)
	{
		GDevice.d3dContext->CSSetShader(this->cShader.Get(), nullptr, 0);
		GDevice.d3dContext->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}
}

void Shader::EndApply()
{
	// Clean the pipeline and make it unbind
	ID3D11ShaderResourceView * NullSRV = nullptr;
	GDevice.d3dContext->PSSetShaderResources(0, 1, &NullSRV);
	Disable();
}

HRESULT Shader::CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* vertexDesc, const UINT vertexDescLength,
								  const void* shaderData, const SIZE_T shaderDataLength)
{
	return GDevice.d3dDevice->CreateInputLayout(
		vertexDesc,
		vertexDescLength,
		shaderData,
		shaderDataLength,
		this->inputLayout.GetAddressOf());
};

HRESULT Shader::CreateInputLayout(VertexDeclaration vertexTypes, const void* shaderData, SIZE_T shaderDataLength)
{
	HRESULT hr = S_OK;
	if (vertexTypes == Position)
	{
		D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		hr = CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), shaderData, shaderDataLength);
	}
	else if( vertexTypes == PositionTexture)
	{
		const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		hr = CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), shaderData, shaderDataLength);
	}
	else if( vertexTypes == PositionColor)
	{
		const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		hr = CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), shaderData, shaderDataLength);
	}
	else if( vertexTypes == PositionNormalColor)
	{
		const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		hr = CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), shaderData, shaderDataLength);
	}
	else if( vertexTypes == PositionNormalTexture)
	{
		const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		hr = CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), shaderData, shaderDataLength);
	}
	else if( vertexTypes == PositionNormalTextureTangent)
	{
		const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		hr = CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), shaderData, shaderDataLength);
	}
	else if( vertexTypes == PositionNormalTextureTangentBinormal)
	{
		const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		hr = CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), shaderData, shaderDataLength);
	}
	else if( vertexTypes == PositionNormalTangentColorTexture)
	{
		const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		hr = CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), shaderData, shaderDataLength);
	}
	else
	{
		hr = S_FALSE;
		throw ref new Exception(0, "Unknown vertex format");
	}
	return hr;
}

void Shader::Disable()
{
	GDevice.d3dContext->VSSetShader(nullptr, nullptr, 0);
	GDevice.d3dContext->PSSetShader(nullptr, nullptr, 0);
	GDevice.d3dContext->GSSetShader(nullptr, nullptr, 0);
	GDevice.d3dContext->DSSetShader(nullptr, nullptr, 0);
	GDevice.d3dContext->HSSetShader(nullptr, nullptr, 0);
	GDevice.d3dContext->CSSetShader(nullptr, nullptr, 0);
}

void Shader::Disable( ShaderType shaderType )
{
	switch (shaderType)
	{
	case ShaderType::VertexShader:
		GDevice.d3dContext->VSSetShader(nullptr, nullptr, 0);
		break;
	case ShaderType::PixelShader:
		GDevice.d3dContext->PSSetShader(nullptr, nullptr, 0);
		break;
	case ShaderType::GeometryShader:
		GDevice.d3dContext->GSSetShader(nullptr, nullptr, 0);
		break;
	case ShaderType::DomainShader:
		GDevice.d3dContext->DSSetShader(nullptr, nullptr, 0);
		break;
	case ShaderType::HullShader:
		GDevice.d3dContext->HSSetShader(nullptr, nullptr, 0);
		break;
	case ShaderType::ComputeShader:
		GDevice.d3dContext->CSSetShader(nullptr, nullptr, 0);
		break;
	default:
		throw ref new Exception(0, "Unknown shader type");
	}
}

void Shader::Release()
{
}

