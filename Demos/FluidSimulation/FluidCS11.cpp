//--------------------------------------------------------------------------------------
// File: FluidCS11.cpp
//
// C++ AMP computation results are render using DXUT library
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Smoothed Particle Hydrodynamics Algorithm Based Upon:
// Particle-Based Fluid Simulation for Interactive Applications
// Matthias Müller
//--------------------------------------------------------------------------------------

#include "stdafx.h"
// #include "DXUT.h"
// #include "DXUTgui.h"
// #include "DXUTsettingsDlg.h"
// #include "SDKmisc.h"
#include "resource.h"
#include "fluidsimulation.h"

using namespace DirectX;
using namespace DXUT;

#ifndef _DECLSPEC_ALIGN_16_
#define _DECLSPEC_ALIGN_16_ __declspec(align(16))
#endif

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

// The algorithm can be extended to support any number of particles,
// but to keep the sample simple, we do not implement boundary conditions to handle it.
const unsigned NUM_PARTICLES_2K = 2 * 1024;
const unsigned NUM_PARTICLES_4K = 4 * 1024;
const unsigned NUM_PARTICLES_8K = 8 * 1024;
const unsigned NUM_PARTICLES_16K = 16 * 1024;
const unsigned NUM_PARTICLES_32K = 32 * 1024;
const unsigned NUM_PARTICLES_64K = 64 * 1024;
unsigned g_iNumParticles = NUM_PARTICLES_8K;

// Particle Properties
// These will control how the fluid behaves
float g_fSmoothlen = 0.012f;
float g_fPressureStiffness = 200.0f;
float g_fRestDensity = 1000.0f;
float g_fParticleMass = 0.0002f;
float g_fViscosity = 0.1f;
float g_fMaxAllowableTimeStep = 0.005f;
float g_fParticleRenderSize = 0.003f;

// Gravity Directions
XMFLOAT2A g_vGravity(0, -0.5f);

// Map Size
// These values should not be larger than 256 * fSmoothlen
// Since the map must be divided up into fSmoothlen sized grid cells
// And the grid cell is used as a 16-bit sort key, 8-bits for x and y
float g_fMapHeight = 1.2f;
float g_fMapWidth = (4.0f / 3.0f) * g_fMapHeight;

// Map Wall Collision Planes
float g_fWallStiffness = 3000.0f;
XMFLOAT3A g_vPlanes[4] = {
    XMFLOAT3A(1, 0, 0),
    XMFLOAT3A(0, 1, 0),
    XMFLOAT3A(-1, 0, g_fMapWidth),
    XMFLOAT3A(0, -1, g_fMapHeight)
};

// Simulation
fluid_simulation* g_fluid_simulation = nullptr;

//--------------------------------------------------------------------------------------
// Direct3D11 Global variables
//--------------------------------------------------------------------------------------
ID3D11ShaderResourceView* const     g_pNullSRV = nullptr;    // Helper to Clear SRVs
ID3D11Buffer* const                 g_pNullBuffer = nullptr; // Helper to Clear Buffers
UINT                                g_iNullUINT = 0;         // Helper to Clear Buffers

CDXUTDialogResourceManager          g_DialogResourceManager; // manager for shared resources of dialogs
CDXUTDialog                         g_HUD;                   // manages the 3D

// Resources
CDXUTTextHelper*                    g_pTxtHelper = nullptr;

// Shaders
ID3D11VertexShader*                 g_pParticleVS = nullptr;
ID3D11GeometryShader*               g_pParticleGS = nullptr;
ID3D11PixelShader*                  g_pParticlePS = nullptr;

// Structured Buffers
ID3D11Buffer*                       g_pParticles = nullptr;
ID3D11ShaderResourceView*           g_pParticlesSRV = nullptr;

ID3D11Buffer*                       g_pParticleDensity = nullptr;
ID3D11ShaderResourceView*           g_pParticleDensitySRV = nullptr;

// Constant Buffer Layout
#pragma warning(push)
#pragma warning(disable:4324) // structure was padded due to __declspec(align())
_DECLSPEC_ALIGN_16_ struct CBRenderConstants
{
    XMFLOAT4X4 mViewProjection;
    float fParticleSize;
};
#pragma warning(pop)

