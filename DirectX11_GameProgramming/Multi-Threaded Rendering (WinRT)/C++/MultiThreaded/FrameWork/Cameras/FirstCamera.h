/*
	FOR GETTING MORE INFORMATION ABOUT THIS CODE PLEASE CHECK http://directx11-1-gameprogramming.azurewebsites.net/ 
	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
	ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
	PARTICULAR PURPOSE.
    Copyright (c) Microsoft Corporation. All rights reserved

	File Name        : FirstCamera.h
	Generated by     : Pooya Eimandar (http://Pooya-Eimandar.com/)
	File Description : The first person camera class
 */
#pragma once

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