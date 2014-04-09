#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Triangle.h
 * File Description : Responsible for creating and rendering a triangle
 */
#include <d3d11_1.h>
#include <wrl/client.h>
#include "Graphics/Shaders/Shader.h"
#include "Graphics/Shaders/CBuffer.h"
#include "TessObjectVars.h"

ref class Triangle
{
internal:
	//Constructor 
	Triangle();
	void Load();
	void Render();
	float GetTessEdge();
	float GetTessInside();
	void SetTessEdge(float _value);
	void SetTessInside(float _value);

private:
	UINT indicesSize;
	UINT stride;
	UINT offset;
	Shader^ shader;
	CBuffer<TessObjectVars> cBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
};