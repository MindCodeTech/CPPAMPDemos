//--------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corp. 
//
// File: ImageEffects.cpp
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this 
// file except in compliance with the License. You may obtain a copy of the License at 
// http://www.apache.org/licenses/LICENSE-2.0  
//  
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, 
// EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR 
// CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT. 
//  
// See the Apache Version 2.0 License for specific language governing permissions and 
// limitations under the License.
//--------------------------------------------------------------------------------------
#include <SDKDDKVer.h>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include "ImageEffects.h"
#define MAX_LOADSTRING 100

// Forward declarations of functions included in this code module:
ATOM            MyRegisterClass(HINSTANCE hInstance);
BOOL            InitInstance(HINSTANCE, int);
LRESULT CALLBACK   WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK   About(HWND, UINT, WPARAM, LPARAM);
void                InitDevice();
void                Cleanup();
void                OpenImageFile();
void                LoadImage();
void                ShowImage(bool original = true);
void                UpdateEffectsSubMenu(image_state previous, image_state current);
image_state         MapFromMsgIdToImageState(UINT id);
UINT                MapFromImageStateToMsgId(image_state id);
void                ApplyEffects(image_state state);

// global variables
HINSTANCE hInst;                             // current instance
TCHAR szTitle[MAX_LOADSTRING];               // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];         // the main window class name
HWND                                g_hWnd = nullptr;
ID3D11Device*                       g_pd3dDevice = nullptr;
ID3D11DeviceContext*                g_pImmediateContext = nullptr ;
IDXGISwapChain*                     g_pSwapChain = nullptr;
ID3D11VertexShader*                 g_pVertexShader = nullptr;
ID3D11PixelShader*                  g_pPixelShader = nullptr;
ID3D11InputLayout*                  g_pVertexLayout = nullptr;
ID3D11SamplerState*                 g_pSampler = nullptr;
ID3D11Buffer *                      g_pVertexBuffer = nullptr;
ID3D11Texture2D*                    g_pInputTexture = nullptr;
ID3D11ShaderResourceView*           g_pInputTextureSRV = nullptr;
ID3D11ShaderResourceView*           g_pProcessedTextureSRV = nullptr;
texture<unorm4, 2>*                 g_pAmpProcessedTexture = nullptr;
accelerator_view                    g_av = accelerator().default_view;
std::wstring                        g_file;
image_state                         g_state = image_state_none;
bool                                g_allow_stretch = false;
static const UINT                   g_effects_sub_menu_pos = 1;
static const UINT                   g_options_sub_menu_pos = 2;
static const UINT                   g_effects_first = IDM_EFFECTS_ORIGINAL;
static const UINT                   g_effects_last  = IDM_EFFECTS_MEAN_REMOVAL;

// Main function
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    MSG msg = {0};
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_IMAGE_EFFECTS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) return FALSE;
    try {
        InitDevice();
        ShowWindow(g_hWnd, nCmdShow);
        UpdateWindow(g_hWnd);
        hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_IMAGE_EFFECTS));
        // Load and display the default image
        std::ifstream image(g_file);
        if (!image.good()) OpenImageFile();
        image.close();
        LoadImage();
        ShowImage();
        g_state = image_state_original;
        UpdateEffectsSubMenu(image_state_none, g_state);
        // Main message loop:
        while (GetMessage(&msg, nullptr, 0, 0)) {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    catch (std::exception & e) {
        MessageBoxA(nullptr, e.what(), "Error", MB_OK);
    }
    Cleanup();
    return (int) msg.wParam;
}

//  Registers the window class.
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra      = 0;
    wcex.cbWndExtra      = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDC_IMAGE_EFFECTS));
    wcex.hCursor      = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground   = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_IMAGE_EFFECTS);
    wcex.lpszClassName   = szWindowClass;
    wcex.hIconSm      = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassEx(&wcex);
}