// Constant Buffers
ID3D11Buffer*                       g_pcbRenderConstants = nullptr;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_RESETSIM                5
#define IDC_NUMPARTICLES            6
#define IDC_GRAVITY_X_LABEL         8
#define IDC_GRAVITY_X               9
#define IDC_GRAVITY_Y_LABEL         10
#define IDC_GRAVITY_Y               11
#define IDC_SMOOTHLEN_LABEL         12
#define IDC_SMOOTHLEN               13
#define IDC_PRESSURESTIFF_LABEL     14
#define IDC_PRESSURESTIFF           15
#define IDC_RESTDENSITY_LABEL       16
#define IDC_RESTDENSITY             17
#define IDC_PARTICLEMASS_LABEL      18
#define IDC_PARTICLEMASS            19
#define IDC_VISCOSITY_LABEL         20
#define IDC_VISCOSITY               21

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext );

void UpdateGUI();
HRESULT CreateSimulationBuffers( ID3D11Device* pd3dDevice );
void InitApp();

void RenderText();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    try
    {
        // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
        _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

        // Set DXUT callbacks
        DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
        DXUTSetCallbackMsgProc( MsgProc );

        DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
        DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
        DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
        DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
        DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

        InitApp();
        DXUTInit( true, true ); // Parse the command line, show msgboxes on error, and an extra cmd line param to force REF for now
        DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
        DXUTCreateWindow( L"C++ AMP - Fluid Simulation" );
        DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 1024, 768 );
        DXUTMainLoop(); // Enter into the DXUT render loop

        return DXUTGetExitCode();
    }
    catch(const std::exception& ex)
    {
        MessageBoxA(0, ex.what(), "Exception caught!", MB_ICONERROR);
        return 1;
    }
    catch(...)
    {
        MessageBoxA(0, "Unknown exception", "Exception caught!", MB_ICONERROR);
        return 1;
    }
}

//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    // Initialize dialogs
    g_HUD.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 20;
  
    g_HUD.AddButton( IDC_RESETSIM, L"Reset Particles", 0, iY, 170, 22 );
    
    g_HUD.AddComboBox( IDC_NUMPARTICLES, 0, iY += 26, 170, 22 );
    g_HUD.GetComboBox( IDC_NUMPARTICLES )->AddItem( L"2K Particles", UIntToPtr(NUM_PARTICLES_2K) );
    g_HUD.GetComboBox( IDC_NUMPARTICLES )->AddItem( L"4K Particles", UIntToPtr(NUM_PARTICLES_4K) );
    g_HUD.GetComboBox( IDC_NUMPARTICLES )->AddItem( L"8K Particles", UIntToPtr(NUM_PARTICLES_8K) );
    g_HUD.GetComboBox( IDC_NUMPARTICLES )->AddItem( L"16K Particles", UIntToPtr(NUM_PARTICLES_16K) );
    g_HUD.GetComboBox( IDC_NUMPARTICLES )->AddItem( L"32K Particles", UIntToPtr(NUM_PARTICLES_32K) );
    g_HUD.GetComboBox( IDC_NUMPARTICLES )->AddItem( L"64K Particles", UIntToPtr(NUM_PARTICLES_64K) );
    g_HUD.GetComboBox( IDC_NUMPARTICLES )->SetSelectedByData( UIntToPtr(g_iNumParticles) );

    g_HUD.AddStatic( IDC_GRAVITY_X_LABEL, L"", 0, iY += 26, 170, 22 );
    g_HUD.AddSlider( IDC_GRAVITY_X, 0, iY += 13, 170, 22, -100, 100, static_cast<int>(g_vGravity.x * 100) );

    g_HUD.AddStatic( IDC_GRAVITY_Y_LABEL, L"", 0, iY += 26, 170, 22 );
    g_HUD.AddSlider( IDC_GRAVITY_Y, 0, iY += 13, 170, 22, -100, 100, static_cast<int>(g_vGravity.y * 100) );

    g_HUD.AddStatic( IDC_SMOOTHLEN_LABEL, L"", 0, iY += 26, 170, 22 );
    g_HUD.AddSlider( IDC_SMOOTHLEN, 0, iY += 13, 170, 22, 10, 20, static_cast<int>(g_fSmoothlen * 1000) );

    g_HUD.AddStatic( IDC_PRESSURESTIFF_LABEL, L"", 0, iY += 26, 170, 22 );
    g_HUD.AddSlider( IDC_PRESSURESTIFF, 0, iY += 13, 170, 22, 1, 500, static_cast<int>(g_fPressureStiffness) );

    g_HUD.AddStatic( IDC_RESTDENSITY_LABEL, L"", 0, iY += 26, 170, 22 );
    g_HUD.AddSlider( IDC_RESTDENSITY, 0, iY += 13, 170, 22, 500, 5000, static_cast<int>(g_fRestDensity) );

    g_HUD.AddStatic( IDC_PARTICLEMASS_LABEL, L"", 0, iY += 26, 170, 22 );
    g_HUD.AddSlider( IDC_PARTICLEMASS, 0, iY += 13, 170, 22, 10, 100, static_cast<int>(g_fParticleMass * 100000) );

    g_HUD.AddStatic( IDC_VISCOSITY_LABEL, L"", 0, iY += 26, 170, 22 );
    g_HUD.AddSlider( IDC_VISCOSITY, 0, iY += 13, 170, 22, 1, 100, static_cast<int>(g_fViscosity * 100) );

    UpdateGUI();
}

