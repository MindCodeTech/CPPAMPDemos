/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : InputManager.cpp
 * File Description : 
 */
#include "pch.h"
#include "InputManager.h"

KeyboardState InputManager::keyboardState;
PointerState  InputManager::pointerState;
GamePadState  InputManager::gamePadState;

void InputManager::Update()
{
	gamePadState.Update();
}

void InputManager::ClearBuffers()
{
	keyboardState.ClearBuffer();
	gamePadState.ClearBuffers();
}

void InputManager::Unload()
{
	keyboardState.Unload();
	gamePadState.Unload();
}