//  Saves instance handle and creates main window
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable
    RECT rc = {0, 0, (LONG)::GetSystemMetrics( SM_CXSCREEN ), (LONG)::GetSystemMetrics( SM_CYSCREEN )};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, 0, 0, 
                          rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
    if (!g_hWnd) return FALSE;
    return TRUE;
}

//  Processes messages for the main window.
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    switch (message) {
    case WM_COMMAND:
        {
            wmId    = LOWORD(wParam);
            wmEvent = HIWORD(wParam);
            // Parse the menu selections:
            switch (wmId) {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_OPEN: // open a new image file
                OpenImageFile();
                break;
                // apply effects
            case IDM_EFFECTS_ORIGINAL:
            case IDM_EFFECTS_SOBELEDGEDETECTION:
            case IDM_EFFECTS_PREWITTEDGEDETECTION:
            case IDM_EFFECTS_SCHARREDGEDETECTION:
            case IDM_EFFECTS_EMBOSSING:
            case IDM_EFFECTS_GAUSSIAN_BLUR:
            case IDM_EFFECTS_SHARPENING:
            case IDM_EFFECTS_MEAN_REMOVAL:
                {
                    image_state curr = MapFromMsgIdToImageState(wmId);
                    if (curr != g_state) { // apply the effect
                        ApplyEffects(curr);
                        UpdateEffectsSubMenu(g_state, curr);
                        g_state = curr;
                    }
                }
                break;
            case IDM_OPTIONS_STRETCH:
                {
                    g_allow_stretch = !g_allow_stretch;
                    HMENU menu = GetSubMenu(GetMenu(g_hWnd), g_options_sub_menu_pos);
                    CheckMenuItem(menu, IDM_OPTIONS_STRETCH, g_allow_stretch ? MF_CHECKED : MF_UNCHECKED );
                    ApplyEffects(g_state);
                }
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) ApplyEffects(g_state);
        break;
    case WM_KEYDOWN:
        {
            // check if the key stroke is used to apply an image effect
            image_state prev = g_state;
            switch (wParam) {
                case 'O':
                    g_state = image_state_original; break;
                case 'P':
                    g_state = image_state_prewitt; break;
                case 'S':
                    g_state = image_state_sobel; break;
                case 'C':
                    g_state = image_state_scharr; break;
                case 'E':
                    g_state = image_state_emboss; break;
                case 'G':
                    g_state = image_state_gaussian; break;
                case 'H':
                    g_state = image_state_sharpen; break;
                case 'M':
                    g_state = image_state_mean_removal; break;
                case 'Q': // exit
                    PostQuitMessage(0); break;
            }
            if (prev != g_state) {  // apply the effect
                ApplyEffects(g_state);
                UpdateEffectsSubMenu(prev, g_state);
            }
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Runtime error
void CheckRuntimeError(HRESULT hr, const char * msg)
{
    if (FAILED(hr)) throw std::runtime_error(msg);
}

// Read shader in binary
std::unique_ptr<char[]> ReadShader(const char * file, size_t & length)
{
    std::ifstream ifstream(file, std::ios::binary);
    if (!ifstream.good()) CheckRuntimeError(E_FAIL, "Failed to open shader file");
    ifstream.seekg(0, std::ios::end);
    length = static_cast<size_t>(ifstream.tellg());
    ifstream.seekg(0, std::ios::beg);
    std::unique_ptr<char[]> p(new char[length]);
    ifstream.read(p.get(), length);
    ifstream.close();
    return p;
}

// Create Direct3D device and swap chain
// Create D3D objects that can be reused
void InitDevice()
{
    HRESULT hr = S_OK;
    RECT rc;
    GetClientRect( g_hWnd, &rc );
    unsigned int width = rc.right - rc.left;
    unsigned int height = rc.bottom - rc.top;
    // Initialize COM (needed for WIC)
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    CheckRuntimeError(hr, "Failed to initialize COM");
    // default image
    g_file = L"default.jpg";
    unsigned int createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    // create swap chain
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
    // try dx 11
    D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS | DXGI_USAGE_SHADER_INPUT;
    // Use default
    hr = D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, &FeatureLevel, 1,  D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pImmediateContext ); 
	if (FAILED(hr))
	{
		// no DX11 device, try WARP fallback.  WARP does not get selected automatically on Windows 7.
		hr = D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, &FeatureLevel, 1,  D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pImmediateContext ); 
	    CheckRuntimeError(hr, "Failed to create D3D11 Device and Swap Chain with D3D_DRIVER_TYPE_HARDWARE and D3D_DRIVER_TYPE_WARP");
	}

    // init the accelerator_view
    g_av = create_accelerator_view(reinterpret_cast<IUnknown *>(g_pd3dDevice));
    size_t length;
    // Compile the vertex shader
    auto shader = ReadShader("vs.cso", length);
    // Create the vertex shader
    hr = g_pd3dDevice->CreateVertexShader(shader.get(), length, nullptr, &g_pVertexShader );
    CheckRuntimeError(hr, "Failed to Create Vertex Shader");
    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE( layout );
    // Create the input layout
    hr = g_pd3dDevice->CreateInputLayout( layout, numElements, shader.get(), length, &g_pVertexLayout );
    CheckRuntimeError(hr, "Failed to Create Input Layout");
    // Set the input layout
    g_pImmediateContext->IASetInputLayout( g_pVertexLayout );
    // Compile the pixel shader
    shader = ReadShader("ps.cso", length);
    // Create the pixel shader
    hr = g_pd3dDevice->CreatePixelShader(shader.get(), length, nullptr, &g_pPixelShader );
    CheckRuntimeError(hr, "Failed to create pixel shader");
    // Create the sample state
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory( &sampDesc, sizeof(sampDesc) );
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_pd3dDevice->CreateSamplerState( &sampDesc, &g_pSampler);
    CheckRuntimeError(hr, "Failed to create sampler state");
    // Create vertex buffer
    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(simple_vertex) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pVertexBuffer);
    CheckRuntimeError(hr, "Failed to create vertex buffer");
    // Set vertex buffer
    UINT stride = sizeof(simple_vertex);
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );
    // Set primitive topology
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
}

