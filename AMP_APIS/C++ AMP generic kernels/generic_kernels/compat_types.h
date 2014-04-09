//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 Arnaud Faucher
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//////////////////////////////////////////////////////////////////////////////////

// future-proof C++ AMP compatibility definitions (based on available information)

#pragma once

#include "iostream"

#define amp direct3d

typedef unsigned int uint;

struct int_2
{
    int x, y;
};

struct int_4
{
    int x, y, z, w;
};

struct uint_3
{
    uint x, y, z;
};

struct uint_4
{
    uint x, y, z, w;
};

struct float_3
{
    float x, y, z;
};

inline float_3 make_float_3(float x, float y, float z) restrict(cpu,amp)
{
	float_3 a;
	a.x = x;
	a.y = y;
	a.z = z;
	return a;
}

inline float_3 operator+(const float_3& a, const float_3& b) restrict(cpu,amp)
{
	return make_float_3(a.x+b.x, a.y+b.y, a.z+b.z);
}

inline float_3 operator-(const float_3& a, const float_3& b) restrict(cpu,amp)
{
	return make_float_3(a.x-b.x, a.y-b.y, a.z-b.z);
}

inline float_3 operator*(const float_3& a, float b) restrict(cpu,amp)
{
	return make_float_3(a.x*b, a.y*b, a.z*b);
}

inline float_3 operator/(const float_3& a, float b) restrict(cpu,amp)
{
	float c = 1.0F/b;
	return a*c;
}

inline float_3 operator/(const float_3& a, int b) restrict(cpu,amp)
{
	float c = 1.0F/b;
	return a*c;
}

inline float_3& operator+= (float_3& a, const float_3& b) restrict(cpu,amp)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
    return a;
}

inline float_3& operator*= (float_3& a, const float_3& b) restrict(cpu,amp)
{
	a.x *= b.x;
	a.y *= b.y;
	a.z *= b.z;
    return a;
}

struct float_4
{
    float x, y, z, w;
};

inline float_4 make_float_4(float x, float y, float z, float w) restrict(cpu,amp)
{
	float_4 a;
	a.x = x;
	a.y = y;
	a.z = z;
	a.w = w;
	return a;
}

inline std::ostream& operator<<(std::ostream& os, const float_4& a)
{
	return os << "(" << a.x << "," << a.y << "," << a.z << "," << a.w << ")";
}

inline bool operator==(const float_4& a, const float_4& b) restrict(cpu,amp)
{
	return (a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
}

inline float_4 operator+(const float_4& a, const float_4& b) restrict(cpu,amp)
{
	return make_float_4(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w);
}

inline float_4 operator-(const float_4& a, const float_4& b) restrict(cpu,amp)
{
	return make_float_4(a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w);
}

inline float_4 operator*(const float_4& a, float b) restrict(cpu,amp)
{
	return make_float_4(a.x*b, a.y*b, a.z*b, a.w*b);
}

inline float_4 operator/(const float_4& a, float b) restrict(cpu,amp)
{
	float c = 1.0F/b;
	return a*c;
}

inline float_4 operator/(const float_4& a, int b) restrict(cpu,amp)
{
	float c = 1.0F/b;
	return a*c;
}

inline float_4& operator+= (float_4& a, const float_4& b) restrict(cpu,amp)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	a.w += b.w;
    return a;
}

inline float_4& operator*= (float_4& a, const float_4& b) restrict(cpu,amp)
{
	a.x *= b.x;
	a.y *= b.y;
	a.z *= b.z;
	a.w *= b.w;
    return a;
}
