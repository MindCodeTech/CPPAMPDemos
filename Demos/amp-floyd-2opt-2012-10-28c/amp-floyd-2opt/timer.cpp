/*
 * 
 *	Copyright (c) 2012 Bernd Paradies
 *  http://blogs.adobe.com/bparadie/
 * 
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

//----------------------------------------------------------------------------
// Source:
//
//		High-resolution timer for C++
//		http://blogs.msdn.com/b/nativeconcurrency/archive/2011/12/28/high-resolution-timer-for-c.aspx
//
// See also:
//
//		How to measure the performance of C++ AMP algorithms?
//		http://blogs.msdn.com/b/nativeconcurrency/archive/2011/12/28/how-to-measure-the-performance-of-c-amp-algorithms.aspx
//
//		Data warm up when measuring performance with C++ AMP
//		http://blogs.msdn.com/b/nativeconcurrency/archive/2012/04/25/data-warm-up-when-measuring-performance-with-c-amp.aspx
//----------------------------------------------------------------------------

#include "timer.hpp"

// Initialize the resolution of the timer
LARGE_INTEGER Timer::m_freq = \
          (QueryPerformanceFrequency(&Timer::m_freq), Timer::m_freq);

// Calculate the overhead of the timer
LONGLONG Timer::m_overhead = Timer::GetOverhead();