// Load the image from the file to a D3D texture, and also create the corresponding amp texture via interop
void LoadImage()
{
    HRESULT hr = S_OK;
    // Load from file
    if (g_pInputTexture) {
        assert(g_pInputTextureSRV);
        g_pInputTextureSRV->Release();
        g_pInputTexture->Release();
    }
    // create D3D texture that stores the original pictures
    hr = CreateWICTextureFromFile(g_pd3dDevice, g_pImmediateContext, g_file.c_str(), reinterpret_cast<ID3D11Resource **>(&g_pInputTexture), &g_pInputTextureSRV);
    CheckRuntimeError(hr, "Failed to create the input texture");
    if (g_pAmpProcessedTexture) {
        assert(g_pProcessedTextureSRV);
        delete g_pAmpProcessedTexture;
        g_pProcessedTextureSRV->Release();
    }
    D3D11_TEXTURE2D_DESC input_tex_desc;
    g_pInputTexture->GetDesc(&input_tex_desc);
    UINT img_width = input_tex_desc.Width;
    UINT img_height = input_tex_desc.Height;
    // Create a texture the same size as g_pInputTexture, it's used to store the effect applied texture
    g_pAmpProcessedTexture = new texture<unorm4, 2>(static_cast<int>(img_height), static_cast<int>(img_width), 8U, g_av);
    // Get the D3D texture from interop, and create the corresponding SRV that could be later bound to graphics pipeline
    ID3D11Texture2D * processedTexture = reinterpret_cast<ID3D11Texture2D *>(get_texture<unorm4, 2>(*g_pAmpProcessedTexture));

    // Define how the Direct3D typeless format should be interpreted.  The C++ AMP texture, g_pAmpProcessedTexture, used 
    // DXGI_FROMAT_R8G8B8A8_TYPELESS as its underlyinhg format.  However, the typeless format cannot be used to create an SRV.
    D3D11_SHADER_RESOURCE_VIEW_DESC shader_view_desc;
    shader_view_desc.Format = input_tex_desc.Format;
    shader_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shader_view_desc.Texture2D.MipLevels = -1;
    shader_view_desc.Texture2D.MostDetailedMip = 0;
    hr = g_pd3dDevice->CreateShaderResourceView(processedTexture, &shader_view_desc, &g_pProcessedTextureSRV);
    CheckRuntimeError(hr, "Failed to create the proccessed texture shader resource view");
    processedTexture->Release();
}

