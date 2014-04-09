//--------------------------------------------------------------------------------------
// File: QJulia4DConstants.h
//
// Copyright (c) 2012, Microsoft
//  All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following 
// conditions are met:
//  - Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
//  - Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
//    disclaimer in the documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF 
// USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Coded by Jan Vlietinck, 11 Oct 2009, V 1.4
// http://users.skynet.be/fquake/
//--------------------------------------------------------------------------------------
#pragma once

#include <amp_graphics.h>

struct QJulia4DConstants
{
	concurrency::graphics::direct3d::float4 diffuse;   // diffuse shading color
	concurrency::graphics::direct3d::float4 mu;        // quaternion julia parameter

	float epsilon;    // detail julia
	int c_width;      // view port size
	int c_height;
	int selfShadow;   // selfshadowing on or off 

	float orientation[4*4]; // rotation matrix
	float zoom;
};