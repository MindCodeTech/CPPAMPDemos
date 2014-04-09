/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : SimpleScene.cpp
 * File Description : 
 */
#include "pch.h"
#include "SimpleScene.h"

using namespace DX;
using namespace DirectX;
using namespace Concurrency;
using namespace Windows::System;

SimpleScene::SimpleScene(Windows::UI::Core::CoreWindow^ coreWindow) : Game(coreWindow)
{
	this->loadingComplete = false;
	this->BackColor[0] = 0.0f;
	this->BackColor[1] = 0.0f;
	this->BackColor[2] = 0.0f;
	this->BackColor[3] = 1.0f;
}

void SimpleScene::Load()
{
	using namespace concurrency;

	Camera = ref new FirstCamera();
	Camera->Position = XMFLOAT3(0, -0.2f, 1.5f);
	Camera->Update(0);

	this->spriteBatch = ref new SpriteBatch();
	this->spriteFont = ref new SpriteFont();
	this->spriteFont->Load();
	this->perf = ref new Perf();
	this->timer = ref new Timer();
	this->shader = ref new Shader();
	this->quad = ref new Quad();
	this->quad->Load();
	
	this->loadingComplete = true;
}

void SimpleScene::Unload()
{
}

void SimpleScene::Update(float TotalTime, float DeltaTime)
{
	this->perf->Update(TotalTime, DeltaTime);
}

void SimpleScene::Render()
{
	auto context = GDevice.d3dContext.Get();

	context->OMSetRenderTargets(1, this->renderTargetView.GetAddressOf(), this->depthStencilView.Get() );
	context->ClearRenderTargetView(this->renderTargetView.Get(), this->BackColor);
	context->ClearDepthStencilView(this->depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0 );

	this->quad->Render();

	auto Position = XMFLOAT2(5, 0);
	this->spriteBatch->Begin();
	{
		this->spriteBatch->ShowRectangle();
		this->perf->ShowStatus(this->spriteBatch, this->spriteFont, &Position);
		this->spriteBatch->ShowString(L"C++AMP default accelerator : " + this->defaultAcceleratorName, &Position, this->spriteFont);
		this->spriteBatch->End();
	}
}

