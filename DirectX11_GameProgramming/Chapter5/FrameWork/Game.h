#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Game.h
 * File Description : The base class for the game
 */
#include <wrl.h>
#include <agile.h>
#include <d2d1_1.h>
#include <dwrite_1.h>
#include <d3d11_1.h>

using namespace Microsoft::WRL;
using namespace Platform;
using namespace Windows::UI::Core;

ref class Game abstract
{
public:
	virtual void Initialize(Windows::UI::Xaml::Controls::SwapChainBackgroundPanel^ swapChainPanel);
	virtual void WindowSizeChanged();
	virtual void Render() = 0;
	virtual void Present();

internal:
	//Constructor 
	Game(Windows::UI::Core::CoreWindow^ coreWindow);

protected private:
	//D3D Objects
	ComPtr<IDXGISwapChain1> swapChain;
	Windows::UI::Xaml::Controls::SwapChainBackgroundPanel^ swapChainPanel;
	ComPtr<ID3D11RenderTargetView> renderTargetView;
	ComPtr<ID3D11DepthStencilView> depthStencilView;
	Platform::String^ defaultAcceleratorName;

	D3D_FEATURE_LEVEL featureLevel;
	Windows::Foundation::Size renderTargetSize;
	Windows::Foundation::Rect windowBounds;
	Agile<CoreWindow> coreWindow;

private:
	void Load();
	void CreateDevice();
	void CreateWindowSize();
	void HandleDeviceLost();
	void LoadDefaultVertexShader();

};
