#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : ChaseCamera.h
 * File Description : The third person camera class
 */
#include "BaseCamera.h"
#include "FrameWork/MathHelper.h"

#define RUp       10.0f
#define RDown     -4.0f
#define LEFTRIGHT 7.0f
#define UPDOWN    15.0f
#define ZINDEX    40.0f

ref class ChaseCamera : public BaseCamera
{
private:

	float yaw;
	float pitch;
	float stiffness;
	float damping;
	float mass;
	DirectX::XMFLOAT3 velocity;
	DirectX::XMFLOAT3 desiredPosition;
	DirectX::XMFLOAT3 lookAtOffset;
	DirectX::XMFLOAT3 UpVec;
	DirectX::XMFLOAT3 RightVec;
	DirectX::XMFLOAT3 HeadingVec;
	DirectX::XMFLOAT3 desiredPositionOffset;

internal:
	ChaseCamera();
	
	property float Yaw
	{
		float get()
		{
			return this->yaw;
		}
		void set(float val)
		{
			this->yaw = val;
		}
	}
	property float Pitch
	{
		float get()
		{
			return this->pitch;
		}
		void set(float val)
		{
			this->pitch = MathHelper::Restrict(val, RDown, RUp);
		}
	}
	property float LeftRight
	{
		float get()
		{
			return this->desiredPositionOffset.x;
		}
		void set(float val)
		{
			this->desiredPositionOffset.x = val;
		}
	}
	property float UpDown
	{
		float get()
		{
			return this->desiredPositionOffset.y;
		}
		void set(float val)
		{
			this->desiredPositionOffset.y = val;
		}
	}
	property float ZIndex
	{
		float get()
		{
			return this->desiredPositionOffset.z;
		}
		void set(float val)
		{
			this->desiredPositionOffset.z = val;
		}
	}

	void Update(DirectX::XMFLOAT3 BindPos, float Yaw);
	void Reset();
};


