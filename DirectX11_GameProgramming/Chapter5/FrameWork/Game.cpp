/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Game.cpp
 * File Description : 
 */
#include "pch.h"
#include "Game.h"
#include "Graphics/Shaders/Shader.h"
#include "Graphics/Shaders/DefaultVertexShader.inc"

using namespace DX;

Game::Game(Windows::UI::Core::CoreWindow^ coreWindow)
{
	this->coreWindow = coreWindow;
}

void Game::Initialize(Windows::UI::Xaml::Controls::SwapChainBackgroundPanel^ swapChainPanel)
{
	this->swapChainPanel = swapChainPanel;
	Load();
}

void Game::Load()
{
#pragma region Initialize & Load

	CreateDevice();
	CreateWindowSize();

#pragma endregion
}

void Game::HandleDeviceLost()
{
#pragma region OnLosingDevice

	// Reset these member variables
	windowBounds.Width = 0;
	windowBounds.Height = 0;
	swapChain = nullptr;
	CreateDevice();
	WindowSizeChanged();

#pragma endregion
}

void Game::CreateDevice()
{
#pragma region Create _3D Device and Context

	auto hr =  S_FALSE;
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
	// For debugging
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
	//creationFlags |= D3D11_CREATE_DEVICE_DEBUGGABLE;
#endif

	D3D_FEATURE_LEVEL featureLevels[] = 
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;

	hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		creationFlags, 
		featureLevels, 
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION, 
		&device, 
		&featureLevel,
		&context);	
	ThrowIfFailed(hr);

	// Get 3D Device
	hr = device.As(&GDevice.d3dDevice);
	ThrowIfFailed(hr);
	
	//Get 3D context
	hr = context.As(&GDevice.d3dContext);
	ThrowIfFailed(hr);

#pragma endregion

#pragma region Create Amp Accelerator

	accViewObj = concurrency::direct3d::create_accelerator_view(reinterpret_cast<IUnknown *>(device.Get()));
	for (int i = 0; i < accViewObj.accelerator.description.length(); i++)
	{
		this->defaultAcceleratorName +=	accViewObj.accelerator.description[i];
	}

#pragma endregion


#pragma region Create _2D Device and Context

	D2D1_FACTORY_OPTIONS factory2DOptions;
	ZeroMemory(&factory2DOptions, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
	factory2DOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	//Create Direct2D factory
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &factory2DOptions, &GDevice.factory);
	ThrowIfFailed(hr);

	//Create DirectWrite factory
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &GDevice.writeFactory );
	ThrowIfFailed(hr);

	//Create image factory
	hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&GDevice.imageFactory));
	ThrowIfFailed(hr);

	//Get IDXGIDevice
	ComPtr<IDXGIDevice> dxgiDevice;
	hr = GDevice.d3dDevice.As(&dxgiDevice);
	ThrowIfFailed(hr);

	//Create 2D Device from 2D factory
	hr = GDevice.factory->CreateDevice(dxgiDevice.Get(), &GDevice.d2dDevice);
	ThrowIfFailed(hr);

	//Create 2D context from 2D device
	hr = GDevice.d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &GDevice.d2dContext);
	ThrowIfFailed(hr);

#pragma endregion

	//Load default sampler
	const D3D11_TEXTURE_ADDRESS_MODE AddressUVW[3] = 
	{ 
		D3D11_TEXTURE_ADDRESS_WRAP,
		D3D11_TEXTURE_ADDRESS_WRAP,
		D3D11_TEXTURE_ADDRESS_WRAP 
	};
	Texture2D::CreateSampler(
		D3D11_FILTER_MIN_MAG_MIP_LINEAR,
		AddressUVW);

	//Load default vertex shader
	LoadDefaultVertexShader();
}

void Game::LoadDefaultVertexShader()
{
	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	auto hr = GDevice.d3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout), DefaultVertexShader, ARRAYSIZE(DefaultVertexShader), &GDevice.defaultLayout);
	ThrowIfFailed(hr);
	hr = GDevice.d3dDevice->CreateVertexShader(DefaultVertexShader, ARRAYSIZE(DefaultVertexShader), nullptr, &GDevice.defaultVS);
	ThrowIfFailed(hr);
}


// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSize()
{ 
	auto hr = S_FALSE;

	// Store the window bounds so the next time we get a SizeChanged event we can
	// avoid rebuilding everything if the size is identical.
	windowBounds = this->coreWindow->Bounds;

	renderTargetSize.Width = windowBounds.Width;
	renderTargetSize.Height = windowBounds.Height;

	//Set aspect ratio
	GDevice.AspectRatio = windowBounds.Width / windowBounds.Height;

	if(swapChain != nullptr)
	{
#pragma region Resize swap chain

		// If the swap chain already exists, resize it.
		auto hr = swapChain->ResizeBuffers(
			2, // Double-buffered swap chain.
			static_cast<UINT>(renderTargetSize.Width),
			static_cast<UINT>(renderTargetSize.Height),
			DXGI_FORMAT_B8G8R8A8_UNORM,
			0);
		ThrowIfFailed(hr);

#pragma endregion
	}
	else
	{
#pragma region Create swap chain

		// Otherwise, create a new one using the same adapter as the existing Direct3D device.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
		swapChainDesc.Width = static_cast<UINT>(renderTargetSize.Width); // Match the size of the window.
		swapChainDesc.Height = static_cast<UINT>(renderTargetSize.Height);
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the most common swap chain format.
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2; // Use double-buffering to minimize latency.
#ifdef XAML
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
#else
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
#endif // XAML
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
		swapChainDesc.Flags = 0;

		ComPtr<IDXGIDevice1>  dxgiDevice;
		hr = GDevice.d3dDevice.As(&dxgiDevice);
		ThrowIfFailed(hr);

		ComPtr<IDXGIAdapter> dxgiAdapter;
		hr = dxgiDevice->GetAdapter(&dxgiAdapter);
		ThrowIfFailed(hr);

		ComPtr<IDXGIFactory2> dxgiFactory;
		hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), &dxgiFactory);
		ThrowIfFailed(hr);

