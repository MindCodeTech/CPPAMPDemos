/*
	FOR GETTING MORE INFORMATION ABOUT THIS CODE PLEASE CHECK http://directx11-1-gameprogramming.azurewebsites.net/
	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
	ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
	PARTICULAR PURPOSE.
	Copyright (c) Microsoft Corporation. All rights reserved

	File Name        : XSound.h
	Generated by     : Pooya Eimandar (http://Pooya-Eimandar.com/)
	File Description :
*/

#pragma once

ref class XSound
{
internal:
	XSound();
	void Load(IXAudio2* engine, WAVEFORMATEX* sourceFormat, Platform::Array<byte>^ soundData);
	void Play(float volume);

private:
	bool                    audioAvailable;
	IXAudio2SourceVoice*    xSourceVoice;
	Platform::Array<byte>^  soundData;
};