// Clean up all the resources
void Cleanup()
{
    if (g_pSwapChain) {
        g_pSwapChain->SetFullscreenState(false, nullptr);
        g_pSwapChain->Release();
    }
    if (g_pVertexBuffer) g_pVertexBuffer->Release();
    if (g_pVertexShader) g_pVertexShader->Release();
    if (g_pVertexLayout) g_pVertexLayout->Release();
    if (g_pPixelShader) g_pPixelShader->Release();
    if (g_pSampler) g_pSampler->Release();
    if (g_pInputTexture) g_pInputTexture->Release();
    if (g_pInputTextureSRV) g_pInputTextureSRV->Release();
    if (g_pAmpProcessedTexture) delete g_pAmpProcessedTexture;
    if (g_pProcessedTextureSRV) g_pProcessedTextureSRV->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
    CoUninitialize();
}

// Display the image 
void ShowImage(bool original)
{
    HRESULT hr = S_OK;
    if(g_pInputTexture == nullptr) return;
    D3D11_TEXTURE2D_DESC input_tex_desc;
    g_pInputTexture->GetDesc(&input_tex_desc);
    size_t img_width = input_tex_desc.Width;
    size_t img_height = input_tex_desc.Height;
    float ratio = (float)img_width / (float)img_height;
    RECT rc;
    GetClientRect( g_hWnd, &rc );
    size_t wd_width = rc.right - rc.left;
    size_t wd_height = rc.bottom - rc.top;
    if (g_allow_stretch) {// if we allow stretch the image, fill the window, do not keep the aspect ratio
        img_height = wd_height;
        img_width = wd_width;
    } else {
        // resize the image to fit into the window without changing the ratio
        // will only scale-down
        if (img_height > wd_height) {
            img_height = std::min(img_height, wd_height);
            img_width = static_cast<size_t>(static_cast<float>(img_height) * ratio);
        }
        if (img_width > wd_width) {
            img_width =  std::min(img_width, wd_width);
            img_height = static_cast<size_t>(static_cast<float>(img_width) / ratio);
        }
    }
    DXGI_SWAP_CHAIN_DESC sd;
    g_pSwapChain->GetDesc(&sd);
    if (sd.BufferDesc.Width != wd_width || sd.BufferDesc.Height != wd_height) {
        hr = g_pSwapChain->ResizeBuffers(sd.BufferCount, (UINT)wd_width, (UINT)wd_height, sd.BufferDesc.Format, 0);
        CheckRuntimeError(hr, "Failed to resize buffers");
    }
    // Create a render target view using the back buffer
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    CheckRuntimeError(hr, "Failed to get back buffer from swap chain");
    ID3D11RenderTargetView * pRenderTargetView = nullptr;
    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
    pBackBuffer->Release();
    CheckRuntimeError(hr, "Failed to create render target view");
    g_pImmediateContext->OMSetRenderTargets( 1, &pRenderTargetView, nullptr );
    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)wd_width;
    vp.Height = (FLOAT)wd_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );
    // set the vertex buffer with four vetices that describe the shape of the 
    // resized image (without changing the aspect ratio)
    float h_offset = (float)img_height / (float)wd_height;
    float w_offset = (float)img_width / (float)wd_width;
    float d_offset = 0.1f;
    simple_vertex vertices[] =
    {
        { float3( w_offset,  h_offset, d_offset), float2(1.0f, 0.0f) },
        { float3( w_offset, -h_offset, d_offset), float2(1.0f, 1.0f) },
        { float3(-w_offset,  h_offset, d_offset), float2(0.0f, 0.0f) },
        { float3(-w_offset, -h_offset, d_offset), float2(0.0f, 1.0f) },
    };
    g_pImmediateContext->UpdateSubresource(g_pVertexBuffer, 0, nullptr, vertices, 0, 0);
    // Clear the back buffer
    float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // black 
    g_pImmediateContext->ClearRenderTargetView(pRenderTargetView, ClearColor);
    // Render
    g_pImmediateContext->VSSetShader( g_pVertexShader, nullptr, 0 );
    g_pImmediateContext->PSSetShader( g_pPixelShader, nullptr, 0 );
    if (original || g_pProcessedTextureSRV == nullptr) { // the original picture uses g_pInputTextureSRV
        g_pImmediateContext->PSSetShaderResources( 0, 1, &g_pInputTextureSRV );
    } else {  // otherwise uses the g_pProcessedTextureSRV
        g_pImmediateContext->PSSetShaderResources( 0, 1, &g_pProcessedTextureSRV );
    }
    g_pImmediateContext->PSSetSamplers( 0, 1, &g_pSampler);
    g_pImmediateContext->Draw(4, 0); // draw the four vertices
    // Present our back buffer to our front buffer
    g_pSwapChain->Present( 0, 0 );
    // Clean the pipeline
    ID3D11ShaderResourceView * emptySRV = nullptr;
    g_pImmediateContext->PSSetShaderResources(0, 1, &emptySRV);
    if (pRenderTargetView) pRenderTargetView->Release();
}

