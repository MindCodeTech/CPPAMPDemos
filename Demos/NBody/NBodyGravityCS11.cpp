//--------------------------------------------------------------------------------------
// File: NBodyGravityCS11.cpp
//
// Demonstrates how to use C++ AMP to do n-body gravity computation
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include <commdlg.h>
#include "resource.h"

using namespace DirectX;
using namespace DXUT;

#define MAX_GPU_PARTICLES           (128*1024)       // the maximum number of particles in the n-body simulation
#define MAX_CPU_PARTICLES           10240           // the maximum number of particles in the CPU n-body simulation
#define DEFAULT_CPU_PARTICLES       4096            // the default number of particles in the CPU n-body simulation
#define PARTICLES_STEP_SIZE         256             // the number of particles added for each slider tick

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

CDXUTDialogResourceManager          g_DialogResourceManager;    // manager for shared resources of dialogs
CModelViewerCamera                  g_Camera;                   // A model viewing camera
CD3DSettingsDlg                     g_D3DSettingsDlg;           // Device settings dialog
CDXUTDialog                         g_HUD;                      // dialog for standard controls
CDXUTDialog                         g_SampleUI;                 // dialog for sample specific controls
CDXUTTextHelper*                    g_pTxtHelper = nullptr;

ID3D11VertexShader*                 g_pRenderParticlesVS = nullptr;
ID3D11GeometryShader*               g_pRenderParticlesGS = nullptr;
ID3D11PixelShader*                  g_pRenderParticlesPS = nullptr;
ID3D11SamplerState*                 g_pSampleStateLinear = nullptr;
ID3D11BlendState*                   g_pBlendingStateParticle = nullptr;
ID3D11DepthStencilState*            g_pDepthStencilState = nullptr;

ID3D11Buffer*                       g_pParticlePosVelo0 = nullptr;
ID3D11Buffer*                       g_pParticlePosVelo1 = nullptr;
ID3D11ShaderResourceView*           g_pParticlePosVeloRV0 = nullptr;
ID3D11ShaderResourceView*           g_pParticlePosVeloRV1 = nullptr;
ID3D11UnorderedAccessView*          g_pParticlePosVeloUAV0 = nullptr;
ID3D11UnorderedAccessView*          g_pParticlePosVeloUAV1 = nullptr;

ID3D11Buffer*                       g_pParticleBuffer = nullptr;
ID3D11InputLayout*                  g_pParticleVertexLayout = nullptr;

ID3D11Buffer*                       g_pcbGS = nullptr;
ID3D11Buffer*                       g_pcbColor = nullptr;

ID3D11ShaderResourceView*           g_pParticleTexRV = nullptr;

float								g_fSpread = 400.0f;

struct PARTICLE_VERTEX
{
    MHCOLOR Color;
};       

struct CB_GS
{
    MHMATRIX m_WorldViewProj;
    MHMATRIX m_InvView;
    MHCOLOR m_Color;            // color value for changing particles color
};

struct CB_CS
{
    UINT param[4];
    float paramf[4];
};

//--------------------------------------------------------------------------------------
// Nbody functionality 
//--------------------------------------------------------------------------------------
#include "nbody.h"

unsigned int                        g_nNumParticles = DEFAULT_CPU_PARTICLES;  // the current number of particles in the n-body simulation
unsigned int                        g_nNumDevices = 0;
MHCOLOR								g_ParticleColor;
nbody*								g_pnbody_instance = nullptr;

particle                            g_Particles[MAX_GPU_PARTICLES];
particle							g_ParticlesTemp[MAX_GPU_PARTICLES];
particle*							g_pParticles = g_Particles;
particle*							g_pParticlesTemp = g_ParticlesTemp;


//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4
#define IDC_RESETPARTICLES      5

#define IDC_COMPUTETYPECOMBO    6
#define IDC_NBODIES_LABEL       7
#define IDC_NBODIES_SLIDER      8
#define IDC_NBODIES_TEXT        9
#define IDC_FPS_TEXT            10


// enumerate compute type for selection
enum COMPUTETYPE
{
    CT_CPUSINGLE = 0,
    CT_CPUMULTI,
	CT_GPUSINGLESIMPLE,
    CT_GPUSINGLETILE,
    CT_GPUMULTI,
};

