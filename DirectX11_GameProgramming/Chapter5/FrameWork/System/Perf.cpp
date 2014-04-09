/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Perf.cpp
 * File Description : Display the performance of application
 */
#include "pch.h"
#include "Perf.h"

Perf::Perf()
{
	this->_FPS = ref new FPS();
	this->_CPU = ref new CpuInfo();
}

void Perf::Update(float TotalTime, float DeltaTime)
{
	this->totalTime = TotalTime;
	this->deltaTime = DeltaTime;

	this->_FPS->Update(DeltaTime);
}

void Perf::ShowStatus(SpriteBatch^ spriteBatch, SpriteFont^ spriteFont, DirectX::XMFLOAT2* Position)
{
	const float YPlus = 20;

	spriteBatch->ShowString("FPS : " + this->_FPS->fps.ToString(), Position, this->_FPS->FpsColor, spriteFont, Matrix3x2F::Identity());
	Position->y += YPlus;

	spriteBatch->ShowString("Elapsed Time : " + this->deltaTime.ToString(), Position, spriteFont);
	Position->y += YPlus;

	spriteBatch->ShowString("Total Time : " + this->totalTime.ToString(), Position, spriteFont);
	Position->y += YPlus;
	
	spriteBatch->ShowString("CPU Core(s) : " + this->_CPU->TotalCores.ToString(), Position, spriteFont);
	Position->y += YPlus;
}