// Open a image file
void OpenImageFile()
{
    wchar_t szFile[1024];
    OPENFILENAME ofn;
    ZeroMemory( &ofn , sizeof( ofn));
    ofn.lStructSize = sizeof ( ofn );
    ofn.hwndOwner = nullptr  ;
    ofn.lpstrFile = szFile ;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof( szFile ) / sizeof(wchar_t);
    ofn.lpstrFilter = L"JPEG\0*.JPG\0BMP\0*.BMP\0PNG\0*.PNG\0";
    ofn.nFilterIndex =1;
    ofn.lpstrFileTitle = nullptr ;
    ofn.nMaxFileTitle = 0 ;
    ofn.lpstrInitialDir=nullptr ;
    ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;
    if (GetOpenFileName(&ofn)) {
        g_file = szFile;
        if (g_allow_stretch) {
            g_allow_stretch = false;
            HMENU menu = GetSubMenu(GetMenu(g_hWnd), g_options_sub_menu_pos);
            CheckMenuItem(menu, IDM_OPTIONS_STRETCH, MF_UNCHECKED );
        }
        LoadImage();
        ShowImage();
        UpdateEffectsSubMenu(g_state, image_state_original);
        g_state = image_state_original;
    }
}

// Translate from msg Id to image_state
image_state  MapFromMsgIdToImageState(UINT id)
{
    image_state state;
    switch(id) {
    case IDM_EFFECTS_SOBELEDGEDETECTION:
        state = image_state_sobel; break;
    case IDM_EFFECTS_PREWITTEDGEDETECTION:
        state = image_state_prewitt; break;
    case IDM_EFFECTS_SCHARREDGEDETECTION:
        state = image_state_scharr; break;
    case IDM_EFFECTS_EMBOSSING:
        state = image_state_emboss; break;
    case IDM_EFFECTS_GAUSSIAN_BLUR:
        state = image_state_gaussian; break;
    case IDM_EFFECTS_SHARPENING:
        state = image_state_sharpen; break;
    case IDM_EFFECTS_MEAN_REMOVAL:
        state = image_state_mean_removal; break;
    default:
        state = image_state_original; break;
    }
    return state;
}

