/*
	FOR GETTING MORE INFORMATION ABOUT THIS CODE PLEASE CHECK http://directx11-1-gameprogramming.azurewebsites.net/
	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
	ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
	PARTICULAR PURPOSE.
	Copyright (c) Microsoft Corporation. All rights reserved

	File Name        : MediEngine.cpp
	Generated by     : Pooya Eimandar (http://Pooya-Eimandar.com/)
	File Description :
*/

#include "pch.h"
#include "FrameWork\DXHelper.h"
#include "..\MediaEngine.h"

#include <mfidl.h>
#include <mfapi.h>
#include <mfreadwrite.h>

using namespace Platform;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Windows::Graphics::Display;

MediaEngine::MediaEngine():
	m_audioAvailable(false)
{
	ZeroMemory(&m_waveFormat, sizeof(m_waveFormat));
}

Platform::Array<byte>^ MediaEngine::LoadSound(Platform::String^ filename)
{
	auto hr = MFStartup(MF_VERSION);
	DXHelper::ThrowIfFailed(hr);

	auto path = DXHelper::GetInstalledLocation() + filename;

	ComPtr<IMFSourceReader> reader;
	hr = MFCreateSourceReaderFromURL(path->Data(), nullptr, &reader );
	DXHelper::ThrowIfFailed(hr);

	ComPtr<IMFMediaType> mediaType;
	hr	= MFCreateMediaType(&mediaType);
	DXHelper::ThrowIfFailed(hr);

	hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	DXHelper::ThrowIfFailed(hr);

	hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	DXHelper::ThrowIfFailed(hr);
	
	hr = reader->SetCurrentMediaType(static_cast<uint32>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0, mediaType.Get());
	DXHelper::ThrowIfFailed(hr);

	ComPtr<IMFMediaType> outputMediaType;
	hr = reader->GetCurrentMediaType(static_cast<uint32>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), &outputMediaType);
	DXHelper::ThrowIfFailed(hr);
	
	UINT32 size = 0;
	WAVEFORMATEX* waveFormat;
	hr = MFCreateWaveFormatExFromMFMediaType(outputMediaType.Get(), &waveFormat, &size);
	DXHelper::ThrowIfFailed(hr);

	CopyMemory(&m_waveFormat, waveFormat, sizeof(m_waveFormat));
	CoTaskMemFree(waveFormat);

	PROPVARIANT propVariant;
	hr = reader->GetPresentationAttribute(static_cast<uint32>(MF_SOURCE_READER_MEDIASOURCE), MF_PD_DURATION, &propVariant);
	DXHelper::ThrowIfFailed(hr);

	LONGLONG duration = propVariant.uhVal.QuadPart;
	unsigned int maxStreamLengthInBytes = static_cast<unsigned int>(((duration * static_cast<ULONGLONG>(m_waveFormat.nAvgBytesPerSec)) + 10000000) / 10000000);

	Platform::Array<byte>^ fileData = ref new Platform::Array<byte>(maxStreamLengthInBytes);

	ComPtr<IMFSample> sample;
	ComPtr<IMFMediaBuffer> mediaBuffer;
	DWORD flags = 0;

	int positionInData = 0;
	bool done = false;
	while (!done)
	{
		reader->ReadSample(static_cast<uint32>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0, nullptr, &flags, nullptr, &sample);
		DXHelper::ThrowIfFailed(hr);

		if (sample != nullptr)
		{
			hr = sample->ConvertToContiguousBuffer(&mediaBuffer);
			DXHelper::ThrowIfFailed(hr);

			BYTE *audioData = nullptr;
			DWORD sampleBufferLength = 0;
			
			hr = mediaBuffer->Lock(&audioData, nullptr, &sampleBufferLength);
			DXHelper::ThrowIfFailed(hr);

			for (DWORD i = 0; i < sampleBufferLength; i++)
			{
				fileData[positionInData++] = audioData[i];
			}
		}
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
		{
			done = true;
		}
	}

	return fileData;
}

void MediaEngine::Load()
{
	UINT32 flags = 0;
		
	auto hr = XAudio2Create(&m_soundEffectEngine, flags);
	DXHelper::ThrowIfFailed(hr);
	
	hr = m_soundEffectEngine->CreateMasteringVoice(&m_soundEffectMasteringVoice);
	DXHelper::ThrowIfFailed(hr);

	m_audioAvailable = true;
}

void MediaEngine::Suspend()
{
	if (m_audioAvailable)
	{
		m_soundEffectEngine->StopEngine();
	}
}

void MediaEngine::Resume()
{
	if (m_audioAvailable)
	{
		auto hr = m_soundEffectEngine->StartEngine();
		DXHelper::ThrowIfFailed(hr);
	}
}