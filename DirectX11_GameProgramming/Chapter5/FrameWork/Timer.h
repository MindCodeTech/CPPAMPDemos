#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Timer.h
 * File Description : A simple timer
 */
#include <wrl.h>

ref class Timer sealed
{
private:
	LARGE_INTEGER frequency, currentTime, startTime, lastTime;
	float totalTime, deltaTime;
public:
	// Duration ( in seconds ) between the last call to Reset() and the last call to Update().
	property float Total
	{
		float get() 
		{ 
			return this->totalTime; 
		}
	}
	// Duration in seconds between the previous two calls to Update().
	property float Delta
	{
		float get() 
		{ 
			return deltaTime; 
		}
	}
	// Constructor of Timer
	Timer()
	{
		if (!QueryPerformanceFrequency(&frequency))
		{
			throw ref new Platform::FailureException();
		}
		Reset();
	}

	// Reset the timer
	void Reset()
	{
		Update();
		this->startTime = currentTime;
		this->totalTime = 0.0f;
		this->deltaTime = 1.0f / 60.0f;
	}

	void Update()
	{
		if (!QueryPerformanceCounter(&currentTime))
		{
			throw ref new Platform::FailureException();
		}

		this->totalTime = static_cast<float>(
			static_cast<double>(this->currentTime.QuadPart - this->startTime.QuadPart) / static_cast<double>(this->frequency.QuadPart));

		if (this->lastTime.QuadPart == this->startTime.QuadPart)
		{
			// If the timer was just reset, report a time delta equivalent to 60Hz frame time.
			this->deltaTime = 1.0f / 60.0f;
		}
		else
		{
			this->deltaTime = static_cast<float>(
				static_cast<double>(this->currentTime.QuadPart - this->lastTime.QuadPart) /
				static_cast<double>(this->frequency.QuadPart)
				);
		}

		this->lastTime = this->currentTime;
	}
};
