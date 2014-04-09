#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Perf.h
 * File Description : 
 */
#include "Graphics\SpriteBatch.h"
#include "FPS.h"
#include "CpuInfo.h"

ref class Perf
{
internal:
	Perf();
	void ShowStatus(SpriteBatch^ spriteBatch, SpriteFont^ spriteFont, DirectX::XMFLOAT2* Position);
private:
	float totalTime, deltaTime;
	FPS^ _FPS;
	CpuInfo^ _CPU;
public:
	void Update(float TotalTime, float DeltaTime);
};