/*
	FOR GETTING MORE INFORMATION ABOUT THIS CODE PLEASE CHECK http://directx11-1-gameprogramming.azurewebsites.net/ 
	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
	ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
	PARTICULAR PURPOSE.
    Copyright (c) Microsoft Corporation. All rights reserved

	File Name        : FPS.cpp
	Generated by     : Pooya Eimandar (http://Pooya-Eimandar.com/)
	File Description : 
 */
#include "pch.h"
#include "Fps.h"

using namespace D2D1;
using namespace DX;

FPS::FPS() :_fps(0), elapsedTime(0)
{
	auto hr = GDevice.d2dContext->CreateSolidColorBrush(ColorF(ColorF::Red), &this->RedSolidBrushColor);
	ThrowIfFailed(hr);

	hr = GDevice.d2dContext->CreateSolidColorBrush(ColorF(ColorF::Yellow), &this->YellowSolidBrushColor);
	ThrowIfFailed(hr);

	hr = GDevice.d2dContext->CreateSolidColorBrush(ColorF(ColorF::Lime), &this->LimeSolidBrushColor);
	ThrowIfFailed(hr);
}

void FPS::Update(float ElapsedTime)
{
	this->elapsedTime += ElapsedTime;
	this->frames++;
	if (this->elapsedTime > 1.0f)
	{
		// Update Fps value and start next sampling period.
		this->_fps = (float)this->frames / this->elapsedTime;
		this->elapsedTime = 0;
		this->frames = 0;

#pragma region Set Fps Color

		if (this->_fps < 20.0f)
		{
			state = State::Error;
		}
		else if (this->_fps < 35.0f)
		{
			state = State::Warning;
		}
		else
		{
			state = State::Normal;
		}

#pragma endregion
	}
}

