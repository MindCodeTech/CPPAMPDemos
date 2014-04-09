#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : SimpleScene.h
 * File Description : The main scene
 */
#include "FrameWork/Game.h"
#include "FrameWork/Timer.h"
#include "FrameWork/System/Perf.h"
#include "Graphics/SpriteBatch.h"
#include "Graphics/Shaders/Shader.h"
#include "Graphics/Models/Quad.h"

ref class SimpleScene sealed : public Game
{
public:
	SimpleScene(Windows::UI::Core::CoreWindow^ coreWindow);
	void Load();
	void Unload();
	void Update(float TotalTime, float DeltaTime);
	virtual void Render() override;

internal:
	Quad^ quad;

private:
	Shader^ shader;
	Timer^ timer;
	enum State
	{
		NotStarted,
		LoadSync,
		LoadAsync,
		LoadTaskGroup,
		Finished,
	} state;

	bool loadingComplete;
	float BackColor[4];
	SpriteBatch^ spriteBatch;
	SpriteFont^ spriteFont;
	Perf^ perf;
}; 