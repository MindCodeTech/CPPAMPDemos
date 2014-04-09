/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Model.cpp
 * File Description : 
 */
#include "pch.h"
#include "Model.h"
#include "Graphics/Textures/Texture2D.h"
#include "../Shaders/Shader.h"

using namespace Concurrency;
using namespace Platform;
using namespace DirectX;
using namespace DX;
using namespace std;

Model::Model() : loadingComplete(false), stride(sizeof(VertexTypes::VertexPositionNormalTangentColorTexture)), 
	world(XMMATRIX(
	1,0,0,0,
	0,1,0,0,
	0,0,1,0,
	0,0,0,1))
{
}

void Model::Load(FILE* f)
{
	std::vector<Shader^> NULLShaders;
	Load(f, NULLShaders);
}

void Model::Load(FILE* f, std::vector<Shader^> Shaders)
{
	using namespace std;
	
#pragma region Read name of mesh

	this->Name = L"";
	UINT Lenght = 0;
	const UINT UIntSize = sizeof(UINT);

	fread(&Lenght, UIntSize, 1, f);
	if (Lenght > 0)
	{
		vector<wchar_t> name(Lenght);
		fread(&name[0], sizeof(wchar_t), Lenght, f);
		for(auto e : name)
		{
			this->Name += e;
		}
	}

#pragma endregion

#pragma region Load materials

	//How many materials
	Lenght = 0;
	fread(&Lenght, UIntSize, 1, f);
	this->materials.resize(Lenght);

	//Load each material
	for (UINT i = 0; i < Lenght; i++)
	{
		// read material name
		Lenght = 0;
		fread(&Lenght, UIntSize, 1, f);
		if (Lenght > 0)
		{
			vector<wchar_t> matName(Lenght);
			fread(&matName[0], sizeof(wchar_t), Lenght, f);
		}

		// read material's elements
		fread(&this->materials[i].Const.Ambient, sizeof(XMFLOAT4), 1, f);//Ambient
		fread(&this->materials[i].Const.Diffuse, sizeof(XMFLOAT4), 1, f);//Diffuse
		fread(&this->materials[i].Const.Specular, sizeof(XMFLOAT4), 1, f);//Specular
		fread(&this->materials[i].Const.Specular.w, sizeof(float), 1, f);//SpecularPower
		fread(&this->materials[i].Const.Emissive, sizeof(XMFLOAT4), 1, f);//Emissive
		fread(&this->materials[i].UVTransform, sizeof(XMFLOAT4X4), 1, f);//UVTransform

		bool useDefaultShader = false;
		Shader^ DefaultShader;
		if (Shaders.size() == 0)
		{
			useDefaultShader = true;

			DefaultShader = ref new Shader();
			//Set default input layout
			DefaultShader->SetInputLayout(GDevice.defaultLayout.Get());
		}

		// read name of the pixel shader
		Lenght = 0;
		fread(&Lenght, UIntSize, 1, f);
		if (Lenght > 0)
		{
			// Read the pixel shader name
			String^ PSPath = "";
			vector<wchar_t> pixelShaderName(Lenght);
			fread(&pixelShaderName[0], sizeof(wchar_t), Lenght, f);
			for(auto e : pixelShaderName)
			{
				PSPath += e;
			}
			//Load pixel shader
			if (!PSPath->IsEmpty() && useDefaultShader)
			{
				auto iter = GDevice.PixelShaders.find(PSPath);
				if (iter != GDevice.PixelShaders.end())
				{
					DefaultShader->SetPixelShader(iter->second);
				}
				else
				{
					auto psPath = wstring(PSPath->Data());
					auto index = psPath.find_last_of('_');
					if (index != std::wstring::npos)
					{
						psPath = psPath.substr(index + 1);
						psPath = psPath.substr(0, psPath.size() - 9);//9 for .dgsl.cso
						psPath.append(L".cso");
					}
					PSPath = ref new String(psPath.c_str());
					Shader::LoadShader(PSPath, ShaderType::PixelShader, VertexDeclaration::NOP, DefaultShader);
					GDevice.PixelShaders[PSPath] = DefaultShader->PixelShader;
				}
			}
		}
		if (useDefaultShader)
		{
			DefaultShader->SetVertexShader(GDevice.defaultVS.Get());
			this->materials[i].shaders.push_back(DefaultShader);
		}
		else
		{
			for (int iter =0 ; iter< Shaders.size(); ++iter)
			{
				this->materials[i].shaders.push_back(Shaders.at(iter));
			}
			Shaders.clear();
		}

		// load textures...
		for (int t = 0; t < MaxTextures; t++)
		{
			// read name of texture
			Lenght = 0;
			fread(&Lenght, UIntSize, 1, f);
			if (Lenght > 0)
			{
				// read the texture name
				std::vector<wchar_t> textureFilename(Lenght);
				fread(&textureFilename[0], sizeof(wchar_t), Lenght, f);
				//if texture is not empty
				if(textureFilename[0] != '\0')
				{
					String^ TPath = "";
					for(auto e : textureFilename)
					{
						TPath += e;
					}
					auto tPath = wstring(TPath->Data());
					auto index = tPath.find_last_of('_');
					if (index != std::wstring::npos)
					{
						tPath = tPath.substr(index + 1);
						tPath = tPath.substr(0, tPath.size() - 8);//8 for .dds.dds
						tPath.append(L".dds");
					}

					TPath = ref new String(tPath.c_str());
					auto texture = ref new Texture2D();
					Texture2D::LoadTexture(TPath, texture);
					materials[i].textures.push_back(texture);
				}
			}
		}
	}

#pragma endregion

	// Does this object contains the skeletal animation?
	BYTE isSkeletalDataPresent = FALSE;
	fread(&isSkeletalDataPresent, sizeof(BYTE), 1, f);

	// Read sub mesh info
	Lenght = 0;
	fread(&Lenght, UIntSize, 1, f);
	this->subMeshes.resize(Lenght);
	for (UINT i = 0; i < Lenght; i++)
	{
		fread(&(this->subMeshes[i]), sizeof(SubMesh), 1, f);
	}

#pragma region Read index buffers

	Lenght = 0;
	fread(&Lenght, UIntSize, 1, f);

	this->indexBuffers.resize(Lenght);
	vector<vector<USHORT>> iBuffers(Lenght);
	for (UINT i = 0; i < Lenght; i++)
	{
		UINT ibCount = 0;
		fread (&ibCount, UIntSize, 1, f);
		if (ibCount > 0)
		{
			iBuffers[i].resize(ibCount);

			// read in the index data
			fread(&iBuffers[i][0], sizeof(USHORT), ibCount, f);

#pragma region Create an index buffer

			D3D11_BUFFER_DESC indexBufferDesc;
			ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

			indexBufferDesc.ByteWidth = sizeof(USHORT) * ibCount;
			indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			indexBufferDesc.CPUAccessFlags = 0;
			indexBufferDesc.MiscFlags = 0;
			indexBufferDesc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA indexBufferData;
			ZeroMemory(&indexBufferData, sizeof(indexBufferData));
			indexBufferData.pSysMem = &iBuffers[i][0];
			indexBufferData.SysMemPitch = 0;
			indexBufferData.SysMemSlicePitch = 0;

			auto hr = GDevice.d3dDevice->CreateBuffer(&indexBufferDesc, &indexBufferData, &this->indexBuffers[i]);
			ThrowIfFailed(hr);

#pragma endregion

		}
	}

#pragma endregion

#pragma region Read vertex buffers

	Lenght = 0;
	fread(&Lenght, UIntSize, 1, f);
	this->vertexBuffers.resize(Lenght);

	vector<vector<VertexTypes::VertexPositionNormalTangentColorTexture>> vBuffers(Lenght);
	for (UINT i = 0; i < Lenght; i++)
	{
		UINT vbCount = 0;
		fread (&vbCount, UIntSize, 1, f);
		if (vbCount > 0)
		{
			vBuffers[i].resize(vbCount);

			// read in the vertex data
			fread(&vBuffers[i][0], sizeof(VertexTypes::VertexPositionNormalTangentColorTexture), vbCount, f);

#pragma region Create a vertex buffer for this data

			D3D11_BUFFER_DESC vertexBufferDesc;
			ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
			vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			vertexBufferDesc.ByteWidth = sizeof(VertexTypes::VertexPositionNormalTangentColorTexture) * vbCount;
			vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vertexBufferDesc.CPUAccessFlags = 0;
			vertexBufferDesc.MiscFlags = 0;
			vertexBufferDesc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA vertexBufferData;
			ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
			vertexBufferData.pSysMem = &vBuffers[i][0];
			vertexBufferData.SysMemPitch = 0;
			vertexBufferData.SysMemSlicePitch = 0;

			auto hr = GDevice.d3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &this->vertexBuffers[i]);
			ThrowIfFailed(hr);

#pragma endregion
		}
	}

