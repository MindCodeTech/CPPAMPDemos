/*
	FOR GETTING MORE INFORMATION ABOUT THIS CODE PLEASE CHECK http://directx11-1-gameprogramming.azurewebsites.net/
	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
	ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
	PARTICULAR PURPOSE.
	Copyright (c) Microsoft Corporation. All rights reserved

	File Name        : Billboard.h
	Generated by     : Pooya Eimandar (http://Pooya-Eimandar.com/)
	File Description :
*/

#pragma once

#include "GeometricObjects\SquareObj.h"
#include "Shader.h"
#include "Texture.h"

ref class Billboard
{
private:
	DirectX::XMFLOAT3 position;
	SquareObj^ squareObj;
	Shader^ shader;
	Texture^ texture0;
	Texture^ texture1;
	Texture^ texture2;
	DirectX::XMFLOAT4X4 world;
internal:
	Billboard();
	Concurrency::task<void> Billboard::LoadAsync(GraphicsDevice GDevice);
	void Update();
	void Render(GraphicsDevice GDevice, float Time);
	void Unload();

	property DirectX::XMFLOAT3 Position
	{
		DirectX::XMFLOAT3 get()
		{
			return this->position;
		}
		void set(DirectX::XMFLOAT3 val)
		{
			this->position =val;
		}
	}
};
