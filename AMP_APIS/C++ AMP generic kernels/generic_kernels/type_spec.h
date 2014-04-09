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

// Common types specifications

#pragma once

#include <limits.h>

#include "compat_types.h"

namespace generic_kernels
{
	template <class T>
	class type_spec
	{
	public:
		inline static T zero() restrict(cpu,amp) { return (T)0; }
		inline static T one() restrict(cpu,amp) { return (T)1; }
		inline static T make_rand() restrict(cpu) { return (T)(rand() - RAND_MAX/2.0); }
		inline static T minvalue() restrict(cpu,amp) { return (T)0; }
		inline static T maxvalue() restrict(cpu,amp) { return (T)0; }
	};

	template <>
	class type_spec<int>
	{
	public:
		inline static int zero() restrict(cpu,amp) { return 0; }
		inline static int one() restrict(cpu,amp) { return 1; }
		inline static int make_rand() restrict(cpu) { return (int)(rand() - RAND_MAX/2.0); }
		inline static int minvalue() restrict(cpu,amp) { return INT_MIN; }
		inline static int maxvalue() restrict(cpu,amp) { return INT_MAX; }
	};

	template <>
	class type_spec<uint>
	{
	public:
		inline static uint zero() restrict(cpu,amp) { return 0U; }
		inline static uint one() restrict(cpu,amp) { return 1U; }
		inline static uint make_rand() restrict(cpu) { return rand(); }
		inline static uint minvalue() restrict(cpu,amp) { return 0; }
		inline static uint maxvalue() restrict(cpu,amp) { return UINT_MAX; }
	};

	template <>
	class type_spec<float>
	{
	public:
		inline static float zero() restrict(cpu,amp) { return 0.0F; }
		inline static float one() restrict(cpu,amp) { return 1.0F; }
		inline static float make_rand() restrict(cpu) { return (float)(rand() - RAND_MAX/2.0); }
		inline static float minvalue() restrict(cpu,amp) { return FLT_MIN; }
		inline static float maxvalue() restrict(cpu,amp) { return FLT_MAX; }
	};

	template <>
	class type_spec<double>
	{
	public:
		inline static double zero() restrict(cpu,amp) { return 0.0; }
		inline static double one() restrict(cpu,amp) { return 1.0; }
		inline static double make_rand() restrict(cpu) { return rand() - RAND_MAX/2.0; }
		inline static double minvalue() restrict(cpu,amp) { return DBL_MIN; }
		inline static double maxvalue() restrict(cpu,amp) { return DBL_MAX; }
	};

	template <>
	class type_spec<float_3>
	{
	public:
		inline static float_3 zero() restrict(cpu,amp) { return make_float_3(0.0F, 0.0F, 0.0F); }
		inline static float_3 one() restrict(cpu,amp) { return make_float_3(1.0F, 1.0F, 1.0F); }
		inline static float_3 make_rand() restrict(cpu) { return make_float_3(rand() - RAND_MAX/2.0F, rand() - RAND_MAX/2.0F, rand() - RAND_MAX/2.0F); }
		inline static float_3 minvalue() restrict(cpu,amp) { float i = FLT_MIN; return make_float_3(i, i, i); }
		inline static float_3 maxvalue() restrict(cpu,amp) { float i = FLT_MAX; return make_float_3(i, i, i); }
	};

	template <>
	class type_spec<float_4>
	{
	public:
		inline static float_4 zero() restrict(cpu,amp) { return make_float_4(0.0F, 0.0F, 0.0F, 0.0F); }
		inline static float_4 one() restrict(cpu,amp) { return make_float_4(1.0F, 1.0F, 1.0F, 1.0F); }
		inline static float_4 make_rand() restrict(cpu) { return make_float_4(rand() - RAND_MAX/2.0F, rand() - RAND_MAX/2.0F, rand() - RAND_MAX/2.0F, rand() - RAND_MAX/2.0F); }
		inline static float_4 minvalue() restrict(cpu,amp) { float i = FLT_MIN; return make_float_4(i, i, i, i); }
		inline static float_4 maxvalue() restrict(cpu,amp) { float i = FLT_MAX; return make_float_4(i, i, i, i); }
	};
}
