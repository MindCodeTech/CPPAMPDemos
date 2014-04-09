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


#pragma once
#include <windows.h>

struct Timer
{
	Timer()
	{
		m_stopped = false;
	}

    void Start() 
    {
        QueryPerformanceCounter(&m_start);
    }

    void Stop() 
    {
		m_stopped = true;
        QueryPerformanceCounter(&m_stop);
    }
    
    // Returns elapsed time in milliseconds (ms)
    double Elapsed() const
    {
		LARGE_INTEGER stop;
		if( m_stopped )
			stop = m_stop;
		else
			QueryPerformanceCounter(&stop);

        return (stop.QuadPart - m_start.QuadPart - m_overhead) \
                                          * 1000.0 / m_freq.QuadPart;
    }

	inline Timer& Timer::operator=( const Timer& other )
	{
		m_stopped = other.m_stopped;
		m_start = other.m_start;
		m_stop = other.m_stop;
		return *this;
	}

private:
	bool m_stopped;

    // Returns the overhead of the timer in ticks
    static LONGLONG GetOverhead()
    {
        Timer t;
        t.Start();
        t.Stop();
        return t.m_stop.QuadPart - t.m_start.QuadPart;
    }

    LARGE_INTEGER m_start;
    LARGE_INTEGER m_stop;
    static LARGE_INTEGER m_freq;
    static LONGLONG m_overhead;
};