#ifdef XAML
			hr = dxgiFactory->CreateSwapChainForComposition(
				GDevice.d3dDevice.Get(),
				&swapChainDesc,
				nullptr,    // allow on all displays
				&this->swapChain);
			ThrowIfFailed(hr);

			ComPtr<ISwapChainBackgroundPanelNative> dxRootPanelAsNative;

			// set the swap chain on the SwapChainBackgroundPanel
			reinterpret_cast<IUnknown*>(this->swapChainPanel)->QueryInterface(__uuidof(ISwapChainBackgroundPanelNative),
				(void**)&dxRootPanelAsNative);
			dxRootPanelAsNative->SetSwapChain(swapChain.Get());
			ThrowIfFailed(hr);
#else
			auto window = this->coreWindow.Get();
			hr = dxgiFactory->CreateSwapChainForCoreWindow(
				GDevice.d3dDevice.Get(),
				reinterpret_cast<IUnknown*>(window),
				&swapChainDesc,
				nullptr, // Allow on all displays.
				&this->swapChain);
			ThrowIfFailed(hr);
#endif // XAML

#pragma endregion

		hr = dxgiDevice->SetMaximumFrameLatency(1);
		ThrowIfFailed(hr);
	}

#pragma region Create back buffer, depth-stencil & view port

	// Create a render target view of the swap chain back buffer.
	ComPtr<ID3D11Texture2D> backBuffer;
	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
	ThrowIfFailed(hr);

	hr = GDevice.d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView);
	ThrowIfFailed(hr);

	// Create a depth stencil view.
	CD3D11_TEXTURE2D_DESC depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT, 
		static_cast<UINT>(renderTargetSize.Width),
		static_cast<UINT>(renderTargetSize.Height),
		1,
		1,
		D3D11_BIND_DEPTH_STENCIL);

	ComPtr<ID3D11Texture2D> depthStencil;
	hr = GDevice.d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencil);
	ThrowIfFailed(hr);

	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	hr = GDevice.d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, &depthStencilView );
	ThrowIfFailed(hr);

	// Set the rendering viewport to target the entire window.
	CD3D11_VIEWPORT viewport(
		0.0f,
		0.0f,
		renderTargetSize.Width,
		renderTargetSize.Height);

	GDevice.d3dContext->RSSetViewports(1, &viewport);

#pragma endregion

#pragma region Create Direct2D Target

	D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
		96, 96);

	ComPtr<IDXGISurface> dxgiBackBuffer;
	hr = this->swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
	ThrowIfFailed(hr);

	ComPtr<ID2D1Bitmap1> d2dTarget;
	hr = GDevice.d2dContext->CreateBitmapFromDxgiSurface(dxgiBackBuffer.Get(), &bitmapProperties, &d2dTarget );
	ThrowIfFailed(hr);

	GDevice.d2dContext->SetTarget(d2dTarget.Get());

	// Grayscale text anti-aliasing is recommended for all Metro style apps.
	GDevice.d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

#pragma endregion
}

// This method is called in the event handler for the SizeChanged event.
void Game::WindowSizeChanged()
{
#pragma region OnWindowSize changed

	if (this->coreWindow->Bounds.Width  != windowBounds.Width ||
		this->coreWindow->Bounds.Height != windowBounds.Height)
	{
		GDevice.d2dContext->SetTarget(nullptr);
		GDevice.factory = nullptr;
		GDevice.d2dContext->Flush();

		ID3D11RenderTargetView* nullViews[] = {nullptr};
		GDevice.d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
		this->renderTargetView = nullptr;
		this->depthStencilView = nullptr;
		GDevice.d3dContext->Flush();
		CreateWindowSize();
	}

#pragma endregion
}

// Method to deliver the final image to the display.
void Game::Present()
{
#pragma region Present

	DXGI_PRESENT_PARAMETERS parameters = {0};
	parameters.DirtyRectsCount = 0;
	parameters.pDirtyRects = nullptr;
	parameters.pScrollRect = nullptr;
	parameters.pScrollOffset = nullptr;

	HRESULT hr = swapChain->Present1(1, 0, &parameters);
	GDevice.d3dContext->DiscardView(renderTargetView.Get());
	GDevice.d3dContext->DiscardView(depthStencilView.Get());

	if (hr == DXGI_ERROR_DEVICE_REMOVED)
	{
		HandleDeviceLost();
	}
	else
	{
		ThrowIfFailed(hr);
	}

#pragma endregion
}