COMPUTETYPE                     g_eComputeType = CT_CPUSINGLE;       // Enumerated compute type

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                         void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                      DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext );

void InitApp();
void RenderText();

//--------------------------------------------------------------------------------------
// Helper function to compile an hlsl shader from file, 
// its binary compiled code is returned
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr;

    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile( str, nullptr, nullptr, szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut, &pErrorBlob );
    if ( FAILED(hr) )
    {
        if ( pErrorBlob != nullptr )
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
        SAFE_RELEASE( pErrorBlob );
        return hr;
    }    

    SAFE_RELEASE( pErrorBlob );

    return hr;
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackFrameMove( OnFrameMove );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );
    
    InitApp();

    //DXUTInit( true, true, L"-forceref" ); // Force create a ref device so that feature level D3D_FEATURE_LEVEL_11_0 is guaranteed
    DXUTInit( true, true );                 // Use this line instead to try to create a hardware device

    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"C++ AMP N-Body Simulation Demo" );
	DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 1280, 800 );
    DXUTMainLoop();                      // Enter into the DXUT render loop

    return DXUTGetExitCode();
}

//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    WCHAR szTemp[256];
    
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    CDXUTComboBox* pComboBox = nullptr;
    g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 23 );
    //g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 23, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 23, VK_F2 );
    g_HUD.AddButton( IDC_RESETPARTICLES, L"Reset particles", 0, iY += 26, 170, 22, VK_F2 );
    
    swprintf_s( szTemp, L"Bodies: %d", g_nNumParticles );
    g_HUD.AddStatic( IDC_NBODIES_LABEL, szTemp, -40, iY += 34, 125, 22 );
    g_HUD.AddSlider( IDC_NBODIES_SLIDER, -40, iY += 34, 200, 22, 1, MAX_GPU_PARTICLES/ (PARTICLES_STEP_SIZE * (g_nNumDevices ? g_nNumDevices : 1)) );
    g_HUD.AddComboBox( IDC_COMPUTETYPECOMBO, -40, iY += 34, 210, 26, L'G', false, &pComboBox );
    
    
    if( pComboBox )
    {
        pComboBox->AddItem( L"CPU Single Core", ( void* )CT_CPUSINGLE );
        pComboBox->AddItem( L"CPU Multi Core", ( void* )CT_CPUMULTI );
		pComboBox->AddItem( L"C++ AMP Simple Model", ( void* )CT_GPUSINGLESIMPLE );
        pComboBox->AddItem( L"C++ AMP Tiled Model", ( void* )CT_GPUSINGLETILE );
        pComboBox->AddItem( L"GPU Multi Device", ( void* )CT_GPUMULTI );
    }
    
    g_HUD.GetSlider( IDC_NBODIES_SLIDER )->SetValue( (DEFAULT_CPU_PARTICLES / (PARTICLES_STEP_SIZE *(g_nNumDevices ? g_nNumDevices : 1))) );
    g_HUD.GetComboBox( IDC_COMPUTETYPECOMBO )->SetSelectedByData( ( void* )CT_CPUSINGLE );
    g_nNumParticles = DEFAULT_CPU_PARTICLES;
    g_eComputeType = CT_CPUSINGLE;
    
    g_SampleUI.SetCallback( OnGUIEvent );
}

HRESULT CreateParticleBuffer( ID3D11Device* pd3dDevice )
{
    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC vbdesc =
    {
        MAX_GPU_PARTICLES * sizeof( PARTICLE_VERTEX ),
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_VERTEX_BUFFER,
        0,
        0
    };
    D3D11_SUBRESOURCE_DATA vbInitData;
    ZeroMemory( &vbInitData, sizeof( D3D11_SUBRESOURCE_DATA ) );

    PARTICLE_VERTEX* pVertices = new PARTICLE_VERTEX[ MAX_GPU_PARTICLES ];
    if( !pVertices )
        return E_OUTOFMEMORY;

    for( UINT i = 0; i < MAX_GPU_PARTICLES; i++ )
    {
        pVertices[i].Color = MHCOLOR( 1, 1, 0.2f, 1 );
    }

    vbInitData.pSysMem = pVertices;
    V_RETURN( pd3dDevice->CreateBuffer( &vbdesc, &vbInitData, &g_pParticleBuffer ) );
    SAFE_DELETE_ARRAY( pVertices );

    return hr;
}

