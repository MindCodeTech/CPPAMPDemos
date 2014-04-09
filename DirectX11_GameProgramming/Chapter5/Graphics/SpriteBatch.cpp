/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : SpriteBatch.cpp
 * File Description : 
 */
#include "pch.h"
#include "SpriteBatch.h"

#define MISSING_BEGIN_CALL "Begin must be called successfully before a method can be called."
#define MISSING_END_CALL   "Begin cannot be called again until End has been successfully called."

using namespace DX;

#pragma region Definition of SpriteBatch

SpriteBatch::SpriteBatch() : state(ENDED), BackgroundText(D2D1::RectF(7, 7, 400, 330))
{
	SetBackgroundColor(ColorF(0.3f, 0.3f, 0.3f, 0.5f), ColorF::White);
	auto hr = GDevice.factory->CreateDrawingStateBlock(&this->stateBlock);
	ThrowIfFailed(hr);
}

void SpriteBatch::Load()
{
	auto hr = GDevice.d2dContext->CreateEffect(CLSID_D2D1BitmapSource, &this->bitmapSourceEffect);
	ThrowIfFailed(hr);

	hr = GDevice.d2dContext->CreateEffect(CLSID_D2D1GaussianBlur, &this->gaussianBlurEffect);
	ThrowIfFailed(hr);
}

void SpriteBatch::SetWICBitmapSource(ComPtr<IWICFormatConverter>  wicConverter)
{
	auto hr = this->bitmapSourceEffect->SetValue(D2D1_BITMAPSOURCE_PROP_WIC_BITMAP_SOURCE, wicConverter.Get());
	ThrowIfFailed(hr);

	this->gaussianBlurEffect->SetInputEffect(0, this->bitmapSourceEffect.Get());
}

void SpriteBatch::SetBackgroundColor(ColorF BackColor, ColorF BorderColor)
{
	auto hr = GDevice.d2dContext->CreateSolidColorBrush(BackColor, &this->BackgroundBrush);
	ThrowIfFailed(hr);

	hr = GDevice.d2dContext->CreateSolidColorBrush(BorderColor, &this->BackgroundBorder);
	ThrowIfFailed(hr);
}

void SpriteBatch::SetGaussianBlurValue(D2D1_GAUSSIANBLUR_PROP effectProperty, float val)
{
	auto hr = this->gaussianBlurEffect.Get()->SetValue(effectProperty, val);
	ThrowIfFailed(hr);
}

void SpriteBatch::Begin()
{
	if (this->state != ENDED) throw ref new Exception(0, MISSING_END_CALL);
	this->state = STARTED;
	GDevice.d2dContext->SaveDrawingState(this->stateBlock.Get());
	GDevice.d2dContext->BeginDraw();
}

void SpriteBatch::ShowRectangle()
{
	if (this->state != STARTED) throw ref new Exception(0, MISSING_BEGIN_CALL);
	GDevice.d2dContext->DrawRectangle(BackgroundText, this->BackgroundBorder.Get());
	GDevice.d2dContext->FillRectangle(&BackgroundText, this->BackgroundBrush.Get() );
}

HRESULT SpriteBatch::ShowString(String^ text, const XMFLOAT2* position, SpriteFont^ spriteFont)
{
	return ShowString(text, position, nullptr, spriteFont, Matrix3x2F::Identity());
}

HRESULT SpriteBatch::ShowString(String^ text, const XMFLOAT2* position, ID2D1SolidColorBrush* brush, SpriteFont^ spriteFont, Matrix3x2F World)
{
	if (this->state != STARTED) throw ref new Exception(0, MISSING_BEGIN_CALL);

	auto hr = S_FALSE;

	if(!text->Equals(this->text))
	{
		this->text = text;

		hr = GDevice.writeFactory->CreateTextLayout(
		this->text->Data(),
		this->text->Length(),
		spriteFont->Font,
		4096.0f,
		4096.0f,
		&this->textLayout);
		ThrowIfFailed(hr);
	}

	GDevice.d2dContext->SetTransform(&World);
	GDevice.d2dContext->DrawTextLayout(Point2F(7 + position->x, 7 + position->y), this->textLayout.Get(), brush == nullptr ? spriteFont->Brush : brush );
	return hr;
}

void SpriteBatch::DrawImage(XMFLOAT2 Position)
{
	if (this->state != STARTED) throw ref new Exception(0, MISSING_BEGIN_CALL);

	GDevice.d2dContext->DrawImage(this->gaussianBlurEffect.Get(), Point2F(Position.x, Position.y));
}

HRESULT SpriteBatch::End()
{
	auto hr = GDevice.d2dContext->EndDraw();
	if (hr != D2DERR_RECREATE_TARGET)
	{
		ThrowIfFailed(hr);
	}

	GDevice.d2dContext->RestoreDrawingState(this->stateBlock.Get());
	this->state = ENDED;
	return hr;
}

#pragma endregion