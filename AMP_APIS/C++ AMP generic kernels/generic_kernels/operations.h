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

// Common functions for use in reduction and scan

#pragma once

#include "type_spec.h"

#include <amp.h>

#include "compat_types.h"

namespace generic_kernels
{
	template <class T>
	class add_op
	{
	public:
		inline T& operator()(T& a, const T& b) restrict(cpu,amp) { a += b; return a; }
		inline T identity() const restrict(cpu,amp) { return type_spec<T>::zero(); }
	};

	template <class T>
	class mul_op
	{
	public:
		inline T& operator()(T& a, const T& b) restrict(cpu,amp)
		{
			a *= b;
			return a;
		}
		inline T identity() const restrict(cpu,amp) { return type_spec<T>::one(); }
	};

	template <class T>
	class min_op
	{
	public:
		inline T& operator()(T& a, const T& b) restrict(cpu,amp) { if (a > b) a = b; return a; }
		inline T identity() const restrict(cpu,amp) { return type_spec<T>::minvalue(); }
	};

	template <class T>
	class max_op
	{
	public:
		inline T& operator()(T& a, const T& b) restrict(cpu,amp) { if (a < b) a = b; return a; }
		inline T identity() const restrict(cpu,amp) { return type_spec<T>::maxvalue(); }
	};
}