// translate image_state to msg id
UINT MapFromImageStateToMsgId(image_state state)
{
    UINT id;
    switch(state) {
    case image_state_sobel:
        id = IDM_EFFECTS_SOBELEDGEDETECTION; break;
    case image_state_prewitt:
        id = IDM_EFFECTS_PREWITTEDGEDETECTION; break;
    case image_state_scharr:
        id = IDM_EFFECTS_SCHARREDGEDETECTION; break;
    case image_state_emboss:
        id = IDM_EFFECTS_EMBOSSING; break;
    case image_state_gaussian:
        id = IDM_EFFECTS_GAUSSIAN_BLUR; break;
    case image_state_sharpen:
        id = IDM_EFFECTS_SHARPENING; break;
    case image_state_mean_removal:
        id = IDM_EFFECTS_MEAN_REMOVAL; break;
    default:
        id = IDM_EFFECTS_ORIGINAL; break;
    }
    return id;
}

// Update the menu of "Effects"
void UpdateEffectsSubMenu(image_state prev, image_state current)
{
    if (prev != current) {
        HMENU menu = GetSubMenu(GetMenu(g_hWnd), g_effects_sub_menu_pos);
        CheckMenuRadioItem(menu, g_effects_first, g_effects_last, MapFromImageStateToMsgId(current), MF_BYCOMMAND);
    }   
}

// Apply the effects according to "state"
void ApplyEffects(image_state state)
{
    bool show_original_image = true;
    if (state != image_state_original && state != image_state_none) {
        // output texture view
#if (_MSC_VER >= 1800)
        texture_view<unorm4, 2> output_tex_view(*g_pAmpProcessedTexture);
#else
        writeonly_texture_view<unorm4, 2> output_tex_view(*g_pAmpProcessedTexture);
#endif
        // input texture, original picture
        const texture<unorm4, 2> input_tex = make_texture<unorm4, 2>(g_av, reinterpret_cast<IUnknown *>(g_pInputTexture));
        assert(input_tex.extent == output_tex_view.extent);
        switch(state) {
        case image_state_sobel:
            ApplyEdgeDetection<sobel_kind>(input_tex, output_tex_view);
            break;
        case image_state_prewitt:
            ApplyEdgeDetection<prewitt_kind>(input_tex, output_tex_view);
            break;
        case image_state_scharr:
            ApplyEdgeDetection<scharr_kind>(input_tex, output_tex_view);
            break;
        case image_state_emboss:
            ApplyOneConvolutionFilter<emboss_kind>(input_tex, output_tex_view);
            break;
        case image_state_gaussian:
            ApplyOneConvolutionFilter<gaussian_kind>(input_tex, output_tex_view);
            break;
        case image_state_sharpen:
            ApplyOneConvolutionFilter<sharpen_kind>(input_tex, output_tex_view);
            break;
        case image_state_mean_removal:
            ApplyOneConvolutionFilter<mean_removal_kind>(input_tex, output_tex_view);
            break;
        default:
            assert(false);
            break;
        }
        show_original_image = false;
    }
    ShowImage(show_original_image);
}

