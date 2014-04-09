#include "stdafx.h"
#include "Timer.h"

using namespace Bisque;

// Initialize resolution of the timer
LARGE_INTEGER Timer::m_freq = (QueryPerformanceFrequency(&Timer::m_freq), Timer::m_freq);

// Calculate overhead of the timer
LONGLONG Timer::m_overhead = Timer::GetOverhead();
