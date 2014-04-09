#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Shader.h
 * File Description : The shader manager for the HLSL files
 */
#include <d3d11.h>
#include <wrl/client.h>
#include "Graphics/VertexTypes.h"
#include "Graphics/Textures/Texture2D.h"

using namespace Platform;
using namespace Microsoft::WRL;
using namespace Platform;

enum ShaderType : byte
{
	VertexShader,
	PixelShader,
	GeometryShader,
	DomainShader,
	HullShader,
	ComputeShader
};

ref class Shader
{
internal:
	Shader();
	virtual void CreateComputeShader(const void* bytes, SIZE_T Length);
	virtual void CreateVertexShader(const void* bytes, SIZE_T Length, VertexDeclaration VDeclaration);
	virtual void CreatePixelShader(const void* bytes, SIZE_T Length);
	virtual void CreateHullShader(const void* bytes, SIZE_T Length);
	virtual void CreateDomainShader(const void* bytes, SIZE_T Length);
	virtual void CreateGeometryShader(const void* bytes, SIZE_T Length);

	virtual void SetComputeShader(_In_ ID3D11ComputeShader* CS);
	virtual void SetVertexShader(_In_ ID3D11VertexShader* VS);
	virtual void SetPixelShader(_In_ ID3D11PixelShader* PS);
	virtual void SetHullShader(_In_ ID3D11HullShader* HS);
	virtual void SetDomainShader(_In_ ID3D11DomainShader* DS);
	virtual void SetGeometryShader(_In_ ID3D11GeometryShader* GS);


	void SetConstantBuffer(UINT startSlot, UINT numBuffer, _In_ ID3D11Buffer** CBuffer);
	void BindSRV( ShaderType shaderType, UINT startSlot, UINT numBuffer, _In_ ID3D11ShaderResourceView** SRV);
	void BindUAV( UINT startSlot, UINT numBuffer, _In_ ID3D11UnorderedAccessView** UAV);
	void UnbindSRV( ShaderType shaderType, UINT startSlot, UINT numBuffer);
	void UnbindUAV( UINT startSlot, UINT numBuffer);

	void SetInputLayout(ID3D11InputLayout* value);

	void SetTexture2D(UINT StartSLot, UINT NumViews, Texture2D^ texture2D);
	void SetSampler( UINT StartSLot, UINT NumViews, ID3D11SamplerState** Sampler );

	virtual void Apply();
	virtual void ApplyWithDispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);
	virtual void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);
	virtual void EndApply();
	virtual void Disable();
	virtual void Disable( ShaderType shaderType );
	void Release();

	static void LoadShader(Platform::String^ path, ShaderType shaderType,
		VertexDeclaration VDeclaration, _Inout_ Shader^ &shader)
	{
		auto bytes = DX::ReadFile(path);

		switch (shaderType)
		{
		case ShaderType::VertexShader:
			shader->CreateVertexShader(bytes->Data, bytes->Length, VDeclaration);
			break;
		case ShaderType::PixelShader:
			shader->CreatePixelShader(bytes->Data, bytes->Length);
			break;
		case ShaderType::HullShader:
			shader->CreateHullShader(bytes->Data, bytes->Length);
			break;
		case ShaderType::DomainShader:
			shader->CreateDomainShader(bytes->Data, bytes->Length);
			break;
		case ShaderType::GeometryShader:
			shader->CreateGeometryShader(bytes->Data, bytes->Length);
			break;
		case ShaderType::ComputeShader:
			shader->CreateComputeShader(bytes->Data, bytes->Length);
			break;
		default:
			throw ref new Exception(0, "Unknown shader type");
		}
	}

	static Concurrency::task<void> LoadShaderAsync(Platform::String^ path, ShaderType shaderType, 
		VertexDeclaration VDeclaration, _Inout_ Shader^ &shader)
	{
		return DX::ReadFileAsync(path)
			.then([shaderType, VDeclaration, shader](Array<byte>^ bytes)
		{
			switch (shaderType)
			{
			case ShaderType::VertexShader:
				shader->CreateVertexShader(bytes->Data, bytes->Length, VDeclaration);
				break;
			case ShaderType::PixelShader:
				shader->CreatePixelShader(bytes->Data, bytes->Length);
				break;
			case ShaderType::HullShader:
				shader->CreateHullShader(bytes->Data, bytes->Length);
				break;
			case ShaderType::DomainShader:
				shader->CreateDomainShader(bytes->Data, bytes->Length);
				break;
			case ShaderType::GeometryShader:
				shader->CreateGeometryShader(bytes->Data, bytes->Length);
				break;
			case ShaderType::ComputeShader:
				shader->CreateComputeShader(bytes->Data, bytes->Length);
				break;
			default:
				throw ref new Exception(0, "Unknown shader type");
			}
		});
	}

	property bool Loaded
	{
		bool get()
		{
			return this->loaded;
		}
		void set(bool _value)
		{
			this->loaded = _value;
		}
	}
	property ID3D11PixelShader* PixelShader
	{
		ID3D11PixelShader* get()
		{
			return this->pShader.Get();
		}
	}

private:
	bool loaded;
	ComPtr<ID3D11InputLayout> inputLayout;
	ComPtr<ID3D11ComputeShader> cShader;
	ComPtr<ID3D11VertexShader> vShader;
	ComPtr<ID3D11PixelShader> pShader;
	ComPtr<ID3D11HullShader> hShader;
	ComPtr<ID3D11DomainShader> dShader;
	ComPtr<ID3D11GeometryShader> gShader;


	HRESULT CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC*, const UINT, const void*, const SIZE_T);
	HRESULT CreateInputLayout(VertexDeclaration, const void*, const SIZE_T);
};