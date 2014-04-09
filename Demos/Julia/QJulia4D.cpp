//--------------------------------------------------------------------------------------
// File: QJulia4D.cpp
//
// Copyright (c) 2012, Microsoft
//  All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following 
// conditions are met:
//  - Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
//  - Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
//    disclaimer in the documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF 
// USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Coded by Jan Vlietinck, 11 Oct 2009, V 1.4
// http://users.skynet.be/fquake/
//--------------------------------------------------------------------------------------

#include <windows.h>
#include <d3d11.h>
#include <string>
#include <memory>

#include "resource.h"
#include "timer.h"
#include "Algebra.h"
#include "TrackBall.h"
#include "QJulia4DConstants.h"
#include "julia4DAMP.h"

using namespace concurrency;
using namespace concurrency::graphics;
using namespace concurrency::graphics::direct3d;

////////////////////////////////////////////////////////////////////////////////


static float g_Epsilon = 0.003f;
static float g_ColorT  = 0.0f;
static float4 g_ColorA(0.25f, 0.45f, 1.0f, 1.0f);
static float4 g_ColorB(0.25f, 0.45f, 1.0f, 1.0f);
static float4 g_ColorC(0.25f, 0.45f, 1.0f, 1.0f);

static float g_MuT = 0.0f;
static float4 g_MuA(-.278f, -.479f, 0.0f, 0.0f);
static float4 g_MuB(0.278f, 0.479f, 0.0f, 0.0f);
static float4 g_MuC(-.278f, -.479f, -.231f, .235f);

////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

const int g_errormsg_size = 256;
WCHAR g_errormsg[g_errormsg_size];

// size of window
int g_rows, g_cols;
int g_width, g_height; 
#define HCOLS (g_cols/2)
#define HROWS (g_rows/2)

//used to change behavior of UI
bool  g_animated = true;
bool  g_selfShadow = false;

//timing
float g_julia_timer=0;
Timer g_julia_time;
float g_zoom = 1.0f;
float g_deltaTime = 0.0f;

// for animation
TrackBall g_trackBall;

HINSTANCE                           g_hInst = NULL;
HWND                                g_hWnd = NULL;

// directx objects
D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
ID3D11Device*                       g_pd3dDevice = NULL;
ID3D11DeviceContext*                g_pImmediateContext = NULL ;
IDXGISwapChain*						g_pSwapChain = NULL;
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

// interop texture object object of the back buffer associated with the window 
std::unique_ptr<writeonly_texture_view<unorm4, 2>> g_pAmpTextureView;


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, unsigned int, WPARAM, LPARAM );
void Render();

//--------------------------------------------------------------------------------------
// WinMain
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE , LPWSTR , int nCmdShow )
{
	try
	{
		g_cols = (LONG)::GetSystemMetrics( SM_CXSCREEN );
		g_rows = (LONG)::GetSystemMetrics( SM_CYSCREEN );

		// Smaller than full screen
		g_cols = static_cast<int>(g_cols * 0.75);
		g_rows = static_cast<int>(g_rows * 0.75);

		SetCursorPos(HCOLS, HROWS); // set mouse to middle screen
  
		if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
		{
			return 0;
		}

		if( FAILED( InitDevice() ) )
		{
			CleanupDevice();
			return 0;
		}

		// Main message loop
		MSG msg = {0};
		while( WM_QUIT != msg.message )
		{
			if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
			else
			{
				Render();
			}
		}

		CleanupDevice();

		return ( int )msg.wParam;
	}
	catch(const std::exception& ex)
	{
		size_t convertedsize = 0;
		mbstowcs_s(&convertedsize, g_errormsg, g_errormsg_size, ex.what(), g_errormsg_size - 1);
		MessageBox(g_hWnd, g_errormsg, NULL, MB_OK);
		return 0;
	}
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
//***********************************************************************************
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
//***********************************************************************************
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof( WNDCLASSEX );
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_MAIN_ICON );
	wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"QJulia4DWindowClass";
	wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_MAIN_ICON );
	if( !RegisterClassEx( &wcex ) )
	{
		return E_FAIL;
	}

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, g_cols, g_rows };
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );

	g_hWnd = CreateWindow( L"QJulia4DWindowClass", L"QJulia4D DX11", WS_OVERLAPPEDWINDOW, 0, 0, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL );

	if( !g_hWnd )
	{
		return E_FAIL;
	}

	ShowWindow( g_hWnd, nCmdShow );

	return S_OK;
}