//--------------------------------------------------------------------------------------
// Called right before creating a D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
		if (( /* DXUT_D3D11_DEVICE == pDeviceSettings->ver && */
              pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning();
        }
    }

    // Use debug device is a debug build
#ifdef _DEBUG
    pDeviceSettings->d3d11.CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    return true;
}

//--------------------------------------------------------------------------------------
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
	g_pTxtHelper->SetForegroundColor(Colors::Yellow);
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
    g_pTxtHelper->DrawFormattedTextLine( L"%i Particles", g_iNumParticles );
    g_pTxtHelper->End();
}

//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );

    return 0;
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_RESETSIM:
            CreateSimulationBuffers( DXUTGetD3D11Device() );
            break;

        case IDC_NUMPARTICLES:
            g_iNumParticles = PtrToUint( ((CDXUTComboBox*)pControl)->GetSelectedData() );
            CreateSimulationBuffers( DXUTGetD3D11Device() );
            break;

        case IDC_GRAVITY_X:
            g_vGravity.x = ((CDXUTSlider*)pControl)->GetValue() * 0.01f;
            UpdateGUI();
            break;

        case IDC_GRAVITY_Y:
            g_vGravity.y = ((CDXUTSlider*)pControl)->GetValue() * 0.01f;
            UpdateGUI();
            break;

        case IDC_SMOOTHLEN:
            g_fSmoothlen = ((CDXUTSlider*)pControl)->GetValue() * 0.001f;
            UpdateGUI();
            break;

        case IDC_PRESSURESTIFF:
            g_fPressureStiffness = ((CDXUTSlider*)pControl)->GetValue() * 1.f;
            UpdateGUI();
            break;

        case IDC_RESTDENSITY:
            g_fRestDensity = ((CDXUTSlider*)pControl)->GetValue() * 1.f;
            UpdateGUI();
            break;

        case IDC_PARTICLEMASS:
            g_fParticleMass = ((CDXUTSlider*)pControl)->GetValue() * 0.00001f;
            UpdateGUI();
            break;

        case IDC_VISCOSITY:
            g_fViscosity = ((CDXUTSlider*)pControl)->GetValue() * 0.01f;
            UpdateGUI();
            break;
    }
}

//--------------------------------------------------------------------------------------
// Updates GUI labels
//--------------------------------------------------------------------------------------
void UpdateGUI()
{
    wchar_t text_buffer[256];

    swprintf(text_buffer, 256, L"Gravity X: %.2f", g_vGravity.x);
    g_HUD.GetStatic(IDC_GRAVITY_X_LABEL)->SetText(text_buffer);

    swprintf(text_buffer, 256, L"Gravity Y: %.2f", g_vGravity.y);
    g_HUD.GetStatic(IDC_GRAVITY_Y_LABEL)->SetText(text_buffer);

    swprintf(text_buffer, 256, L"Smooth. len.: %.3f", g_fSmoothlen);
    g_HUD.GetStatic(IDC_SMOOTHLEN_LABEL)->SetText(text_buffer);

    swprintf(text_buffer, 256, L"Press. stiff.: %.0f", g_fPressureStiffness);
    g_HUD.GetStatic(IDC_PRESSURESTIFF_LABEL)->SetText(text_buffer);

    swprintf(text_buffer, 256, L"Rest density: %.0f", g_fRestDensity);
    g_HUD.GetStatic(IDC_RESTDENSITY_LABEL)->SetText(text_buffer);

    swprintf(text_buffer, 256, L"Particle mass: %.5f", g_fParticleMass);
    g_HUD.GetStatic(IDC_PARTICLEMASS_LABEL)->SetText(text_buffer);

    swprintf(text_buffer, 256, L"Viscosity: %.2f", g_fViscosity);
    g_HUD.GetStatic(IDC_VISCOSITY_LABEL)->SetText(text_buffer);
}

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut, const D3D_SHADER_MACRO* pDefines = nullptr )
{
    HRESULT hr = S_OK;

    // find the file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    // compile the shader
    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile( str, pDefines, nullptr, szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
    if( FAILED(hr) )
    {
        if( pErrorBlob != nullptr )
            OutputDebugStringA( reinterpret_cast<char*>(pErrorBlob->GetBufferPointer()) );
        SAFE_RELEASE( pErrorBlob );
        return hr;
    }
    SAFE_RELEASE( pErrorBlob );

    return S_OK;   
}

//--------------------------------------------------------------------------------------
// Helper for creating constant buffers
//--------------------------------------------------------------------------------------
template <class T>
HRESULT CreateConstantBuffer( ID3D11Device* pd3dDevice, ID3D11Buffer** ppCB )
{
    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = 0;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = sizeof( T );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, nullptr, ppCB ) );

    return hr;
}

