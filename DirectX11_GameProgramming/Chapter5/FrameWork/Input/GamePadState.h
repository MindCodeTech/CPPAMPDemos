#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : GamePadState.h
 * File Description : Represent the state of gamepad
 */
#pragma comment(lib, "xinput.lib")
#include <Xinput.h>

#define MAXCONTROLLERS 4

enum GamePadButtons { A, B, X, Y, START, BACK, UP, DOWN, LEFT, RIGHT, LTHUMB, RTHUMB, LB, RB };

class GamePadState
{
private:
	struct State
	{
		XINPUT_STATE XState;
		bool isConnected;
		std::map<GamePadButtons, bool> Buttons;

		State()
		{
			this->Buttons.insert(std::make_pair(GamePadButtons::A, false));
			this->Buttons.insert(std::make_pair(GamePadButtons::B, false));
			this->Buttons.insert(std::make_pair(GamePadButtons::X, false));
			this->Buttons.insert(std::make_pair(GamePadButtons::Y, false));
			this->Buttons.insert(std::make_pair(GamePadButtons::START, false));
			this->Buttons.insert(std::make_pair(GamePadButtons::BACK, false));
			this->Buttons.insert(std::make_pair(GamePadButtons::UP, false));
			this->Buttons.insert(std::make_pair(GamePadButtons::DOWN, false));
			this->Buttons.insert(std::make_pair(GamePadButtons::LEFT, false));
			this->Buttons.insert(std::make_pair(GamePadButtons::RIGHT, false));
			this->Buttons.insert(std::make_pair(GamePadButtons::LTHUMB, false));
			this->Buttons.insert(std::make_pair(GamePadButtons::RTHUMB, false));
			this->Buttons.insert(std::make_pair(GamePadButtons::LB, false));
			this->Buttons.insert(std::make_pair(GamePadButtons::RB, false));
		}
	}states[MAXCONTROLLERS];
public:
	GamePadState();
	~GamePadState();

	void Update();
	
	bool IsLeftThumbToLeft(UINT index);
	bool IsLeftThumbToRight(UINT index);
	bool IsLeftThumbToUp(UINT index);
	bool IsLeftThumbToDown(UINT index);

	bool IsRightThumbToLeft(UINT index);
	bool IsRightThumbToRight(UINT index);
	bool IsRightThumbToUp(UINT index);
	bool IsRightThumbToDown(UINT index);

	bool IsRightTriggerPressed(UINT index);
	bool IsLeftTriggerPressed(UINT index);

	const std::map<GamePadButtons, bool> GetButtons(UINT index);
	void ClearBuffers();
	void Unload();
};

