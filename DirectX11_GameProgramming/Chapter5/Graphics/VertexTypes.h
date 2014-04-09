#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : SpriteBatch.h
 * File Description : The declaration of vertex types
 */
#include <wrl/client.h>
#include <DirectXMath.h>

using namespace DirectX;

enum VertexDeclaration : byte
{
	NOP,
	Position,
	PositionColor,
	PositionNormalColor,
	PositionTexture,
	PositionNormalTexture,
	PositionNormalTextureTangent,
	PositionNormalTextureTangentBinormal,
	PositionNormalTangentColorTexture
};

namespace VertexTypes
{
	struct VertexPosition
	{
		XMFLOAT3 Position;
	};

	struct VertexPositionColor
	{
		XMFLOAT3 Position;
		XMFLOAT4 Color;
	};

	struct VertexPositionNormalColor
	{
		XMFLOAT3 Position;
		XMFLOAT3 Normal;
		XMFLOAT4 Color;
	};
	
	struct VertexPositionTexture
	{
		XMFLOAT3 Position;
		XMFLOAT2 TexCoord;
	};

	struct VertexPositionNormalTexture
	{
		XMFLOAT3 Position;
		XMFLOAT3 Normal;
		XMFLOAT2 TexCoord;
	}; 

	struct VertexPositionNormalTextureTangent
	{
		XMFLOAT3 Position;
		XMFLOAT3 Normal;
		XMFLOAT2 Texcoord;
		XMFLOAT3 Tangent;
	};

	struct VertexPositionNormalTextureTangentBinormal
	{
		XMFLOAT3 Position;
		XMFLOAT3 Normal;
		XMFLOAT2 Texcoord;
		XMFLOAT3 Tangent;
		XMFLOAT3 Binormal;
	};

	struct VertexPositionNormalTangentColorTexture
	{
		XMFLOAT3 Position;
		XMFLOAT3 Normal;
		XMFLOAT4 Tangent;
		UINT     Color;
		XMFLOAT2 TexCoord;
	};
}