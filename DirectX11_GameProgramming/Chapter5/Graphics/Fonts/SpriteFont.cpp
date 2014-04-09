/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : SpriteFont.cpp
 * File Description : 
 */
#include "pch.h"
#include "SpriteFont.h"

using namespace DX;

SpriteFont::SpriteFont()
{
	this->fontStyle.WEIGHT = DWRITE_FONT_WEIGHT_LIGHT;
	this->fontStyle.STYLE = DWRITE_FONT_STYLE_NORMAL;
	this->fontStyle.STRETCH = DWRITE_FONT_STRETCH_NORMAL;
	this->fontStyle.TEXT_ALIGNMENT = DWRITE_TEXT_ALIGNMENT_LEADING;
	this->fontStyle.PARAGRAPH_ALIGNMENT = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
	this->fontStyle.SIZE = 16.0;
	this->fontStyle.LOCAL = L"en-US";
}

HRESULT SpriteFont::Load()
{
	auto hr = GDevice.writeFactory->CreateTextFormat(
		L"Times New Roman",
		nullptr, 
		fontStyle.WEIGHT,
		fontStyle.STYLE,
		fontStyle.STRETCH,
		fontStyle.SIZE,
		fontStyle.LOCAL,
		&this->font);
	ThrowIfFailed(hr);

	hr = this->font->SetTextAlignment(this->fontStyle.TEXT_ALIGNMENT);
	ThrowIfFailed(hr);

	hr = this->font->SetParagraphAlignment(this->fontStyle.PARAGRAPH_ALIGNMENT);
	ThrowIfFailed(hr);

	hr = SetColor(ColorF::White);
	ThrowIfFailed(hr);

	return hr;
}

HRESULT SpriteFont::SetColor(ColorF color)
{
	return GDevice.d2dContext->CreateSolidColorBrush(color, &this->brush);
}