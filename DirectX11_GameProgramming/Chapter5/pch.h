#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : pch.h
 * File Description : The pre-compiled header
 */

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "windowscodecs.lib")

#include <wrl/client.h>
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <d2d1effects.h>
#include <dwrite_1.h>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <wincodec.h>
#include <memory>
#include <vector>
#include <map>
#include <ppl.h>
#include <ppltasks.h>
#include <amp.h>
#include <amp_graphics.h>
#include <amp_math.h>
#include <agile.h>

#ifdef XAML
#include "windows.ui.xaml.media.dxinterop.h"
#endif // XAML

#include "Framework/MathHelper.h"
#include "FrameWork/Cameras/FirstCamera.h"

#define SAFE_DELETE(x) { if(x){ delete x; x = nullptr; } }
#define SAFE_RELEASE(x) { if(x){ x->Release(); x = nullptr; } }
#define SAFE_UNLOAD(x) { if(x){ x->Unload(); x = nullptr; } }

class GraphicsDevice
{
public:
	float AspectRatio;
	Microsoft::WRL::ComPtr<ID2D1Factory1> factory;
	Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory;
	Microsoft::WRL::ComPtr<IWICImagingFactory2> imageFactory;
	Microsoft::WRL::ComPtr<ID2D1Device> d2dDevice;
	Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dContext;
	Microsoft::WRL::ComPtr<ID3D11Device1> d3dDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> d3dContext;
	std::vector<Microsoft::WRL::ComPtr<ID3D11SamplerState>> Samplers;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> defaultVS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> defaultLayout;
	std::map<Platform::String^, ID3D11PixelShader*> PixelShaders;
};

namespace DX
{
	inline void OutputDebug(Platform::String^ Msg)
	{
		auto data = std::wstring(Msg->Data());
		OutputDebugString(data.c_str());
	}

	inline Platform::Array<byte>^ ReadFile(Platform::String^ path)
	{
		using namespace Platform;

		Array<byte>^ bytes = nullptr;

		FILE* f = nullptr;
		_wfopen_s(&f, path->Data(), L"rb");
		if (f == nullptr)
		{
			throw ref new Exception(0, "Could not open file on following path : " + path);
		}
		else
		{
			fseek(f, 0, SEEK_END);
			auto pos = ftell(f);
			bytes = ref new Array<byte>(pos);
			fseek(f, 0, SEEK_SET);

			// read data into the prepared buffer
			if (pos > 0)
			{
				fread(&bytes[0], 1, pos, f);
			}
			// close the file
			fclose(f);
		}
		return bytes;
	}

	inline Concurrency::task<Platform::Array<byte>^> ReadFileAsync(Platform::String^ path)
	{
		using namespace Windows::Storage;
		using namespace Streams;
		using namespace Platform;

		auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;

		return Concurrency::create_task(folder->GetFileAsync(path)).then([] (StorageFile^ file) 
		{
			return FileIO::ReadBufferAsync(file);

		}).then([] (IBuffer^ fileBuffer) -> Array<byte>^ 
		{
			auto fileData = ref new Array<byte>(fileBuffer->Length);
			DataReader::FromBuffer(fileBuffer)->ReadBytes(fileData);
			return fileData;
		});
	}

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch Win32 API errors.
			throw Platform::Exception::CreateException(hr);
		}
	}

	extern GraphicsDevice GDevice;
	extern FirstCamera^ Camera;
	extern concurrency::accelerator_view  accViewObj;
	extern bool UseDispMap;
}
