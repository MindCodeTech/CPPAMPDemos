/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : KeyboardState.cpp
 * File Description : 
 */
#include "pch.h"
#include "KeyboardState.h"

using namespace Windows::System;

KeyboardState::KeyboardState()
{
}

KeyboardState::~KeyboardState()
{
	Unload();
}

bool KeyboardState::IsKeyDown(VirtualKey key)
{
	return this->keys[key] == true;
}
bool KeyboardState::IsKeyUp(VirtualKey key)
{
	return this->keys[key] == false;
}

void KeyboardState::SaveKeyState(VirtualKey key, bool IsPressed)
{
	this->keys[key] = IsPressed;
}

void KeyboardState::ClearBuffer()
{
	for (auto k : this->keys)
	{    
		k.second = false; 
	}
}
void KeyboardState::Unload()
{
	this->keys.clear();
}


