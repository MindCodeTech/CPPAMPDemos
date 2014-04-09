/*
	FOR GETTING MORE INFORMATION ABOUT THIS CODE PLEASE CHECK http://directx11-1-gameprogramming.azurewebsites.net/
	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
	ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
	PARTICULAR PURPOSE.
	Copyright (c) Microsoft Corporation. All rights reserved

	File Name        : Texture.h
	Generated by     : Pooya Eimandar (http://Pooya-Eimandar.com/)
	File Description :
*/

#pragma once

ref class Texture
{
internal:
	Texture();
	void Load(Graphics3D G3D, Platform::String^ filename);
	Concurrency::task<void> LoadAsync(Graphics3D G3D, Platform::String^ filename);

	property ID3D11ShaderResourceView** TextureView
	{
		ID3D11ShaderResourceView** get() 
		{ 
			return this->textureView.GetAddressOf(); 
		}
	}
	property ID3D11SamplerState** Sampler
	{
		ID3D11SamplerState** get() 
		{ 
			return this->sampler.GetAddressOf(); 
		}
	}

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureView;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;	
	void CreateSampler(ID3D11Device1* device);
};