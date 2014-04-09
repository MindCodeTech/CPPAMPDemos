/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : ModelManager.cpp
 * File Description : 
 */
#include "pch.h"
#include "ModelManager.h"
#include "Framework/MathHelper.h"

using namespace Concurrency;
using namespace Platform;
using namespace DirectX;
using namespace MathHelper;

ModelManager::ModelManager() : activeIndex(0)
{
}

void ModelManager::LoadModel(String^ path)
{
	std::vector<Shader^> NULLShaders;
	LoadModel(path, NULLShaders);
}

void ModelManager::LoadModel(Platform::String^ path, std::vector<Shader^> Shaders)
{
	if(path->IsEmpty()) return;

	FILE* f = nullptr;
	_wfopen_s(&f, path->Data(), L"rb"); 
	if (f == nullptr)
	{
		throw ref new Exception(0, "Could not load model on following path : " + path );
	}
	else
	{
		// How many models?
		UINT Lenght = 0;
		fread(&Lenght, sizeof(Lenght), 1, f);
		//Load each mesh
		for (UINT i = 0; i < Lenght; i++)
		{
			auto model = ref new Model();
			model->Load(f, Shaders);
			this->models.push_back(model);
		}
	}
	if (f != nullptr)
	{
		fclose(f);
	}
}

void ModelManager::UpdateActiveModel(XMFLOAT3 transfome, float Yaw )
{
	if(this->models.size() > 0)
	{
		this->models[activeIndex]->Position = this->models[activeIndex]->Position + transfome;
		this->models[activeIndex]->Rotation.y = Yaw;
	}
}

void ModelManager::UpdateModels( float time )
{
	std::for_each(this->models.begin(), this->models.end(), [time](Model^ m)
	{
		m->Update(time);		
	});
}

void ModelManager::RenderModels()
{
	std::for_each(this->models.begin(), this->models.end(), [](Model^ m)
	{
		m->Render();		
	});
}

void ModelManager::Unload()
{
	std::for_each(this->models.begin(), this->models.end(), [](Model^ m)
	{
		m->Unload();		
	});
}