//--------------------------------------------------------------------------------------

float
normalize(MHVECTOR3& vector)
{
    float dist = sqrtf(vector.x*vector.x + vector.y*vector.y + vector.z*vector.z);
    if (dist > 1e-6)
    {
        vector.x /= dist;
        vector.y /= dist;
        vector.z /= dist;
    }
    return dist;
}

HRESULT CreateParticlePosVeloBuffers( ID3D11Device* pd3dDevice )
{
    HRESULT hr = S_OK;

    // Initialize the data in the buffers
    if( !g_pParticles )
        return E_OUTOFMEMORY;    

    srand( GetTickCount() );   

    // Disk Galaxy Formation
    float fCenterSpread = g_fSpread * 0.50f;
    for(size_t i=0; i<MAX_GPU_PARTICLES; i += PARTICLES_STEP_SIZE)
    {
		::g_pnbody_instance->load_particles( &g_pParticles[i],
            float_3( fCenterSpread, 0, 0 ), float_4( 0, 0, -20, 1/10000.0f / 10000.0f ),
            g_fSpread, PARTICLES_STEP_SIZE / 2 );
        ::g_pnbody_instance->load_particles( &g_pParticles[i + PARTICLES_STEP_SIZE / 2],
            float_3( -fCenterSpread, 0, 0 ), float_4( 0, 0, 20, 1/10000.0f / 10000.0f ),
            g_fSpread, (PARTICLES_STEP_SIZE + 1) / 2 );
    }

    ::g_pnbody_instance->create_accelerators((int&)g_nNumDevices);
    g_HUD.GetSlider( IDC_NBODIES_SLIDER )->SetRange(1, MAX_GPU_PARTICLES/ (PARTICLES_STEP_SIZE * (g_nNumDevices ? g_nNumDevices : 1)));
    ::g_pnbody_instance->create_compute_buffers(g_pParticles, MAX_GPU_PARTICLES);
     
    D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
    ZeroMemory( &DescRV, sizeof( DescRV ) );
    DescRV.Format = DXGI_FORMAT_R32_TYPELESS;
    DescRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    DescRV.BufferEx.FirstElement = 0;
    DescRV.BufferEx.NumElements = (MAX_GPU_PARTICLES * sizeof(particle)) / sizeof(float);
    DescRV.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pParticlePosVelo0, &DescRV, &g_pParticlePosVeloRV0 ) );
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pParticlePosVelo1, &DescRV, &g_pParticlePosVeloRV1 ) );

    D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
    ZeroMemory( &DescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
    DescUAV.Format = DXGI_FORMAT_R32_TYPELESS;
    DescUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    DescUAV.Buffer.FirstElement = 0;
    DescUAV.Buffer.NumElements = (MAX_GPU_PARTICLES * sizeof(particle)) / sizeof(float);
    DescUAV.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
    V_RETURN( pd3dDevice->CreateUnorderedAccessView( g_pParticlePosVelo0, &DescUAV, &g_pParticlePosVeloUAV0 ) );
    V_RETURN( pd3dDevice->CreateUnorderedAccessView( g_pParticlePosVelo1, &DescUAV, &g_pParticlePosVeloUAV1 ) );

    return hr;
}

bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    //assert( pDeviceSettings->ver == DXUT_D3D11_DEVICE );

    // Disable vsync
    pDeviceSettings->d3d11.SyncInterval = 0;
    g_D3DSettingsDlg.GetDialogControl()->GetComboBox( DXUTSETTINGSDLG_D3D11_PRESENT_INTERVAL )->SetEnabled( false );    

    // For the first device created if it is a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning();
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
   switch( g_eComputeType )
    {
    case CT_CPUSINGLE:
        {
            g_ParticleColor = MHCOLOR( 1.0f, 0.05f, 0.05f, 1.0f );
            ::g_pnbody_instance->cpu_single_core( g_pParticles, g_pParticlesTemp, g_nNumParticles);
            std::swap(g_pParticles, g_pParticlesTemp);
        }
        break;
    case CT_CPUMULTI:
        {
            g_ParticleColor = MHCOLOR( 0.8f, 0.0f, 0.0f, 1.0f );
            ::g_pnbody_instance->cpu_ppl( g_pParticles, g_pParticlesTemp, g_nNumParticles);
            std::swap(g_pParticles, g_pParticlesTemp);
        }
        break;

    case CT_GPUSINGLESIMPLE:
        {
            g_ParticleColor = MHCOLOR( 0.05f, 1.0f, 0.05f, 1.0f );
            ::g_pnbody_instance->amp_single_gpu( g_nNumParticles, false );
        }
        break;
    case CT_GPUSINGLETILE:
        {
            g_ParticleColor = MHCOLOR( 0.05f, 1.0f, 0.05f, 1.0f );
            ::g_pnbody_instance->amp_single_gpu( g_nNumParticles, true );
        }
        break;

    case CT_GPUMULTI:
        {
            g_ParticleColor = MHCOLOR( 0.05f, 0.05f, 1.0f, 1.0f );
            ::g_pnbody_instance->amp_multi_gpu( g_pParticles, g_nNumParticles);
        }
        break;
    }
    
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
}

//--------------------------------------------------------------------------------------
// Before handling window messages, DXUT passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then DXUT will not process this message.
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                         void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    CDXUTComboBox* pComboBox = nullptr;
    CDXUTSlider* pSlider = nullptr;
    WCHAR szTemp[256];
    
    switch( nControlID )
    {
    case IDC_TOGGLEFULLSCREEN:
        DXUTToggleFullScreen(); break;
    //case IDC_TOGGLEREF:
    //    DXUTToggleREF(); break;
    case IDC_CHANGEDEVICE:
        g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() ); break;

    case IDC_RESETPARTICLES:
        {
            SAFE_RELEASE(g_pParticlePosVelo0);
            SAFE_RELEASE(g_pParticlePosVelo1);
            SAFE_RELEASE(g_pParticlePosVeloRV0);
            SAFE_RELEASE(g_pParticlePosVeloRV1);
            SAFE_RELEASE(g_pParticlePosVeloUAV0);
            SAFE_RELEASE(g_pParticlePosVeloUAV1);
            if (g_pnbody_instance) 
            {
                g_pnbody_instance->release();
            }
            CreateParticlePosVeloBuffers(DXUTGetD3D11Device());
            break;
        }        
    case IDC_COMPUTETYPECOMBO:
        {
            pComboBox = ( CDXUTComboBox* )pControl;
            g_eComputeType = ( COMPUTETYPE )( INT_PTR )pComboBox->GetSelectedData();
            
            if(g_nNumParticles > MAX_CPU_PARTICLES && (g_eComputeType == CT_CPUSINGLE || g_eComputeType == CT_CPUMULTI))
            {
                // cap the number of particles in the CPU version
                g_nNumParticles = MAX_CPU_PARTICLES;
                g_HUD.GetSlider( IDC_NBODIES_SLIDER )->SetValue( MAX_CPU_PARTICLES / (PARTICLES_STEP_SIZE * g_nNumDevices) );
            }

			swprintf_s( szTemp, L"Bodies: %d", g_nNumParticles );    
            g_HUD.GetStatic( IDC_NBODIES_LABEL )->SetText( szTemp );
        }
        break;
        
    case IDC_NBODIES_SLIDER:
        {
            pSlider = ( CDXUTSlider* )pControl;

            if(g_nNumParticles > MAX_CPU_PARTICLES && (g_eComputeType == CT_CPUSINGLE || g_eComputeType == CT_CPUMULTI))
            {
                // cap the number of particles in the CPU version
                g_nNumParticles = MAX_CPU_PARTICLES;
                g_HUD.GetSlider( IDC_NBODIES_SLIDER )->SetValue( MAX_CPU_PARTICLES / (PARTICLES_STEP_SIZE  * g_nNumDevices));
            }
            else
            {
                g_nNumParticles = pSlider->GetValue() * PARTICLES_STEP_SIZE * g_nNumDevices;
            }

            swprintf_s( szTemp, L"Bodies: %d", g_nNumParticles );    
            g_HUD.GetStatic( IDC_NBODIES_LABEL )->SetText( szTemp );
        }
        break;
    }
}

bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                      DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    // reject any device which doesn't support CS4x
    if ( DeviceInfo->ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x == FALSE )
        return false;
    
    return true;
}

//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// created, which will happen during application initialization and windowed/full screen 
// toggles. This is the best location to create D3DPOOL_MANAGED resources since these 
// resources need to be reloaded whenever the device is destroyed. Resources created  
// here should be released in the OnD3D11DestroyDevice callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
    HRESULT hr;
    
    D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS ho;
    V_RETURN( pd3dDevice->CheckFeatureSupport( D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &ho, sizeof(ho) ) );

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

    ID3DBlob* pBlobRenderParticlesVS = nullptr;
    ID3DBlob* pBlobRenderParticlesGS = nullptr;
    ID3DBlob* pBlobRenderParticlesPS = nullptr;

    // Create the shaders
    V_RETURN( CompileShaderFromFile( L"\\..\\Media\\Shaders\\ParticleDraw.hlsl", "VSParticleDraw", "vs_4_0", &pBlobRenderParticlesVS ) );
    V_RETURN( CompileShaderFromFile( L"\\..\\Media\\Shaders\\ParticleDraw.hlsl", "GSParticleDraw", "gs_4_0", &pBlobRenderParticlesGS ) );
    V_RETURN( CompileShaderFromFile( L"\\..\\Media\\Shaders\\ParticleDraw.hlsl", "PSParticleDraw", "ps_4_0", &pBlobRenderParticlesPS ) );

    V_RETURN( pd3dDevice->CreateVertexShader( pBlobRenderParticlesVS->GetBufferPointer(), pBlobRenderParticlesVS->GetBufferSize(), nullptr, &g_pRenderParticlesVS ) );
    V_RETURN( pd3dDevice->CreateGeometryShader( pBlobRenderParticlesGS->GetBufferPointer(), pBlobRenderParticlesGS->GetBufferSize(), nullptr, &g_pRenderParticlesGS ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pBlobRenderParticlesPS->GetBufferPointer(), pBlobRenderParticlesPS->GetBufferSize(), nullptr, &g_pRenderParticlesPS ) );

    // Create our vertex input layout
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    V_RETURN( pd3dDevice->CreateInputLayout( layout, sizeof( layout ) / sizeof( layout[0] ),
        pBlobRenderParticlesVS->GetBufferPointer(), pBlobRenderParticlesVS->GetBufferSize(), &g_pParticleVertexLayout ) );

    SAFE_RELEASE( pBlobRenderParticlesVS );
    SAFE_RELEASE( pBlobRenderParticlesGS );
    SAFE_RELEASE( pBlobRenderParticlesPS );

	// Create NBody object
	g_pnbody_instance = new nbody(pd3dDevice);
	g_pnbody_instance->verify_SSE_implementation();

    V_RETURN( CreateParticleBuffer( pd3dDevice ) );
    V_RETURN( CreateParticlePosVeloBuffers(pd3dDevice) );

    // Setup constant buffer
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = sizeof( CB_GS );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, nullptr, &g_pcbGS ) );

    // Load the Particle Texture
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"\\..\\Media\\Textures\\Particle.dds" ) );
    V_RETURN( DXUTCreateShaderResourceViewFromFile( pd3dDevice, str, &g_pParticleTexRV ) );

    D3D11_SAMPLER_DESC SamplerDesc;
    ZeroMemory( &SamplerDesc, sizeof(SamplerDesc) );
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    V_RETURN( pd3dDevice->CreateSamplerState( &SamplerDesc, &g_pSampleStateLinear ) );

    D3D11_BLEND_DESC BlendStateDesc;
    ZeroMemory( &BlendStateDesc, sizeof(BlendStateDesc) );
    BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
    BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
    V_RETURN( pd3dDevice->CreateBlendState( &BlendStateDesc, &g_pBlendingStateParticle ) );

    D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
    ZeroMemory( &DepthStencilDesc, sizeof(DepthStencilDesc) );
    DepthStencilDesc.DepthEnable = FALSE;
    DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    pd3dDevice->CreateDepthStencilState( &DepthStencilDesc, &g_pDepthStencilState );

    // Setup the camera's view parameters
    MHVECTOR3 vecEye( -g_fSpread * 2, g_fSpread * 4, -g_fSpread * 3 );
    MHVECTOR3 vecAt ( 0.0f, 0.0f, 0.0f );
    g_Camera.SetViewParams( vecEye, vecAt );

    return S_OK;
}

HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( XM_PI / 4, fAspectRatio, 10.0f, 500000.0f );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( 0, MOUSE_WHEEL, MOUSE_LEFT_BUTTON | MOUSE_MIDDLE_BUTTON | MOUSE_RIGHT_BUTTON );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300 );
    g_SampleUI.SetSize( 170, 300 );

    return hr;
}

void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();    
}

void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( MHCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
    g_pTxtHelper->SetInsertionPos( 20, 60 );
    g_pTxtHelper->DrawFormattedTextLine( L"Bodies: %d", g_nNumParticles );
    g_pTxtHelper->DrawFormattedTextLine( L"FPS:    %.2f", DXUTGetFPS() );

    float gflops = (g_nNumParticles / 1000.0f) * (g_nNumParticles / 1000.0f) * DXUTGetFPS() * 20 / 1000.0f;
    WCHAR strFlops[128];
    swprintf_s( strFlops, 128, L"GFlops: %f ", gflops );
    g_pTxtHelper->DrawTextLine( strFlops );

    g_pTxtHelper->End();
}

//--------------------------------------------------------------------------------------
bool RenderParticles( ID3D11DeviceContext* pd3dImmediateContext, MHMATRIX& mView, MHMATRIX& mProj )
{
    MHMATRIX mWorldViewProj;
    // copy in particle position and velocity values
	if (g_eComputeType == CT_CPUSINGLE || g_eComputeType == CT_CPUMULTI)
	{
		UINT size = (UINT)(g_nNumParticles * sizeof(particle));
		D3D11_BOX box;   
		box.left = 0;
		box.top = 0;
		box.front = 0;
		box.right = size;
		box.bottom = 1;
		box.back = 1;
		pd3dImmediateContext->UpdateSubresource(g_pParticlePosVelo0, 0, &box, g_pParticles, size, 0);
	}

    ID3D11BlendState *pBlendState0 = nullptr;
    ID3D11DepthStencilState *pDepthStencilState0 = nullptr;
    UINT SampleMask0, StencilRef0;
    MHVECTOR4 BlendFactor0;
    pd3dImmediateContext->OMGetBlendState( &pBlendState0, &BlendFactor0.x, &SampleMask0 );
    pd3dImmediateContext->OMGetDepthStencilState( &pDepthStencilState0, &StencilRef0 );

    pd3dImmediateContext->VSSetShader( g_pRenderParticlesVS, nullptr, 0 );
    pd3dImmediateContext->GSSetShader( g_pRenderParticlesGS, nullptr, 0 );
    pd3dImmediateContext->PSSetShader( g_pRenderParticlesPS, nullptr, 0 );
    
    pd3dImmediateContext->IASetInputLayout( g_pParticleVertexLayout );

    // Set IA parameters
    ID3D11Buffer* pBuffers[1] = { g_pParticleBuffer };
    UINT stride[1] = { sizeof( PARTICLE_VERTEX ) };
    UINT offset[1] = { 0 };
    pd3dImmediateContext->IASetVertexBuffers( 0, 1, pBuffers, stride, offset );
    pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );

    ID3D11ShaderResourceView* aRViews[ 1 ] = { g_pParticlePosVeloRV0 };
    pd3dImmediateContext->VSSetShaderResources( 0, 1, aRViews );
    
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map( g_pcbGS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
/*  CB_GS* pCBGS = ( CB_GS* )MappedResource.pData; 
    D3DXMatrixMultiply( &pCBGS->m_WorldViewProj, &mView, &mProj );   
    D3DXMatrixInverse( &pCBGS->m_InvView, nullptr, &mView );*/
	auto pCBGS = reinterpret_cast<CB_GS*>( MappedResource.pData);
	XMStoreFloat4x4( &pCBGS->m_WorldViewProj, XMMatrixMultiply(mView, mProj));
	XMStoreFloat4x4( &pCBGS->m_InvView, XMMatrixInverse(nullptr, mView));
    pCBGS->m_Color = g_ParticleColor;
    pd3dImmediateContext->Unmap( g_pcbGS, 0 );
    pd3dImmediateContext->GSSetConstantBuffers( 0, 1, &g_pcbGS );
    
    pd3dImmediateContext->PSSetShaderResources( 0, 1, &g_pParticleTexRV );
    pd3dImmediateContext->PSSetSamplers( 0, 1, &g_pSampleStateLinear );

    pd3dImmediateContext->OMSetBlendState( g_pBlendingStateParticle, MHCOLOR( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF  );
    pd3dImmediateContext->OMSetDepthStencilState( g_pDepthStencilState, 0 );

    pd3dImmediateContext->Draw( g_nNumParticles, 0 );

    ID3D11ShaderResourceView* ppSRVnullptr[1] = { nullptr };
    pd3dImmediateContext->VSSetShaderResources( 0, 1, ppSRVnullptr );
    pd3dImmediateContext->PSSetShaderResources( 0, 1, ppSRVnullptr );

    pd3dImmediateContext->GSSetShader( nullptr, nullptr, 0 );
    pd3dImmediateContext->OMSetBlendState( pBlendState0, &BlendFactor0.x, SampleMask0 ); SAFE_RELEASE(pBlendState0);
    pd3dImmediateContext->OMSetDepthStencilState( pDepthStencilState0, StencilRef0 ); SAFE_RELEASE(pDepthStencilState0);

    return true;
}

void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext )
{
    float ClearColor[4] = { 0.0, 0.0, 0.0, 0.0 };

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }
    
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    MHMATRIX mView;
    MHMATRIX mProj;

    // Get the projection & view matrix from the camera class
    mProj = g_Camera.GetProjMatrix();
    mView = g_Camera.GetViewMatrix();

    // Render the particles
    RenderParticles( pd3dImmediateContext, mView, mProj );

    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    g_HUD.OnRender( fElapsedTime );
    g_SampleUI.OnRender( fElapsedTime );
    RenderText();
    DXUT_EndPerfEvent();

    // The following could be used to output fps stats into debug output window,
    // which is useful because you can then turn off all UI rendering as they cloud performance
    /*static DWORD dwTimefirst = GetTickCount();
    if ( GetTickCount() - dwTimefirst > 5000 )
    {    
        OutputDebugString( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
        OutputDebugString( L"\n" );
        dwTimefirst = GetTickCount();
    }*/
}

