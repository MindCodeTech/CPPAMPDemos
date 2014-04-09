﻿/*
	FOR GETTING MORE INFORMATION ABOUT THIS CODE PLEASE CHECK http://directx11-1-gameprogramming.azurewebsites.net/
	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
	ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
	PARTICULAR PURPOSE.
	Copyright (c) Microsoft Corporation. All rights reserved

	File Name        : pch.h
	Generated by     : Pooya Eimandar (http://Pooya-Eimandar.com/)
	File Description : 
*/

#pragma once

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfcore.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

#include <wrl/client.h>
#include <d3d11_1.h>
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <d2d1effects.h>
#include <dwrite_1.h>
#include <DirectXMath.h>
#include <Xinput.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <memory>
#include <map>
#include <vector>
#include <ppl.h>
#include <ppltasks.h>
#include <amp.h>
#include <amp_math.h>
#include <assert.h>
#include <agile.h>
#include "windows.ui.xaml.media.dxinterop.h"

#define SafeDelete(x) { if(x){ delete x; x = nullptr; } }
#define SafeUnload(x) { if(x){ x->Unload(); x = nullptr; } }

struct Graphics2D
{
public:
	Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory;
	Microsoft::WRL::ComPtr<ID2D1Device> device;
	Microsoft::WRL::ComPtr<ID2D1DeviceContext> context;
	Microsoft::WRL::ComPtr<ID2D1Factory1> factory;
};

struct Graphics3D
{
public:
	Microsoft::WRL::ComPtr<ID3D11Device1> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> context;
};

class Rasterizer
{
public:
	Microsoft::WRL::ComPtr<ID3D11RasterizerState1> cullBack;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState1> cullFront;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState1> cullNone;
	Rasterizer(Graphics3D G3D)
	{
		D3D11_RASTERIZER_DESC1 rasterDesc;
		ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC1));
		rasterDesc.AntialiasedLineEnable = true;
		rasterDesc.FillMode =  D3D11_FILL_SOLID;
		rasterDesc.MultisampleEnable =  true;
		rasterDesc.CullMode =  D3D11_CULL_BACK;
		auto hr = G3D.device->CreateRasterizerState1(&rasterDesc, &this->cullBack);
		if (FAILED(hr))
		{
			throw Platform::Exception::CreateException(hr);
		}
		rasterDesc.CullMode =  D3D11_CULL_FRONT;
		hr = G3D.device->CreateRasterizerState1(&rasterDesc, &this->cullFront);
		if (FAILED(hr))
		{
			throw Platform::Exception::CreateException(hr);
		}
		rasterDesc.CullMode =  D3D11_CULL_NONE;
		hr = G3D.device->CreateRasterizerState1(&rasterDesc, &this->cullNone);
		if (FAILED(hr))
		{
			throw Platform::Exception::CreateException(hr);
		}
	}
};

enum D3D11_BLEND_MODE { None, Additive, AlphaBlend };
class Blend
{
public:
	Microsoft::WRL::ComPtr<ID3D11BlendState1> none;
	Microsoft::WRL::ComPtr<ID3D11BlendState1> additive;
	Microsoft::WRL::ComPtr<ID3D11BlendState1> alphaBlend;
	Blend(Graphics3D G3D)
	{
		D3D11_BLEND_DESC1 blendDesc;
		ZeroMemory(&blendDesc, sizeof(blendDesc));
		auto hr = G3D.device->CreateBlendState1(&blendDesc, &this->none);
		if (FAILED(hr))
		{
			throw Platform::Exception::CreateException(hr);
		}

		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].LogicOpEnable = false;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		hr = G3D.device->CreateBlendState1(&blendDesc, &this->alphaBlend);
		if (FAILED(hr))
		{
			throw Platform::Exception::CreateException(hr);
		}

		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		hr = G3D.device->CreateBlendState1(&blendDesc, &this->additive);
		if (FAILED(hr))
		{
			throw Platform::Exception::CreateException(hr);
		}
	}
};

class GraphicsDevice
{
private:
	D3D11_BLEND_MODE blendState;
	Blend* blend;

	D3D11_CULL_MODE rasterizerState;
	Rasterizer* rasterizer;
public:
	GraphicsDevice(){}

	float AspectRatio;
	DirectX::XMFLOAT4X4 ViewMatrix;
	DirectX::XMFLOAT4X4 ProjectionMatrix;
	DirectX::XMFLOAT3 EyeVector;
	Graphics2D G2D;
	Graphics3D G3D;

	void Load()
	{
		this->blend = new Blend(this->G3D);
		this->rasterizer = new Rasterizer(G3D);
	}

	void Unload()
	{
		SafeDelete(blend);
	}

#pragma region RasterizerState
	void SetRasterizerState(D3D11_CULL_MODE val)
	{
		if (this->rasterizerState !=  val)
		{
			this->rasterizerState = val;
			switch(val)
			{
			case D3D11_CULL_BACK:
				this->G3D.context->RSSetState(this->rasterizer->cullBack.Get());
				break;
			case D3D11_CULL_FRONT:
				this->G3D.context->RSSetState(this->rasterizer->cullFront.Get());
				break;
			case D3D11_CULL_NONE:
				this->G3D.context->RSSetState(this->rasterizer->cullNone.Get());
				break;
			}
		}
	}
	D3D11_CULL_MODE GetRasterizerState()
	{
		return this->rasterizerState;
	}
	ID3D11RasterizerState1* CreateRasterizerState(D3D11_CULL_MODE cullMode, D3D11_FILL_MODE fillMode)
	{
		ID3D11RasterizerState1* rasterizerState;

		D3D11_RASTERIZER_DESC1 rasterDesc;
		ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC1));
		rasterDesc.AntialiasedLineEnable = true;
		rasterDesc.FillMode =  fillMode;
		rasterDesc.MultisampleEnable =  true;
		rasterDesc.CullMode =  cullMode;
		auto hr = G3D.device->CreateRasterizerState1(&rasterDesc, &rasterizerState);
		if (FAILED(hr))
		{
			throw Platform::Exception::CreateException(hr);
		}
		return rasterizerState;
	}
#pragma endregion

#pragma region BlendState
	void SetBlendState(D3D11_BLEND_MODE val)
	{
		if (this->blendState !=  val)
		{
			this->blendState = val;
			switch(val)
			{
			case None:
				this->G3D.context->OMSetBlendState(this->blend->none.Get(), nullptr, 0xFFFFFFFF);
				break;
			case AlphaBlend:
				this->G3D.context->OMSetBlendState(this->blend->alphaBlend.Get(), nullptr, 0xFFFFFFFF);
				break;
			case Additive:
				this->G3D.context->OMSetBlendState(this->blend->additive.Get(), nullptr, 0xFFFFFFFF);
				break;
			}
		}
	}
	D3D11_BLEND_MODE GetBlendState()
	{
		return this->blendState;
	}
#pragma endregion
};