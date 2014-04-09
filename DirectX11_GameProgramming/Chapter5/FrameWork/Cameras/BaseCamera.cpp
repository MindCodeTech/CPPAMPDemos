/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : BaseCamera.cpp
 * File Description : 
 */
#include "pch.h"
#include "BaseCamera.h"
#include "FrameWork/MathHelper.h"

using namespace DirectX;
using namespace MathHelper;

BaseCamera::BaseCamera() : up(XMFLOAT3(0, 1, 0)) 
{
	this->lookAt = XMFLOAT3(0, 1, 0);
	UpdateProjection(70.0f, DX::GDevice.AspectRatio, 0.01f, 1000.0f);
}

void BaseCamera::UpdateView()
{
	XMVECTOR vPosition = XMLoadFloat3(&this->position);
	XMVECTOR vLook = XMLoadFloat3(&lookAt);
	XMVECTOR vUp = XMLoadFloat3(&this->up);

	this->view = XMMatrixLookAtRH(vPosition, vLook, vUp);
}

void BaseCamera::UpdateProjection(float fieldOfView, float aspectRatio, float nearPlane, float farPlane )
{
	float fovAngleY = fieldOfView * XM_PI / 180.0f;

	if (aspectRatio < 1.0f)
	{
		///
		/// portrait or snap view
		///
		this->up = XMFLOAT3(1.0f, 0.0f, 0.0f);
		fovAngleY = 120.0f * XM_PI / 180.0f;
	}
	else
	{
		///
		/// landscape view
		///
		this->up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	}

	this->projection = XMMatrixPerspectiveFovRH(fovAngleY, aspectRatio, nearPlane, farPlane);
}
