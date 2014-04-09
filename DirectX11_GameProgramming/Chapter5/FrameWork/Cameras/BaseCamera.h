#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : BaseCamera.h
 * File Description : The base class for types of camera
 */
#include <DirectXMath.h>

ref class BaseCamera
{
internal:
	BaseCamera();
	void UpdateView();
	void UpdateProjection(float fieldOfView, float aspectRatio, float nearPlane, float farPlane );

	property DirectX::XMMATRIX View
	{
		DirectX::XMMATRIX get()
		{
			return this->view;
		}
	}
	property DirectX::XMMATRIX Projection
	{
		DirectX::XMMATRIX get()
		{
			return this->projection;
		}
	}
	property DirectX::XMFLOAT3 Position
	{
		DirectX::XMFLOAT3 get()
		{
			return this->position;
		}
		void set(DirectX::XMFLOAT3 val)
		{
			this->position = val;
		}
	}

protected private:
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 lookAt;
	DirectX::XMFLOAT3 direction;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
};
