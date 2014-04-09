/*
	FOR GETTING MORE INFORMATION ABOUT THIS CODE PLEASE CHECK http://directx11-1-gameprogramming.azurewebsites.net/
	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
	ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
	PARTICULAR PURPOSE.
	Copyright (c) Microsoft Corporation. All rights reserved

	File Name        : Debugger.h
	Generated by     : Pooya Eimandar (http://Pooya-Eimandar.com/)
	File Description :
*/

#pragma once

#include "Graphics\SpriteBatch.h"
#include "Comps\FPS.h"
#include "Comps\CPU.h"

ref class Debugger
{
internal:
	Debugger(Graphics2D G2D);
	void ShowStatus(SpriteBatch^ spriteBatch, SpriteFont^ spriteFont, DirectX::XMFLOAT2* Position);
private:
	float totalTime, deltaTime;
	Fps^ _Fps;
	CPU^ _Cpu;
public:
	void Update(float TotalTime, float DeltaTime);
};

