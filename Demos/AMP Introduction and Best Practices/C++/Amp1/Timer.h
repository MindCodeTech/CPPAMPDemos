/*
// Timer.h
// Utility to measure performance time
//
// Usage:

	const int reps = 500;
	Timer t;

	t.Start();

	for (int r = 1; r <= reps; ++r)
	{
//#pragma loop(no_vector)
		for (int n = 0; n< N; ++n)
		{
			a[n] = sin(b[n]) * cos(b[n]);
		}
	}

	t.Stop();

	cout << "Time in milliseconds = " << t.Elapsed() / (double)reps << endl;
*/

#pragma once

#include "stdafx.h"

namespace Bisque
{
	struct Timer
	{
	public:
		void Start()
		{
			QueryPerformanceCounter(&m_start);
		}

		void Stop()
		{
			QueryPerformanceCounter(&m_stop);
		}

		// Returns elapsed time in milliseconds (ms)
		double Elapsed()
		{
			return (m_stop.QuadPart - m_start.QuadPart - m_overhead) * 1000.0 / m_freq.QuadPart;
		}

	private:
		static LONGLONG GetOverhead()
		{
			Timer t;
			t.Start();
			t.Stop();

			return t.m_stop.QuadPart - t.m_start.QuadPart;
		}

	private:
		LARGE_INTEGER			m_start;
		LARGE_INTEGER			m_stop;
		static LARGE_INTEGER	m_freq;
		static LONGLONG			m_overhead;
	};
}