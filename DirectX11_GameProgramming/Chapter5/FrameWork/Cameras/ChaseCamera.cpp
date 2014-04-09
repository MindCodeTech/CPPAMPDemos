/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : ChaseCamera.cpp
 * File Description : 
 */
#include "pch.h"
#include "ChaseCamera.h"

using namespace DirectX;
using namespace MathHelper;

ChaseCamera::ChaseCamera() : stiffness(4000.0f), damping(800.0f), mass(50.0f),
	lookAtOffset(XMFLOAT3(1.0f, 3.5f, -1.0f)), velocity(XMFLOAT3())
{
	this->position = XMFLOAT3(0,0,0);
	this->desiredPositionOffset = XMFLOAT3(LEFTRIGHT, UPDOWN, ZINDEX);
	this->HeadingVec = this->lookAt - this->position;
	this->HeadingVec = Normalize(this->HeadingVec);
	this->UpVec = XMFLOAT3(0, 1, 0);
	this->RightVec = Cross(this->HeadingVec, this->UpVec);

	UpdateView();
}

void ChaseCamera::Update(XMFLOAT3 BindPos, float Yaw)
{
	const float Step = 0.016f;

	this->yaw = Yaw;
	
	auto rotationMatrix = XMMatrixIdentity() * XMMatrixRotationRollPitchYaw(this->pitch, this->yaw, 0);
	// Updates the camera position relative to the model Matrix
	this->desiredPosition = BindPos + TransformNormal(desiredPositionOffset, rotationMatrix);
	
	auto stretch = this->position - this->desiredPosition;
	auto force = ( -this->stiffness * stretch ) - ( this->damping * this->velocity );
	auto acceleration = force / this->mass;
	this->velocity = this->velocity + acceleration * Step;
	this->position = this->position + this->velocity * Step;

	this->lookAt = this->position + XMMatrixForward(rotationMatrix);
	
	BaseCamera::UpdateView();
}

void ChaseCamera::Reset()
{
	this->desiredPositionOffset = XMFLOAT3(LEFTRIGHT, UPDOWN, ZINDEX);
	BaseCamera::UpdateView();
}