// ===================================================================================================================
// C++ AMP algorithm layer
// ===================================================================================================================
// Apply edge detection with two filters
#if (_MSC_VER >= 1800)
template<filter_kind kind>
void ApplyEdgeDetection(const texture<unorm4, 2> & input_tex,  const texture_view<unorm4, 2> output_tex_view)
#else
template<filter_kind kind>
void ApplyEdgeDetection(const texture<unorm4, 2> & input_tex, const writeonly_texture_view<unorm4, 2> output_tex_view)
#endif
{
    parallel_for_each(input_tex.accelerator_view, output_tex_view.extent, [=, &input_tex] (index<2> idx) restrict(amp) {
        int y = idx[0];
        int x = idx[1];
        const int N = filter_trait<kind>::size; // filter size
        static_assert(N % 2 == 1, "N has to be a odd number"); // odd number
        static const int W = N/2;
        if( idx[0] < W || idx[0] >= output_tex_view.extent[0] - W || idx[1] < W || idx[1] >= output_tex_view.extent[1] - W) { // edge
            output_tex_view.set(idx, unorm4());
        } else {
            static const int LB = -(W), UB = W; // loop bound
            filter<N> GX, GY;
            filter_trait<kind>::create_filter(GX, GY);
            float sumX = 0.0f;
            float sumY = 0.0f;
            float sum = 0.0f;
            auto f = [=, &input_tex] (int offset_y, int offset_x) restrict(amp) -> float {
                index<2> id(offset_y + y, offset_x + x);
                float3 pixel = static_cast<float3>(input_tex[id].rgb);
                return (pixel.r + pixel.g + pixel.b)/3.0f;
            };
            // apply the filter
#ifdef _DEBUG // for easier debugging
            for(int i = LB; i <= UB; i++) {
                for (int j = LB; j <= UB; j++) {
                    float val = f(i, j);
                    sumX += val * GX(i+W, j+W);
                    sumY += val * GY(i+W, j+W); 
                }
            }
#else // !_DEBUG 
            array_view<float> sumX_view(1, &sumX);
            array_view<float> sumY_view(1, &sumY);
            loop_unroller<LB, UB>::func([=](int i) restrict(amp) {
                loop_unroller<LB, UB>::func([=](int j) restrict(amp) {
                    float val = f(i, j);
                    sumX_view(0) += val * GX(i+W, j+W);
                    sumY_view(0) += val * GY(i+W, j+W); 
                });
            });
#endif // _DEBUG
            sum = fabs(sumX); 
            sum += fabs(sumY);
            sum = 1.0f  - fmin(sum, 1.0f);
            output_tex_view.set(idx, unorm4(sum, sum, sum, 1.0f));
        }
    });
}

// Apply a filter via convolution
#if (_MSC_VER >= 1800)
template<filter_kind kind>
void ApplyOneConvolutionFilter(const texture<unorm4, 2> & input_tex,  const texture_view<unorm4, 2> output_tex_view)
#else
template<filter_kind kind>
void ApplyOneConvolutionFilter(const texture<unorm4, 2> & input_tex, const writeonly_texture_view<unorm4, 2> output_tex_view)
#endif
{
    parallel_for_each(input_tex.accelerator_view, output_tex_view.extent, [=, &input_tex] (index<2> idx) restrict(amp) {
        int y = idx[0];
        int x = idx[1];
        const int N = filter_trait<kind>::size;
        static_assert(N % 2 == 1, "N has to be a odd number"); // odd number
        static const int W = N/2;
        if( idx[0] < W || idx[0] >= output_tex_view.extent[0] - W || idx[1] < W || idx[1] >= output_tex_view.extent[1] - W) { // edge
            output_tex_view.set(idx, unorm4());
        } else {
            static const int LB = -(W), UB = W; // loop bound
            filter<N> GX;
            filter_trait<kind>::create_filter(GX);
            auto f = [=, &input_tex] (int offset_y, int offset_x) restrict(amp) -> float3 {
                index<2> id(offset_y + y, offset_x + x);
                float3 pixel = static_cast<float4>(input_tex[id]).rgb;
                return pixel * GX(offset_y + W, offset_x + W);
            };
            float3 result(0.0f);
#ifdef _DEBUG // for easier debugging
            for(int i = LB; i <= UB; i++) {
                for (int j = LB; j <= UB; j++) {
                    result += f(i, j);
                }
            }
#else // !_DEBUG
            array_view<float3> result_view(1, &result);
            loop_unroller<LB, UB>::func([=](int i) restrict(amp) {
                loop_unroller<LB, UB>::func([=](int j) restrict(amp) {
                    result_view(0) += f(i, j);
                });
            });
#endif // _DEBUG
            result /= static_cast<float>(filter_trait<kind>::factor);
            result += (static_cast<float>(filter_trait<kind>::offset) / 255.0f);
            output_tex_view.set(idx, unorm4(result.r, result.g, result.b, input_tex[idx].a));
        }
    });
}
