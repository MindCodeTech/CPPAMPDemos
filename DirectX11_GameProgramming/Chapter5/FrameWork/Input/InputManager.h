#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : InputManager.h
 * File Description : Manage the inputs
 */
#include "KeyboardState.h"
#include "PointerState.h"
#include "GamePadState.h"

namespace InputManager
{
	extern KeyboardState keyboardState;
	extern PointerState  pointerState;
	extern GamePadState  gamePadState;

	extern void Update();
	extern void ClearBuffers();
	extern void Unload();
}
