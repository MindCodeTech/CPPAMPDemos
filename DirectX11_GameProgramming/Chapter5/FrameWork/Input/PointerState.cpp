/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : PointerState.cpp
 * File Description : 
 */
#include "pch.h"
#include "PointerState.h"

using namespace Windows::UI::Input;
using namespace Windows::Foundation;

PointerState::PointerState()
{
}

void PointerState::SavePosition( Point Position )
{
	this->position.x = Position.X;
	this->position.y = Position.Y;
}

void PointerState::SaveState( PointerPointProperties^ Properties )
{
	this->state = Properties;
}

bool PointerState::IsLeftButtonPressed()
{
	if(this->state == nullptr) return false;
	return this->state->IsLeftButtonPressed;
}

bool PointerState::IsRightButtonPressed()
{
	if(this->state == nullptr) return false;
	return this->state->IsRightButtonPressed;
}