#pragma endregion

#pragma region Update triangles

	Lenght = this->subMeshes.size();
	for (UINT i = 0; i < Lenght; i++)
	{
		auto subMesh = this->subMeshes[i];
		std::vector<USHORT>& ib = iBuffers[subMesh.IndexBufferIndex];
		std::vector<VertexTypes::VertexPositionNormalTangentColorTexture>& vb = vBuffers[subMesh.VertexBufferIndex];

		for (UINT j = 0; j < ib.size(); j += 3)
		{
			VertexTypes::VertexPositionNormalTangentColorTexture& v0 = vb[ib[j]];
			VertexTypes::VertexPositionNormalTangentColorTexture& v1 = vb[ib[j+1]];
			VertexTypes::VertexPositionNormalTangentColorTexture& v2 = vb[ib[j+2]];

			Triangle tri;
			tri.points[0] = v0.Position;
			tri.points[1] = v1.Position;
			tri.points[2] = v2.Position;

			this->triangles.push_back(tri);
		}
	}

#pragma endregion

	// done with temp buffers
	vBuffers.clear();
	iBuffers.clear();

	// We skipped Reading skinning vertex buffers
	Lenght = 0;
	fread(&Lenght, UIntSize, 1, f);

	// Read bounding sphere
	fread(&this->boundingSphere, sizeof(BoundingSphere), 1, f);

	//No more to read, start performing tasks
	fclose(f);
	f = nullptr;
	
	this->loadingComplete = true;
	this->CObjectInfo.Load();	
	this->CMaterialInfo.Load();
	this->CLightInfo.Load();
	this->CTessInfo.Load();
}

