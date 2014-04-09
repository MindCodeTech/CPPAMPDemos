#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Model.h
 * File Description : This class responsible for loading CMO file.
                      The code modified based on Visual Studio 3D Starter Kit "http://code.msdn.microsoft.com/Visual-Studio-3D-Starter-455a15f1"
 */
#define MaxLight 1

#include "Graphics/Shaders/Shader.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Shaders/CBuffer.h"

#define MaxTextures  8

struct Triangle
{
	XMFLOAT3 points[3];
};

struct BoundingSphere
{
	DirectX::XMFLOAT3 Center;
	float Radius;
	DirectX::XMFLOAT3 Min;
	DirectX::XMFLOAT3 Max;
	BoundingSphere() : Center(0,0,0), Radius(0), Min(0,0,0), Max(0,0,0) {}
};

struct SubMesh
{
	UINT MaterialIndex;
	UINT IndexBufferIndex;
	UINT VertexBufferIndex;
	UINT StartIndex;
	UINT PrimCount;
	SubMesh() : MaterialIndex(0), IndexBufferIndex(0), VertexBufferIndex(0), StartIndex(0), PrimCount(0) {}
};

struct DirLight
{
	DirectX::XMFLOAT4   Ambient;
	DirectX::XMFLOAT4   Diffuse;
	DirectX::XMFLOAT4   Specular;
	DirectX::XMFLOAT3   Direction;
	float				DPadding;

	DirLight()
	{
		ZeroMemory(this, sizeof(DirLight));
		Ambient = DirectX::XMFLOAT4(1.0f,1.0f,1.0f,1.0f);
	}
};

struct ObjectInfo
{
	DirectX::XMMATRIX   World;
	DirectX::XMMATRIX   WorldInvTranspose;
	DirectX::XMMATRIX   WorldViewProj;
	DirectX::XMMATRIX   ViewProjection;

	ObjectInfo()
	{
		ZeroMemory(this, sizeof(ObjectInfo));
	}
};

struct MaterialInfo
{
	DirectX::XMFLOAT4   Ambient;
	DirectX::XMFLOAT4   Diffuse;
	DirectX::XMFLOAT4   Specular;
	DirectX::XMFLOAT4   Emissive;
	float               SpecularPower;
	DirectX::XMFLOAT3   MPaddings;

	MaterialInfo()
	{
		using namespace DirectX;

		Ambient = XMFLOAT4(0.0f,0.0f,0.0f,1.0f);
		Diffuse = XMFLOAT4(1.0f,1.0f,1.0f,1.0f);
		Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		Emissive = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		SpecularPower = 120;
	}
};

struct LightInfo
{
	DirLight dirLight;
	DirectX::XMFLOAT3 eyePos;
	float  LPadding;

	LightInfo()
	{
		using namespace DirectX;
		ZeroMemory(this, sizeof(LightInfo));

		dirLight.Ambient = XMFLOAT4(0.6f,0.6f,0.6f,1.0f);
		dirLight.Diffuse = XMFLOAT4(0.7f,0.7f,0.7f,1.0f);
		dirLight.Specular = XMFLOAT4(0.7f,0.7f,0.7f,0.0f);
		dirLight.Direction = XMFLOAT3(1.0f, 1.0f, 1.0f);
	}
}; 

struct TessInfo
{
	float MaxTessDistance;
	float MinTessDistance;
	float MinTessFactor;
	float MaxTessFactor;
	float Height;
	DirectX::XMFLOAT3   TPaddings;

	TessInfo()
	{
		this->Height = 5.0f;
		this->MinTessDistance = 1.0f;
		this->MaxTessDistance = 30.0f;
		this->MinTessFactor = 5.0f;
		this->MaxTessFactor = 16.0f;
	}
};

struct Material
{
	std::vector<Shader^> shaders;
	std::vector<Texture2D^> textures;
	MaterialInfo Const;
	DirectX::XMFLOAT4X4 UVTransform;
	Material()
	{
		ZeroMemory(this, sizeof(Material));
	}
};

ref class Model
{
internal:
	Model();
	void Load(FILE* f);
	void Load(FILE* f, std::vector<Shader^> Shaders);
	void Update(float time);
	void Render();
	void Unload();

	DirectX::XMFLOAT3 Rotation;
	property DirectX::XMFLOAT3 Position
	{
		DirectX::XMFLOAT3 get()
		{
			return this->boundingSphere.Center;
		}
		void set(DirectX::XMFLOAT3 val)
		{
			this->boundingSphere.Center = val;
		}
	}
	property DirectX::XMMATRIX World
	{
		DirectX::XMMATRIX get()
		{
			return this->world;
		}
	}
	property float MinTessDistance
	{
		float get()
		{
			return this->CTessInfo.Const.MinTessDistance;
		}
		void set(float val)
		{
			if (this->CTessInfo.Const.MinTessDistance != val)
			{
				this->CTessInfo.Const.MinTessDistance = val;
			}
		}
	}
	property float MaxTessDistance
	{
		float get()
		{
			return this->CTessInfo.Const.MaxTessDistance;
		}
		void set(float val)
		{
			if (this->CTessInfo.Const.MaxTessDistance != val)
			{
				this->CTessInfo.Const.MaxTessDistance = val;
			}
		}
	}
	property float MinTessFactor
	{
		float get()
		{
			return this->CTessInfo.Const.MinTessFactor;
		}
		void set(float val)
		{
			if (this->CTessInfo.Const.MinTessFactor != val)
			{
				this->CTessInfo.Const.MinTessFactor = val;
			}
		}
	}
	property float MaxTessFactor
	{
		float get()
		{
			return this->CTessInfo.Const.MaxTessFactor;
		}
		void set(float val)
		{
			if (this->CTessInfo.Const.MaxTessFactor != val)
			{
				this->CTessInfo.Const.MaxTessFactor = val;
			}
		}
	}
	property float Height
	{
		float get()
		{
			return this->CTessInfo.Const.Height;
		}
		void set(float val)
		{
			if (this->CTessInfo.Const.Height != val)
			{
				this->CTessInfo.Const.Height = val;
			}
		}
	}
public:
	property int Triangles
	{
		int get()
		{
			return this->triangles.size();
		}
	}
private:
	bool loadingComplete;
	Platform::String^ Name;
	DirectX::XMMATRIX world;
	std::vector<SubMesh> subMeshes;
	std::vector<Material> materials;
	std::vector<ID3D11Buffer*> vertexBuffers;
	std::vector<ID3D11Buffer*> indexBuffers;
	std::vector<Triangle> triangles;
	BoundingSphere boundingSphere;
	CBuffer<ObjectInfo> CObjectInfo;
	CBuffer<MaterialInfo> CMaterialInfo;
	CBuffer<LightInfo> CLightInfo;
	CBuffer<TessInfo> CTessInfo;
	UINT stride;
};