//***********************************************************************************
void Resize()
//***********************************************************************************
{ 
	if ( g_pd3dDevice == NULL)
	{
		return;
	}

	RECT rc;
	GetClientRect( g_hWnd, &rc );
	unsigned int width = rc.right - rc.left;
	unsigned int height = rc.bottom - rc.top;	

	g_height = height;
	g_width  = width;

	// release references to back buffer before resize, else fails
	g_pAmpTextureView.reset(nullptr);
  
	DXGI_SWAP_CHAIN_DESC sd;
	g_pSwapChain->GetDesc(&sd);
	HRESULT hr = g_pSwapChain->ResizeBuffers(sd.BufferCount, width, height, sd.BufferDesc.Format, 0);
	if(FAILED(hr))
	{
		throw std::exception("Failed to resize buffers");
	}

	ID3D11Texture2D* pTexture;
	hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pTexture );
	if(FAILED(hr))
	{
		throw std::exception("Failed to get buffer");
	}


	// create amp texture using interop
	accelerator_view av = concurrency::direct3d::create_accelerator_view(g_pd3dDevice);
	texture<unorm4, 2> tex = make_texture<unorm4, 2>(av, pTexture);
	g_pAmpTextureView.reset(new writeonly_texture_view<unorm4, 2>(tex));

	pTexture->Release();
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
//***********************************************************************************
HRESULT InitDevice()
//***********************************************************************************
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect( g_hWnd, &rc );
	unsigned int width = rc.right - rc.left;
	unsigned int height = rc.bottom - rc.top;

	unsigned int createDeviceFlags = 0;
	#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof( sd ) );
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;    
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	D3D_FEATURE_LEVEL FeatureLevel[] = {D3D_FEATURE_LEVEL_11_0};
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS | DXGI_USAGE_SHADER_INPUT;

	// Use default DirectX11 hardware 
	hr = D3D11CreateDeviceAndSwapChain( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, FeatureLevel, 1,  D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, NULL, &g_pImmediateContext );		
	if(FAILED(hr))
	{
		throw std::exception("Failed to create DirectX11 device.");		
	}

	// Initialize C++ AMP texture
    Resize();

    // other random animation for each run
    srand(2012);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Cleanup
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if( g_pImmediateContext ) g_pImmediateContext->ClearState();

	if( g_pSwapChain )
	{
		g_pSwapChain->SetFullscreenState(false, NULL); // switch back to full screen else not working ok
		g_pSwapChain->Release();
	}

	if( g_pImmediateContext ) g_pImmediateContext->Release();
	if( g_pd3dDevice ) g_pd3dDevice->Release();
}

//--------------------------------------------------------------------------------------
// Handle keyboard and mouse events
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam )
{
	const float fStepSize = 0.05f;    
	static bool lbdown = false;

	PAINTSTRUCT ps;
	HDC hdc;

	// Consolidate the mouse button messages and pass them to the right volume renderer
	if( msg == WM_LBUTTONDOWN ||
		msg == WM_LBUTTONUP ||
		msg == WM_LBUTTONDBLCLK ||
		msg == WM_MBUTTONDOWN ||
		msg == WM_MBUTTONUP ||
		msg == WM_MBUTTONDBLCLK ||
		msg == WM_RBUTTONDOWN ||
		msg == WM_RBUTTONUP ||
		msg == WM_RBUTTONDBLCLK ||
		msg == WM_MOUSEWHEEL || 
		msg == WM_MOUSEMOVE )
	{
		int xPos = (short)LOWORD(lParam);
		int yPos = (short)HIWORD(lParam);

		int nMouseWheelDelta = 0;
		if( msg == WM_MOUSEWHEEL ) 
		{
			nMouseWheelDelta = (short) HIWORD(wParam);
			if (nMouseWheelDelta < 0)
			{
				g_zoom = g_zoom * 1.1f;
			}
			else
			{
				g_zoom = g_zoom / 1.1f;
			}
		}

		if (msg == WM_LBUTTONDOWN && !lbdown)
		{
			g_trackBall.DragStart(xPos, yPos, g_width, g_height);
			lbdown = true;
		}

		if (msg == WM_MOUSEMOVE && lbdown)
		{
			g_trackBall.DragMove(xPos, yPos, g_width, g_height);
		}

		if (msg == WM_LBUTTONUP)
		{
			g_trackBall.DragEnd();
			lbdown = false;
		}
	}
	else switch( msg)
	{
		case WM_PAINT:
		{
			hdc = BeginPaint( hWnd, &ps );
			EndPaint( hWnd, &ps );
			break;
		}

		case WM_CREATE:
		{
			break;
		}

		case WM_CLOSE:
		{
			DestroyWindow( hWnd );
			UnregisterClass( L"QJulia4DWindowClass", NULL );
			return 0;
		}

		case WM_DESTROY:
		{
			PostQuitMessage( 0 );
			break;
		}

		case WM_SIZE:          
		{
			if ( wParam != SIZE_MINIMIZED )
			{
				try
				{
					Resize();
				}
				catch(const std::exception& ex)
				{
					size_t convertedsize = 0;
					mbstowcs_s(&convertedsize, g_errormsg, g_errormsg_size, ex.what(), g_errormsg_size - 1);
					MessageBox(g_hWnd, g_errormsg, NULL, MB_OK);
					return 0;
				}
			}
			break;
		}

		case WM_KEYDOWN:
		{
				char key = (char)tolower((int)wParam);
				switch (key)
				{
					case 's':  // toggle selfshadow
						g_selfShadow = !g_selfShadow;
						break;

					case '+':
						if(g_Epsilon >= 0.002f)
						{
							g_Epsilon *= (1.0f / 1.05f);
						}
						break;

					case '-':
						if(g_Epsilon < 0.01f)
						{
							g_Epsilon *= 1.05f;
						}
						break;

					case 'w':
						g_MuC.x += fStepSize; 
						break;

					case 'x':
						g_MuC.x -= fStepSize; 
						break;

					case 'q':
						g_MuC.y += fStepSize; 
						break;

					case 'z':
						g_MuC.y -= fStepSize; 
						break;

					case 'a':
						g_MuC.z += fStepSize; 
						break;

					case 'd':
						g_MuC.z -= fStepSize; 
						break;

					case 'e':
						g_MuC.w += fStepSize; 
						break;

					case 'c':
						g_MuC.w -= fStepSize; 
						break;
				}
			
				switch( wParam )
				{
					case VK_ESCAPE:
						SendMessage( hWnd, WM_CLOSE, 0, 0 );
						break;

					case VK_ADD :
						if(g_Epsilon >= 0.002f)
						g_Epsilon *= (1.0f / 1.05f);
						break;

					case VK_SUBTRACT :
						if(g_Epsilon < 0.01f)
						g_Epsilon *= 1.05f;
						break;

					case VK_SPACE :
						g_animated = !g_animated;
						break;
				} 
				break;
			}

			default:
			{
				return DefWindowProc( hWnd, msg, wParam, lParam );
			}
	}	

	return 0;
}


