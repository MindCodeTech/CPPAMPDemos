#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Quad.h
 * File Description : Responsible for creating and rendering a quad 
 */
#include <d3d11_1.h>
#include <wrl/client.h>
#include "Graphics/Shaders/Shader.h"
#include "Graphics/Shaders/CBuffer.h"
#include "Graphics/Textures/RWTexture.h"

using namespace std;
using namespace Concurrency;
using namespace concurrency::direct3d;
using namespace concurrency::graphics;
using namespace concurrency::graphics::direct3d;

struct ObjectInfo
{
	DirectX::XMMATRIX WorldViewProj;
	DirectX::XMFLOAT4 PixelState;
	ObjectInfo()
	{
		ZeroMemory(this, sizeof(ObjectInfo));
		PixelState.x = PixelState.y = PixelState.z = PixelState.w = 0;
	}
};

ref class Quad
{
internal:
	//Constructor 
	Quad();
	void InitializeAMP();
	void InitializeCS();
	void Load();
	void Render();
	
	property bool Negative
	{
		bool get()
		{
			return this->negative;
		}
		void set(bool val)
		{
			if (this->negative != val)
			{
				this->needToUpdate = true;
				this->negative = val;
			}
		}
	}
	property bool HighLighter
	{
		bool get()
		{
			return this->highLighter;
		}
		void set(bool val)
		{
			if (this->highLighter != val)
			{
				this->needToUpdate = true;
				this->highLighter = val;
			}
		}
	}
	property bool OddEven
	{
		bool get()
		{
			return this->oddEven;
		}
		void set(bool val)
		{
			if (this->oddEven != val)
			{
				this->needToUpdate = true;
				this->oddEven = val;
			}
		}
	}
	property bool PostProcessType
	{
		bool get()
		{
			return this->postProcessType;
		}
		void set(bool val)
		{
			if (this->postProcessType != val)
			{
				this->needToChangeState = &val;
			}
		}
	}

private:
	bool needToUpdate;
	bool negative;
	bool highLighter;
	bool oddEven;
	bool postProcessType;
	bool* needToChangeState;

	UINT indicesSize;
	UINT stride;
	UINT offset;
	Shader^ shader;
	Texture2D^ texture2D;
	RWTexture^ rwTexture;
	CBuffer<ObjectInfo> ObjectInforVars;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	unique_ptr<texture<unorm4, 2>> readTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> processedResourceView;
	unique_ptr<writeonly_texture_view<unorm4, 2>> amp_textureView;
	
	void InitializeAMP(Microsoft::WRL::ComPtr<ID3D11Texture2D> texture);
	void InitializeCS(Microsoft::WRL::ComPtr<ID3D11Texture2D> texture);
	void AMPPostProcess();
	void CSPostProcess();
};