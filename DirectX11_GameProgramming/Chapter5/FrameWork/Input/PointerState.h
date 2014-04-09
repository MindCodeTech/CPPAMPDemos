#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : PointerState.h
 * File Description : Represent the state of pointer
 */
ref class PointerState
{
private:
	DirectX::XMFLOAT2 position;
	Windows::UI::Input::PointerPointProperties^ state;
internal:
	PointerState();

	void SavePosition(Windows::Foundation::Point Position);
	void SaveState( Windows::UI::Input::PointerPointProperties^ Properties );
	bool IsLeftButtonPressed();
	bool IsRightButtonPressed();

	property DirectX::XMFLOAT2 Position
	{
		DirectX::XMFLOAT2 get()
		{
			return this->position;
		}
	}
};