#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : FPS.h
 * File Description : Display the frame per second
 */
using namespace Microsoft::WRL;

ref class FPS
{
internal:
	FPS();
	void Update(float ElapsedTime);
	void Render();

	property ID2D1SolidColorBrush* FpsColor
	{
		ID2D1SolidColorBrush* get()
		{
			if (this->state == State::Normal)
			{
				return this->LimeSolidBrushColor.Get();
			}
			if (this->state == State::Warning)
			{
				return this->YellowSolidBrushColor.Get();
			}
			return this->RedSolidBrushColor.Get();
		}
	}	
	property float fps
	{
		float get()
		{
			return this->_fps;
		}
	}

private:
	float _fps;
	float elapsedTime;
	UINT frames;
	enum State { Normal, Warning, Error } state;
	ComPtr<ID2D1SolidColorBrush> RedSolidBrushColor;
	ComPtr<ID2D1SolidColorBrush> YellowSolidBrushColor;
	ComPtr<ID2D1SolidColorBrush> LimeSolidBrushColor;
};