//--------------------------------------------------------------------------------------
// Helper for creating shader resource views
//--------------------------------------------------------------------------------------
template <class T>
HRESULT CreateSRV( ID3D11Device* pd3dDevice, UINT iNumElements, ID3D11Buffer** ppBuffer, ID3D11ShaderResourceView** ppSRV )
{
    HRESULT hr = S_OK;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory( &srvDesc, sizeof(srvDesc) );
    srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    srvDesc.BufferEx.FirstElement = 0;
    srvDesc.BufferEx.NumElements = iNumElements * sizeof(T) / sizeof(float);
    srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
    V_RETURN( pd3dDevice->CreateShaderResourceView( *ppBuffer, &srvDesc, ppSRV ) );

    return hr;
}

//--------------------------------------------------------------------------------------
// Create the buffers used for the simulation data
//--------------------------------------------------------------------------------------
HRESULT CreateSimulationBuffers( ID3D11Device* pd3dDevice )
{
    HRESULT hr = S_OK;

    // Destroy the old buffers in case the number of particles has changed
    SAFE_RELEASE( g_pParticles );
    SAFE_RELEASE( g_pParticlesSRV );

    SAFE_RELEASE( g_pParticleDensity );
    SAFE_RELEASE( g_pParticleDensitySRV );

    delete g_fluid_simulation;

    // Create simulation object
    g_fluid_simulation = new fluid_simulation(g_iNumParticles, pd3dDevice);

    // Create buffer views
    g_pParticles = g_fluid_simulation->get_particles_buffer();
    V_RETURN(CreateSRV<fluid_simulation::particle>(pd3dDevice, g_iNumParticles, &g_pParticles, &g_pParticlesSRV));
    DXUT_SetDebugName( g_pParticles, "Particles" );
    DXUT_SetDebugName( g_pParticlesSRV, "Particles SRV" );

    g_pParticleDensity = g_fluid_simulation->get_particles_density_buffer();
    V_RETURN(CreateSRV<float>(pd3dDevice, g_iNumParticles, &g_pParticleDensity, &g_pParticleDensitySRV));
    DXUT_SetDebugName( g_pParticleDensity, "Density" );
    DXUT_SetDebugName( g_pParticleDensitySRV, "Density SRV" );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

    // Compile the Shaders
    ID3DBlob* pBlob = nullptr;

    // Rendering Shaders
	V_RETURN(DXUTCompileFromFile(L"\\..\\Media\\Shaders\\FluidRender.hlsl", nullptr, "ParticleVS", "vs_4_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlob));
    V_RETURN( pd3dDevice->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &g_pParticleVS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pParticleVS, "ParticleVS" );

	V_RETURN(DXUTCompileFromFile(L"\\..\\Media\\Shaders\\FluidRender.hlsl", nullptr, "ParticleGS", "gs_4_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlob));
    V_RETURN( pd3dDevice->CreateGeometryShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &g_pParticleGS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pParticleGS, "ParticleGS" );

	V_RETURN(DXUTCompileFromFile(L"\\..\\Media\\Shaders\\FluidRender.hlsl", nullptr, "ParticlePS", "ps_4_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlob));
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &g_pParticlePS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pParticlePS, "ParticlePS" );

    // Create the Simulation Buffers
    V_RETURN( CreateSimulationBuffers( pd3dDevice ) );

    // Create Constant Buffers
    V_RETURN( CreateConstantBuffer< CBRenderConstants >( pd3dDevice, &g_pcbRenderConstants ) );
    DXUT_SetDebugName( g_pcbRenderConstants, "Render" );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 200, 0 );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Fluid Simulation
//--------------------------------------------------------------------------------------
void SimulateFluid( ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime )
{
    fluid_simulation::params params;

    // Clamp the time step when the simulation runs slowly to prevent numerical explosion
    params.f_time_step = __min(g_fMaxAllowableTimeStep, fElapsedTime);
    params.f_smooth_len = g_fSmoothlen;
    params.f_pressure_stiffness = g_fPressureStiffness;
    params.f_rest_density = g_fRestDensity;
    params.f_density_coef = g_fParticleMass * 315.0f / (64.0f * XM_PI * pow(g_fSmoothlen, 9));
    params.f_grad_pressure_coef = g_fParticleMass * -45.0f / (XM_PI * pow(g_fSmoothlen, 6));
    params.f_lap_viscosity_coef = g_fParticleMass * g_fViscosity * 45.0f / (XM_PI * pow(g_fSmoothlen, 6));

    params.v_gravity.x = g_vGravity.x;
    params.v_gravity.y = g_vGravity.y;

    params.f_wall_stiffness = g_fWallStiffness;
    for (unsigned int i = 0 ; i < 4 ; i++)
    {
        params.v_planes[i].x = g_vPlanes[i].x;
        params.v_planes[i].y = g_vPlanes[i].y;
        params.v_planes[i].z = g_vPlanes[i].z;
    }

    g_fluid_simulation->simulate(params);
}

//--------------------------------------------------------------------------------------
// Fluid Rendering
//--------------------------------------------------------------------------------------
void RenderFluid( ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime )
{
    // Simple orthographic projection to display the entire map
    XMMATRIX mView = XMMatrixTranslation( -g_fMapWidth / 2.0f, -g_fMapHeight / 2.0f, 0 );
    XMMATRIX mProjection = XMMatrixOrthographicLH( g_fMapWidth, g_fMapHeight, 0, 1 );
    XMMATRIX mViewProjection = mView * mProjection;

    // Update Constants
    CBRenderConstants pData;
    XMStoreFloat4x4( &pData.mViewProjection, XMMatrixTranspose( mViewProjection ) );
    pData.fParticleSize = g_fParticleRenderSize;

    pd3dImmediateContext->UpdateSubresource( g_pcbRenderConstants, 0, nullptr, &pData, 0, 0 );
    
    // Set the shaders
    pd3dImmediateContext->VSSetShader( g_pParticleVS, nullptr, 0 );
    pd3dImmediateContext->GSSetShader( g_pParticleGS, nullptr, 0 );
    pd3dImmediateContext->PSSetShader( g_pParticlePS, nullptr, 0 );

    // Set the constant buffers
    pd3dImmediateContext->VSSetConstantBuffers( 0, 1, &g_pcbRenderConstants );
    pd3dImmediateContext->GSSetConstantBuffers( 0, 1, &g_pcbRenderConstants );
    pd3dImmediateContext->PSSetConstantBuffers( 0, 1, &g_pcbRenderConstants );

    // Setup the particles buffer and IA
    pd3dImmediateContext->VSSetShaderResources( 0, 1, &g_pParticlesSRV );
    pd3dImmediateContext->VSSetShaderResources( 1, 1, &g_pParticleDensitySRV );
    
    pd3dImmediateContext->IASetVertexBuffers( 0, 1, &g_pNullBuffer, &g_iNullUINT, &g_iNullUINT );
    pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );

    // Draw the mesh
    pd3dImmediateContext->Draw( g_iNumParticles, 0 );

    // Unset the particles buffer
    pd3dImmediateContext->VSSetShaderResources( 0, 1, &g_pNullSRV );
    pd3dImmediateContext->VSSetShaderResources( 1, 1, &g_pNullSRV );
}

//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext )
{
    // Clear the render target and depth stencil
    float ClearColor[4] = { 0.05f, 0.05f, 0.05f, 0.0f };
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    SimulateFluid( pd3dImmediateContext, fElapsedTime );
    RenderFluid( pd3dImmediateContext, fElapsedTime );

    // Render the HUD
    g_HUD.OnRender( fElapsedTime );
    RenderText();
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{

    SAFE_DELETE( g_fluid_simulation );

    g_DialogResourceManager.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );

    SAFE_RELEASE( g_pcbRenderConstants );

    SAFE_RELEASE( g_pParticleVS );
    SAFE_RELEASE( g_pParticleGS );
    SAFE_RELEASE( g_pParticlePS );

    SAFE_RELEASE( g_pParticles );
    SAFE_RELEASE( g_pParticlesSRV );

    SAFE_RELEASE( g_pParticleDensity );
    SAFE_RELEASE( g_pParticleDensitySRV );
}