//--------------------------------------------------------------------------------------
// Functions to update various parameters of the fractal
//--------------------------------------------------------------------------------------
static void Interpolate( float4& m, float t, float4& a, float4& b )
{
	m = ( 1.0f - t ) * a + t * b;
}

static void UpdateMu( float* t, float4& a, float4& b )
{
	*t += 0.01f *g_deltaTime;
    
	if ( *t >= 1.0f )
	{
		*t = 0.0f;

		a = b;

		b = 2.0f * rand() / (float) RAND_MAX - 1.0f;
	}
}

static void RandomColor( float4& v )
{
	do
	{
		v.r = 2.0f * rand() / (float) RAND_MAX - 1.0f;
		v.g = 2.0f * rand() / (float) RAND_MAX - 1.0f;
		v.b = 2.0f * rand() / (float) RAND_MAX - 1.0f;
	}
	while (v.r < 0 && v.g < 0 && v.b < 0); // prevent black colors

	v.a = 1.0f;
}

static void UpdateColor( float* t, float4& a, float4& b )
{
	*t += 0.01f *g_deltaTime;
   
	if ( *t >= 1.0f )
	{
		*t = 0.0f;

		a = b;

		RandomColor(b);
	}
}

static void UpdateJuliaConstants(QJulia4DConstants& mc)
{		
	mc.c_height = g_height;
	mc.c_width  = g_width;
	mc.diffuse = g_ColorC;
	mc.epsilon = g_Epsilon;
	mc.mu = g_MuC;
	for (int j=0; j<3; j++)
	{
		for (int i=0; i<3; i++)
		{
			mc.orientation[i + 4*j] = g_trackBall.GetRotationMatrix()(j,i);
		}
	}
	mc.selfShadow = g_selfShadow;
	mc.zoom = g_zoom;
}

//--------------------------------------------------------------------------------------
// Draw fractal to the screen 
//--------------------------------------------------------------------------------------
void Render()
{
	static float elapsedPrev = 0;
	float t = g_julia_time.Elapsed();
	g_deltaTime = (elapsedPrev ==0) ? 1 :(t-elapsedPrev)/1000 *20;
	elapsedPrev = t;

	// prepare next frame if animated
	if(g_animated)
	{
		UpdateMu( &g_MuT, g_MuA, g_MuB );
		Interpolate( g_MuC, g_MuT, g_MuA, g_MuB );
  
		UpdateColor( &g_ColorT, g_ColorA, g_ColorB );
		Interpolate(g_ColorC, g_ColorT, g_ColorA, g_ColorB );
	}

	QJulia4DConstants mc;
	UpdateJuliaConstants(mc);
	
	writeonly_texture_view<unorm4, 2> tv = *g_pAmpTextureView.get();   

	ampCompute(tv, mc);
  
	// Present our back buffer to our front buffer
	g_pSwapChain->Present( 0, 0 );  

  	
	if ( (t=g_julia_time.Elapsed()/1000) - g_julia_timer > 1) // every second
	{  
		g_julia_timer = t;
	
		swprintf_s(g_errormsg,L"QJulia4D on %s, zoom: %f", tv.accelerator_view.accelerator.description.c_str(), g_zoom);		
	
		SetWindowText(g_hWnd, g_errormsg);		
	}
}