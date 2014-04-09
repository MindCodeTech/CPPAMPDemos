#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : ModelManager.h
 * File Description : This class responsible for managing all models
 */
#include "Model.h"
#include <vector>

ref class ModelManager
{
internal:
	std::vector<Model^> models;

	ModelManager();
	void LoadModel(Platform::String^ path);
	void LoadModel(Platform::String^ path, std::vector<Shader^> Shaders);
	void UpdateActiveModel(DirectX::XMFLOAT3 transfome, float Yaw );
	void UpdateModels( float time );
	void RenderModels();
	void Unload();	

	property UINT ActiveIndex
	{
		UINT get()
		{
			return this->activeIndex;
		}
		void set(UINT val)
		{
			this->activeIndex = val;
		}
	};
	property DirectX::XMFLOAT3 ActivePositionModel
	{
		DirectX::XMFLOAT3 get()
		{
			if(this->models.size() > 0)
			{
				return this->models[activeIndex]->Position;
			}
			return DirectX::XMFLOAT3();
		}
	};
	property float ActiveYawModel
	{
		float get()
		{
			if(this->models.size() > 0)
			{
				return this->models[activeIndex]->Rotation.y;
			}
			return 0;
		}
	};
	property UINT ActiveTotalTriangles
	{
		UINT get()
		{
			if(this->models.size() > 0)
			{
				return this->models[this->activeIndex]->Triangles;
			}
			return 0;
		}
	};

private:
	UINT activeIndex;
	void RenderModel(Model^ model);
};

