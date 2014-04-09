#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : FirstCamera.h
 * File Description : The first person camera class
 */
#include "BaseCamera.h"

#define CameraMovementSpeed 100
#define CameraRotationSpeed 1

ref class FirstCamera sealed : public BaseCamera
{
internal:
	FirstCamera();

	void UpdateView();
	void UpdateWorld(DirectX::XMFLOAT3 MoveVector);
	void Reset();
	void ProcessInput(float time);

	property DirectX::XMFLOAT2 Angle
	{
		DirectX::XMFLOAT2 get()
		{
			return this->angle;
		}
		void set(DirectX::XMFLOAT2 val)
		{
			this->angle = val;
		}
	}

private:
	DirectX::XMFLOAT2 angle;
	DirectX::XMMATRIX rotationMatrix;
	DirectX::XMFLOAT2 lastPointerPos;
	DirectX::XMFLOAT2 currentPointerPos;

public:
	void Update(float time);
};