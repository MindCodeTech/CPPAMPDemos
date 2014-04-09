/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : FirstCamera.cpp
 * File Description : 
 */
#include "pch.h"
#include "FirstCamera.h"
#include "FrameWork\Input\InputManager.h"

using namespace DirectX;
using namespace Windows::System;

const XMFLOAT3 InitialiPosition = XMFLOAT3(0, 7, 50);
const XMFLOAT2 InitialAngle = XMFLOAT2(-0.1f, 0.0f);
const float debounce = 2.0f;

FirstCamera::FirstCamera() : angle(InitialAngle)
{
	this->position = InitialiPosition;
	BaseCamera::UpdateView();
}

void FirstCamera::UpdateView()
{
	auto forward = XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f);
	auto _lookAt = XMLoadFloat3(&position) + XMVector3Transform(forward, this->rotationMatrix);

	this->lookAt.x = XMVectorGetX(_lookAt);
	this->lookAt.y = XMVectorGetY(_lookAt);
	this->lookAt.z = XMVectorGetZ(_lookAt);

	BaseCamera::UpdateView();
}

void FirstCamera::UpdateWorld(XMFLOAT3 MoveVector)
{
	auto vect = XMVector3Transform(XMLoadFloat3(&MoveVector), this->rotationMatrix);

	this->position.x += XMVectorGetX(vect);
	this->position.y += XMVectorGetY(vect);
	this->position.z += XMVectorGetZ(vect);
}

void FirstCamera::Reset()
{
	this->position = InitialiPosition;
	this->angle = InitialAngle;
	UpdateView();
}

void FirstCamera::Update(float time)
{
	this->rotationMatrix = XMMatrixTranspose(XMMatrixRotationX(this->angle.x) * XMMatrixRotationY(this->angle.y));
	ProcessInput(time);
	UpdateView();
}

void FirstCamera::ProcessInput(float time)
{
	bool ForceUpdate = false;
	this->lastPointerPos = this->currentPointerPos;
	this->currentPointerPos = InputManager::pointerState.Position;

	auto moveVect = XMFLOAT3(0, 0, 0);	

#pragma region Change camera rotation

	if (InputManager::pointerState.IsRightButtonPressed())
	{
		if (currentPointerPos.y - lastPointerPos.y > debounce)
		{
			this->angle.x += time * CameraRotationSpeed;
		}
		else if (currentPointerPos.y - lastPointerPos.y < -debounce)
		{
			this->angle.x -= time * CameraRotationSpeed;
		}
		else if (currentPointerPos.x - lastPointerPos.x > debounce)
		{
			this->angle.y += time * CameraRotationSpeed;
		}
		else if (currentPointerPos.x - lastPointerPos.x < -debounce)
		{
			this->angle.y -= time * CameraRotationSpeed;
		}

		if (this->angle.x > 1.4)
		{
			this->angle.x = 1.4f;
		}
		else if (this->angle.x < -1.4)
		{
			this->angle.x = -1.4f;
		}

		if (this->angle.y > XM_PI)
		{
			this->angle.y -= 2 * XM_PI;
		}
		else if (this->angle.y < -XM_PI)
		{
			this->angle.y += 2 * XM_PI;
		}
		ForceUpdate = true;
	}

#pragma endregion

#pragma region Change camera movements

	if (InputManager::keyboardState.IsKeyDown(VirtualKey::D))
	{
		moveVect.x += time * CameraMovementSpeed;
		ForceUpdate = true;
	}
	else if (InputManager::keyboardState.IsKeyDown(VirtualKey::A))
	{
		moveVect.x -= time * CameraMovementSpeed;
		ForceUpdate = true;
	}
	else if (InputManager::keyboardState.IsKeyDown(VirtualKey::W))
	{
		moveVect.z -= time * CameraMovementSpeed;
		ForceUpdate = true;
	}
	else if (InputManager::keyboardState.IsKeyDown(VirtualKey::S))
	{
		moveVect.z += time * CameraMovementSpeed;
		ForceUpdate = true;
	}
	else if (InputManager::keyboardState.IsKeyDown(VirtualKey::Q))
	{
		moveVect.y += time * CameraMovementSpeed;
		ForceUpdate = true;
	}
	else if (InputManager::keyboardState.IsKeyDown(VirtualKey::Z))
	{
		moveVect.y -= time * CameraMovementSpeed;
		ForceUpdate = true;
	}

#pragma endregion

	if (ForceUpdate)
	{
		UpdateWorld(moveVect);
	}
}
