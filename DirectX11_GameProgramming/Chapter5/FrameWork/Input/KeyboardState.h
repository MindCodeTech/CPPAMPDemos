#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : KeyboardState.h
 * File Description : Represent the state of keyboard
 */
#include <map>

class KeyboardState
{
private:
	std::map<Windows::System::VirtualKey, bool> keys;
public:
	KeyboardState();
	~KeyboardState();
	
	bool IsKeyDown(Windows::System::VirtualKey key);
	bool IsKeyUp(Windows::System::VirtualKey key);
	void SaveKeyState(Windows::System::VirtualKey key, bool IsPressed);
	void ClearBuffer();
	void Unload();
};

