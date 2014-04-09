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

// Various simple functions

#pragma once

#include <vector>
#include <amp.h>

#include "compat_types.h"

template <class T, unsigned int _Rank>
inline T getvalue(concurrency::array<T, _Rank>& a, concurrency::index<_Rank>& i)
{
	concurrency::extent<_Rank> e;
	concurrency::index<_Rank> zero;
	for (unsigned int j = 0; j < _Rank; ++j)
	{
		e[j] = 1;
		zero[j] = 0;
	}
	return a.section(concurrency::grid<_Rank>(i, e))[zero];
}

template <class T>
void memset(concurrency::array<T>& a, unsigned int offset, T value, unsigned int count)
{
	concurrency::extent<1> e(count);
	concurrency::grid<1> g(e);

	concurrency::parallel_for_each(g, [=, &a](concurrency::index<1> idx) restrict(amp)
	{
		a[idx.x + offset] = value;
	});
}

template <class T>
void memcpy(concurrency::array<T>& dst, unsigned int dstOffset, const concurrency::array<T>& src, unsigned int srcOffset, unsigned int count)
{
	concurrency::extent<1> e(count);
	concurrency::grid<1> g(e);

	concurrency::parallel_for_each(g, [=, &dst, &src](concurrency::index<1> idx) restrict(amp)
	{
		dst[idx.x + dstOffset] = src[idx.x + srcOffset];
	});
}
