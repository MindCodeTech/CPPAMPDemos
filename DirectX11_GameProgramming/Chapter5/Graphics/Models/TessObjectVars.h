#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : TessObjectVars.h
 * File Description : 
 */
#include <DirectXMath.h>
#include <WinBase.h>

struct TessObjectVars
{
	DirectX::XMMATRIX   WorldViewProjection;
	float				TessEdge;
	float				TessInside;
	DirectX::XMFLOAT2   Paddings;

	TessObjectVars()
	{
		ZeroMemory(this, sizeof(TessObjectVars));
		this->TessEdge = this->TessInside  = 3;
	}
};