void Model::Update(float time)
{
	/*this->Rotation.x += time;
	this->Rotation.y += time;
	this->Rotation.z += time;
	*/
	//Update world
	this->world = 
		XMMatrixScaling(1, 1, 1) * 
		XMMatrixRotationX(this->Rotation.x) * 
		XMMatrixRotationY(this->Rotation.y) *
		XMMatrixRotationZ(this->Rotation.z) *
		XMMatrixTranslation(this->Position.x, this->Position.y, this->Position.z);
}

void Model::Render()
{
	if (!this->loadingComplete) return;

	auto view = Camera->View;
	auto projection = Camera->Projection;

	int ShaderIndex; 
	if (DX::UseDispMap)
	{
		GDevice.d3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		ShaderIndex = 1;
	}
	else
	{
		GDevice.d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ShaderIndex = 0;
	}

	for (UINT i = 0; i < this->subMeshes.size(); i++)
	{
		auto submesh = this->subMeshes[i];
		if (submesh.IndexBufferIndex < this->indexBuffers.size() && submesh.VertexBufferIndex < this->vertexBuffers.size())
		{
			UINT offset = 0;
			GDevice.d3dContext->IASetVertexBuffers(0, 1, &this->vertexBuffers[submesh.VertexBufferIndex], &stride, &offset);
			GDevice.d3dContext->IASetIndexBuffer(this->indexBuffers[submesh.IndexBufferIndex], DXGI_FORMAT_R16_UINT, 0);
		}	

		auto _shader = this->materials[submesh.MaterialIndex].shaders.at(ShaderIndex) ;

		#pragma region Update Material
		
		memcpy(&CMaterialInfo.Const.Ambient, &this->materials[submesh.MaterialIndex].Const.Ambient, sizeof(this->materials[submesh.MaterialIndex].Const.Ambient));
		memcpy(&CMaterialInfo.Const.Diffuse, &this->materials[submesh.MaterialIndex].Const.Diffuse, sizeof(this->materials[submesh.MaterialIndex].Const.Diffuse));
		memcpy(&CMaterialInfo.Const.Specular, &this->materials[submesh.MaterialIndex].Const.Specular, sizeof(this->materials[submesh.MaterialIndex].Const.Specular));
		memcpy(&CMaterialInfo.Const.Emissive, &this->materials[submesh.MaterialIndex].Const.Emissive, sizeof(this->materials[submesh.MaterialIndex].Const.Emissive));
		CMaterialInfo.Update();
		
		#pragma endregion

		#pragma region Update Transform
		
		DirectX::XMMATRIX viewProjection = view * projection;

		CObjectInfo.Const.WorldInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, world));
		CObjectInfo.Const.World = XMMatrixTranspose(this->world);
		CObjectInfo.Const.WorldViewProj = XMMatrixTranspose(world * viewProjection);
		CObjectInfo.Const.ViewProjection = XMMatrixTranspose(viewProjection);
		CObjectInfo.Update();

		#pragma endregion

		#pragma region Update Light
		
		CLightInfo.Const.eyePos = Camera->Position;
		CLightInfo.Update();

		#pragma endregion
		
		#pragma region Set Constant Buffers
		
		_shader->SetConstantBuffer(0, 1, CObjectInfo.Buffer);
		_shader->SetConstantBuffer(1, 1, CMaterialInfo.Buffer);
		_shader->SetConstantBuffer(2, 1, CLightInfo.Buffer);

		//Update Tessellation
		if (DX::UseDispMap)
		{
			CTessInfo.Update();
			_shader->SetConstantBuffer(3, 1, CTessInfo.Buffer);
		}

		#pragma endregion
		
		//Set Textures
		_shader->SetTexture2D(0, 1, this->materials[submesh.MaterialIndex].textures.at(0));
		_shader->SetTexture2D(1, 1, this->materials[submesh.MaterialIndex].textures.at(1));

		GDevice.d3dContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
		_shader->Apply();
		{
			GDevice.d3dContext->DrawIndexed(submesh.PrimCount * 3, submesh.StartIndex, 0);
		}
	}
}

void Model::Unload()
{
	for (UINT i = 0; i < this->materials.size(); i++)
	{
		this->materials[i].shaders.clear();
		this->materials[i].textures.clear();
	}
}
