/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : SpriteFont.h
 * File Description : Declare the font style
 */
#include <wrl/client.h>
#include <d2d1_1.h>
#include <dwrite_1.h>

using namespace D2D1;
using namespace Microsoft::WRL;

struct SpriteFontStyle
{
	DWRITE_FONT_WEIGHT WEIGHT;
	DWRITE_FONT_STYLE  STYLE;
	DWRITE_FONT_STRETCH STRETCH;
	DWRITE_TEXT_ALIGNMENT TEXT_ALIGNMENT;
	DWRITE_PARAGRAPH_ALIGNMENT PARAGRAPH_ALIGNMENT;
	float SIZE;
	const WCHAR* LOCAL;
};

ref class SpriteFont
{
internal:
	SpriteFont();
	HRESULT Load();
	HRESULT SetColor(ColorF color);
	property IDWriteTextFormat* Font
	{
		IDWriteTextFormat* get()
		{
			return this->font.Get();
		}
	}
	property ID2D1SolidColorBrush* Brush
	{
		ID2D1SolidColorBrush* get()
		{
			return this->brush.Get();
		}
	}

private:
	SpriteFontStyle fontStyle;
	ComPtr<ID2D1SolidColorBrush> brush; 
	ComPtr<IDWriteTextFormat> font;
	ComPtr<IDWriteTextLayout> textLayout;
};
