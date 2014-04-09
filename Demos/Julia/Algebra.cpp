//--------------------------------------------------------------------------------------
// File: Algebra.cpp
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
#include "Algebra.h"
#include <math.h>

void mat3::identity()
{
	for (int j = 0; j < 3; j++)
	{
		for (int i=0; i<3; i++)
		{
			m[j][i] = 0;
		}
	}

	m[0][0] = 1;
	m[1][1] = 1;
	m[2][2] = 1;
}
  
// matrix multiplication
mat3 mat3::operator * (mat3& m2) const
{
	mat3 res;
	for (int j = 0; j < 3; j++)
	{
		for (int i = 0; i < 3; i++)
		{
			res(j,i) = 0;
			for (int k = 0; k < 3; k++)
			{
				res(j,i) += m[j][k] * m2(k,i);      
			}
		}
	}
	return res;
}

// access operator
float& mat3::operator () (int Row, int Col)
{
    return m[Row][Col];
}

// casting operators
mat3::operator float* ()
{
    return &m[0][0];
}

// rotate 'angle' around axis (xa, ya, za)
void mat3::rotateAroundAxis(float xa, float ya, float za, float angle)
{
	// normalize axis for safety else rotation can include scale
	float t = sqrt(xa*xa + ya*ya + za*za);
	if (t != 0)
	{
		xa /= t;
		ya /= t;
		za /= t;
	}

	m[0][0] = 1 + (1-cos(angle)) * (xa * xa - 1);
	m[1][1] = 1 + (1-cos(angle)) * (ya * ya - 1);
	m[2][2] = 1 + (1-cos(angle)) * (za * za - 1);

	m[0][1] = +za * sin(angle) + (1 - cos(angle)) * xa * ya;
	m[1][0] = -za * sin(angle) + (1 - cos(angle)) * xa * ya;

	m[0][2] = -ya * sin(angle) + (1 - cos(angle)) * xa * za;
	m[2][0] = +ya * sin(angle) + (1 - cos(angle)) * xa * za;

	m[1][2] = +xa * sin(angle) + (1 - cos(angle)) * ya * za;
	m[2][1] = -xa * sin(angle) + (1 - cos(angle)) * ya * za;
}