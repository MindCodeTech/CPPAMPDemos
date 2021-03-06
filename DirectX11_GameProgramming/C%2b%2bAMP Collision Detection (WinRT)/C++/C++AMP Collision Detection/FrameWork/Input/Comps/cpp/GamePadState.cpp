/*
	FOR GETTING MORE INFORMATION ABOUT THIS CODE PLEASE CHECK http://directx11-1-gameprogramming.azurewebsites.net/
	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
	ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
	PARTICULAR PURPOSE.
	Copyright (c) Microsoft Corporation. All rights reserved

	File Name        : GamePadState.cpp
	Generated by     : Pooya Eimandar (http://Pooya-Eimandar.com/)
	File Description :
*/

#include "pch.h"
#include "..\GamePadState.h"

GamePadState::GamePadState()
{
}

GamePadState::~GamePadState()
{
	Unload();
}

void GamePadState::Update()
{
	for(UINT i=0; i< MAXCONTROLLERS; i++)
	{
		auto hr = XInputGetState( i, &this->states[i].XState );
		if( hr == ERROR_SUCCESS )
		{
			this->states[i].isConnected = true;
		}
		else
		{
			this->states[i].isConnected = false;
		}
	}
}

bool GamePadState::IsLeftThumbToLeft(UINT index)
{
	auto state = this->states[index];
	if(!state.isConnected) return false;
	
	if (state.XState.Gamepad.sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
	{
		return true;
	}
	return false;
}

bool GamePadState::IsLeftThumbToRight(UINT index)
{
	auto state = this->states[index];
	if(!state.isConnected) return false;

	if (state.XState.Gamepad.sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
	{
		return true;
	}
	return false;
}

bool GamePadState::IsLeftThumbToUp(UINT index)
{
	auto state = this->states[index];
	if(!state.isConnected) return false;

	if (state.XState.Gamepad.sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
	{
		return true;
	}
	return false;
}

bool GamePadState::IsLeftThumbToDown(UINT index)
{
	auto state = this->states[index];
	if(!state.isConnected) return false;

	if (state.XState.Gamepad.sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
	{
		return true;
	}
	return false;
}

bool GamePadState::IsRightThumbToLeft(UINT index)
{
	auto state = this->states[index];
	if(!state.isConnected) return false;

	if (state.XState.Gamepad.sThumbRX < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
	{
		return true;
	}
	return false;
}

bool GamePadState::IsRightThumbToRight(UINT index)
{
	auto state = this->states[index];
	if(!state.isConnected) return false;

	if (state.XState.Gamepad.sThumbRX > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
	{
		return true;
	}
	return false;
}

bool GamePadState::IsRightThumbToUp(UINT index)
{
	auto state = this->states[index];
	if(!state.isConnected) return false;

	if (state.XState.Gamepad.sThumbRY > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
	{
		return true;
	}
	return false;
}

bool GamePadState::IsRightThumbToDown(UINT index)
{
	auto state = this->states[index];
	if(!state.isConnected) return false;

	if (state.XState.Gamepad.sThumbRY < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
	{
		return true;
	}
	return false;
}

bool GamePadState::IsRightTriggerPressed(UINT index)
{
	auto state = this->states[index];
	if(!state.isConnected) return false;

	if (state.XState.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
	{
		return true;
	}
	return false;
}

bool GamePadState::IsLeftTriggerPressed(UINT index)
{
	auto state = this->states[index];
	if(!state.isConnected) return false;

	if (state.XState.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
	{
		return true;
	}
	return false;
}

const std::map<GamePadButtons, bool> GamePadState::GetButtons(UINT index)
{
	auto state = this->states[index];
	auto wButtons = state.XState.Gamepad.wButtons;
	if(state.isConnected && wButtons != 0)
	{
		if (wButtons & XINPUT_GAMEPAD_DPAD_UP)
		{
			state.Buttons[GamePadButtons::UP] = true;
		}
		else if (wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
		{
			state.Buttons[GamePadButtons::DOWN] = true;
		}
		if (wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
		{
			state.Buttons[GamePadButtons::LEFT] = true;
		}
		else if (wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
		{
			state.Buttons[GamePadButtons::RIGHT] = true;
		}
		if (wButtons & XINPUT_GAMEPAD_START)
		{
			state.Buttons[GamePadButtons::START] = true;
		}
		else if (wButtons & XINPUT_GAMEPAD_BACK)
		{
			state.Buttons[GamePadButtons::BACK] = true;
		}
		else if (wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
		{
			state.Buttons[GamePadButtons::LTHUMB] = true;
		}
		else if (wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
		{
			state.Buttons[GamePadButtons::RTHUMB] = true;
		}
		else if (wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
		{
			state.Buttons[GamePadButtons::LB] = true;
		}
		else if (wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
		{
			state.Buttons[GamePadButtons::RB] = true;
		}
		else if (wButtons & XINPUT_GAMEPAD_A)
		{
			state.Buttons[GamePadButtons::A] = true;
		}
		else if (wButtons & XINPUT_GAMEPAD_B)
		{
			state.Buttons[GamePadButtons::B] = true;
		}
		else if (wButtons & XINPUT_GAMEPAD_X)
		{
			state.Buttons[GamePadButtons::X] = true;
		}
		else if (wButtons & XINPUT_GAMEPAD_Y)
		{
			state.Buttons[GamePadButtons::Y] = true;
		}
	}
	return state.Buttons;
}

void GamePadState::ClearBuffers()
{
	for (UINT i = 0; i < MAXCONTROLLERS; i++)
	{
		if(this->states[i].isConnected)
		{ 
			for (auto word : this->states[i].Buttons)
			{    
				if(!word.second)
				{
					word.second = false; 
				}
			}
		}
	}
}

void GamePadState::Unload()
{
	for (UINT i = 0; i < MAXCONTROLLERS; i++)
	{
		if(this->states[i].isConnected)
		{ 
			this->states[i].Buttons.clear();
		}
	}
}