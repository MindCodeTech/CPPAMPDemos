#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : SpriteBatch.h
 * File Description : This class used for handling Direct2D functions
 */
#include "Fonts/SpriteFont.h"
#include <DirectXMath.h>

using namespace Platform;
using namespace Microsoft::WRL;
using namespace DirectX;

ref class SpriteBatch
{
internal:
	SpriteBatch();
	void Load();
	void SetWICBitmapSource(ComPtr<IWICFormatConverter>  wicConverter);
	void SetBackgroundColor(ColorF BackColor, ColorF BorderColor);
	void SetGaussianBlurValue(D2D1_GAUSSIANBLUR_PROP effectProperty, float val);
	void Begin();
	void ShowRectangle();
	HRESULT ShowString(String^ text, const XMFLOAT2* position, SpriteFont^ spriteFont);
	HRESULT ShowString(String^ text, const XMFLOAT2* position, ID2D1SolidColorBrush* brush, SpriteFont^ spriteFont, Matrix3x2F World);
	void DrawImage(XMFLOAT2 Position);
	HRESULT End();

private:
	String^ text;
	enum SpriteState : byte { STARTED, ENDED };
	SpriteState state;
	D2D1_RECT_F BackgroundText;
	ComPtr<ID2D1Effect> bitmapSourceEffect;
	ComPtr<ID2D1Effect> gaussianBlurEffect;
	ComPtr<ID2D1SolidColorBrush> BackgroundBrush;
	ComPtr<ID2D1SolidColorBrush> BackgroundBorder;
	ComPtr<IDWriteTextLayout> textLayout;
	ComPtr<ID2D1DrawingStateBlock1> stateBlock;
};