//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// been destroyed, which generally happens as a result of application termination or 
// windowed/full screen toggles. Resources created in the OnD3D11CreateDevice callback 
// should be released here, which generally includes all D3DPOOL_MANAGED resources. 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );

    SAFE_RELEASE( g_pParticleBuffer ); 
    SAFE_RELEASE( g_pParticleVertexLayout );
        
    SAFE_RELEASE( g_pParticlePosVelo0 );
    SAFE_RELEASE( g_pParticlePosVelo1 );
    SAFE_RELEASE( g_pParticlePosVeloRV0 );
    SAFE_RELEASE( g_pParticlePosVeloRV1 );
    SAFE_RELEASE( g_pParticlePosVeloUAV0 );
    SAFE_RELEASE( g_pParticlePosVeloUAV1 );

    SAFE_RELEASE( g_pcbGS );
    SAFE_RELEASE( g_pcbColor );

    SAFE_RELEASE( g_pParticleTexRV );

    SAFE_RELEASE( g_pRenderParticlesVS );
    SAFE_RELEASE( g_pRenderParticlesGS );
    SAFE_RELEASE( g_pRenderParticlesPS );
    SAFE_RELEASE( g_pSampleStateLinear );
    SAFE_RELEASE( g_pBlendingStateParticle );
    SAFE_RELEASE( g_pDepthStencilState );

	if (g_pnbody_instance) 
	{
		g_pnbody_instance->release();
		delete g_pnbody_instance;
	}
}
