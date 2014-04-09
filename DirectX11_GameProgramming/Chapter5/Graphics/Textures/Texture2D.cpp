/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Texture2D.cpp
 * File Description : 
 */
#include "pch.h"
#include "Texture2D.h"

Texture2D::Texture2D()
{
}

void Texture2D::Load(ComPtr<ID3D11Resource> resource , _In_ ID3D11ShaderResourceView* shaderResourceView)
{
	if (shaderResourceView == nullptr) return;

	this->srv = shaderResourceView;
	auto hr = resource.As(&this->texture2D);
	DX::ThrowIfFailed(hr);
	this->texture2D->GetDesc(&this->texDesc);
}


