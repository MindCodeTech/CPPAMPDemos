/*
	FOR GETTING MORE INFORMATION ABOUT THIS CODE PLEASE CHECK http://directx11-1-gameprogramming.azurewebsites.net/
	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
	ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
	PARTICULAR PURPOSE.
	Copyright (c) Microsoft Corporation. All rights reserved

	File Name        : SimpleScene.h
	Generated by     : Pooya Eimandar (http://Pooya-Eimandar.com/)
	File Description : 
*/

#pragma once

#include "FrameWork\Game.h"
#include "FrameWork\System\Debugger.h"
#include "Graphics\SpriteBatch.h"
#include "Graphics\Shader.h"
#include "FrameWork\Cameras\ChaseCamera.h"
#include "Graphics\Manager.h"

#define  MAX_MODELS 50

ref class SimpleScene sealed : public Game
{
private:
	float Time;
	bool wireFrame;
	bool loadingComplete;
	SpriteBatch^ spriteBatch;
	Debugger^ debugger;
	SpriteFont^ spriteFont;
	ChaseCamera^ camera;
	Manager^ manager;
	ID3D11RasterizerState1* wireFrameState;
	ID3D11RasterizerState1* solidState;

internal:
	property bool WireFrame
	{
		bool get()
		{
			return this->wireFrame;
		}
		void set(bool val)
		{
			this->wireFrame = val;
		}
	}
public:
	SimpleScene(Windows::UI::Core::CoreWindow^ coreWindow);
	virtual void WindowSizeChanged() override;
	void Load();
	void SetDefaultAccelerator();
	void Unload();
	void Update(float TotalTime, float DeltaTime);
	virtual void Render() override;
}; 