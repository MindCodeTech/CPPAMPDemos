//--------------------------------------------------------------------------------------
// File: window.cpp
//
// This application uses C++ AMP, AMPBLAS, AMPLAPACK, and DirectX interoperability to
// numerically simulate antennas of varying lengths and plots the power versus angle.
//
// Based on this demo:
// http://msdn.microsoft.com/en-us/library/windows/apps/ff729719.aspx
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <thread>
#include <dxgi.h>
#include <d3dcommon.h>
#include <d3d9types.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <amp.h>
#include <amp_graphics.h>
#include <stdio.h>
#include "resource.h"
#include "antenna_sim.h"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Configuration
//--------------------------------------------------------------------------------------

// output dimensions (and initial window size)
int width = 1280;
int height = 960;

// controls the precision of each line - higher numbers use more memory and take longer
int complexity = 512;

// compute precision - use float for gaming accelerators
typedef float precision;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE                 g_hInst = nullptr;
HWND                      g_hWnd = nullptr;
ID3D11Device*             g_pd3dDevice = nullptr;
ID3D11DeviceContext*      g_pImmediateContext = nullptr;
IDXGISwapChain*           g_pSwapChain = nullptr;
ID3D11RenderTargetView*   g_pRenderTargetView = nullptr;
ID3D11VertexShader*       g_pVertexShader = nullptr;
ID3D11PixelShader*        g_pPixelShader = nullptr;
ID3D11InputLayout*        g_pVertexLayout = nullptr;
ID3D11Buffer*             g_pVertexBuffer = nullptr;
ID3D11ShaderResourceView* g_pSRV = nullptr;
ID3D11ShaderResourceView* g_pSRV2 = nullptr;

std::mutex g_thread_m;
std::unique_ptr<std::thread> g_worker;
std::unique_ptr<antenna_sim<precision>> g_as;

concurrency::accelerator_view g_av(concurrency::accelerator().default_view);
std::unique_ptr<concurrency::graphics::texture<float,2>> g_tex;
std::unique_ptr<concurrency::graphics::texture<float,2>> g_tex2;
std::atomic<bool> g_render_switch = false;
std::wstring g_gpu_description;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    try
    {
        UNREFERENCED_PARAMETER( hPrevInstance );
        UNREFERENCED_PARAMETER( lpCmdLine );

        if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
            return 0;

        if( FAILED( InitDevice() ) )
        {
            CleanupDevice();
            return 0;
        }

        // Main message loop
        MSG msg = {0};
        while( WM_QUIT != msg.message )
        {
            if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
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
    catch ( std::exception &e)
    {
        const char* s = e.what();
        MessageBoxA( nullptr, s, "Error", MB_OK);

        return 1;
    }
}

HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_ICON );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"AntennaWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_ICON );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    g_hInst = hInstance;
    RECT rc = { 0, 0, width, height};
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"AntennaWindowClass", L"Dipole Antenna Demo - Antenna Length Sweeps",
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                           nullptr );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}

std::unique_ptr<char[]> ReadShader(const char * file, size_t & length)
{
    std::ifstream ifstream(file, std::ios::binary);

    if (!ifstream.good()) 
    {
        throw std::exception("Failed to open shader file");
    }

    ifstream.seekg(0, std::ios::end);
    length = static_cast<size_t>(ifstream.tellg());
    ifstream.seekg(0, std::ios::beg);
    std::unique_ptr<char[]> p(new char[length]);
    ifstream.read(p.get(), length);
    ifstream.close();

    return p;
}

HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        // these can be attempted for debugging purposes
        //D3D_DRIVER_TYPE_WARP,
        //D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        D3D_DRIVER_TYPE g_driverType = driverTypes[driverTypeIndex];
        D3D_FEATURE_LEVEL g_featureLevel;
        hr = D3D11CreateDeviceAndSwapChain( nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"No DirectX11 devices were found", L"Error", MB_OK );
        return hr;
    }

    IDXGIAdapter *pAdapter = NULL;
    IDXGIDevice *pDevice = NULL;
    hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDevice);
    DXGI_ADAPTER_DESC adapterDesc;
    if (FAILED(hr) || (pDevice == NULL))
    {
        return hr;
    }

    hr = pDevice->GetAdapter(&pAdapter);
    if (FAILED(hr)) 
    {
        return hr;
    }

    hr = pAdapter->GetDesc(&adapterDesc);
    if (FAILED(hr)) 
    {
        return hr;
    }

    g_gpu_description = adapterDesc.Description;

    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, nullptr );

    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

    size_t vs_length;
    auto pp_VSCode = ReadShader("VertexShader.cso", vs_length);

    hr = g_pd3dDevice->CreateVertexShader( pp_VSCode.get(), vs_length, nullptr, &g_pVertexShader );
    if( FAILED( hr ) )
    {
        return hr;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE( layout );

    hr = g_pd3dDevice->CreateInputLayout( layout, numElements,  pp_VSCode.get(),
                                          vs_length, &g_pVertexLayout );
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->IASetInputLayout( g_pVertexLayout );

    size_t ps_length;
    auto pp_PSCode = ReadShader("PixelShader.cso", ps_length);
    hr = g_pd3dDevice->CreatePixelShader( pp_PSCode.get(), ps_length, nullptr, &g_pPixelShader );
    if( FAILED( hr ) )
        return hr;

    struct SimpleVertex
    {
        XMFLOAT3 Pos;
        XMFLOAT2 Tex;
    };

    // Two textured triangles, forming a flat square
    SimpleVertex vertices[] =
    {
        { XMFLOAT3( -1.f, -1.f, 0.5f ), XMFLOAT2(0.f,0.f) },
        { XMFLOAT3( -1.f,  1.f, 0.5f ), XMFLOAT2(0.f,(float)height) },
        { XMFLOAT3(  1.f, -1.f, 0.5f ), XMFLOAT2((float)width,0.f) },
        { XMFLOAT3(  1.f,  1.f, 0.5f ), XMFLOAT2((float)width,(float)height) },
    };

    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( vertices );
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory( &InitData, sizeof(InitData) );
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
    if( FAILED( hr ) )
        return hr;

    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

    g_pImmediateContext->VSSetShader( g_pVertexShader, nullptr, 0 );
    g_pImmediateContext->PSSetShader( g_pPixelShader, nullptr, 0 );

    // initialize amp portion and the display texture and its SRV
    g_av = concurrency::direct3d::create_accelerator_view(g_pd3dDevice);
    g_tex.reset(new concurrency::graphics::texture<float,2>(int(height),int(width),g_av));
    g_tex2.reset(new concurrency::graphics::texture<float,2>(int(height),int(width),g_av));
    auto pTexture = (ID3D11Texture2D*) concurrency::graphics::direct3d::get_texture(*g_tex);
    auto pTexture2 = (ID3D11Texture2D*) concurrency::graphics::direct3d::get_texture(*g_tex2);
    hr = g_pd3dDevice->CreateShaderResourceView(pTexture, nullptr, &g_pSRV);
    if( FAILED( hr ) )
        return hr;
    hr = g_pd3dDevice->CreateShaderResourceView(pTexture2, nullptr, &g_pSRV2);
    if( FAILED( hr ) )
        return hr;

    try
    {
        // initialization of the g_worker thread and data
        g_as.reset(new antenna_sim<precision>(complexity,width,height,g_thread_m,g_tex, g_tex2, g_render_switch));
    }
    catch(...)
    {
        MessageBox( nullptr,
                    L"Unable to allocate solver resources - out of accelerator memory likely", L"Error", MB_OK );
        return S_FALSE;
    }

    // start a worker thread using the runnable function in the simulator
    g_worker.reset(new std::thread( std::ref(*g_as)));

    return S_OK;
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        case WM_KEYDOWN:
            g_as->toggle();

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }
    return 0;
}

void Render()
{
    // build the window title
    std::wostringstream os;
    if (g_as->get_device() == L"CPU")
        os << L"Current computing device: " << g_as->get_device();
    else
        os << L"Current computing device: " << g_gpu_description;

    auto fps = g_as->get_fps();
    if( fps > 0 )
    {
        os << " - FPS: " << std::setprecision(3) << fps;
    }
    SetWindowText(g_hWnd, os.str().c_str());

    std::lock_guard<std::mutex> lg(g_thread_m); // protect access to the shared texture

    // bind color map and data textures
    auto resource = g_render_switch ? g_pSRV2 : g_pSRV;
    g_pImmediateContext->PSSetShaderResources( 0, 1, &resource );

    // draw a shaded poly
    g_pImmediateContext->Draw( 4, 0 );
    g_pSwapChain->Present( 0, 0 );

    // unbind the texture so the compute thread can use it
    resource = nullptr;
    g_pImmediateContext->PSSetShaderResources( 0, 1, &resource );
}

void CleanupDevice()
{
    if( g_as ) g_as->quit();
    if( g_worker ) g_worker->join();

    if( g_pImmediateContext ) g_pImmediateContext->ClearState();
    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pSRV ) g_pSRV->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}
