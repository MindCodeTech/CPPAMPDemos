#pragma once
#include <windows.h>
#include <amp.h>

using namespace concurrency;

class Timer
{
 private:
    LARGE_INTEGER m_Start;
    LARGE_INTEGER m_Stop;
    LARGE_INTEGER m_Freq;
    __int64 m_Overhead;
public:
    Timer()
    {
        QueryPerformanceFrequency(&m_Freq);
        start();
        stop();
        m_Overhead = m_Stop.QuadPart - m_Start.QuadPart;
    }

    void start()
    {
        QueryPerformanceCounter(&m_Start);
    }

    void stop()
    {
        QueryPerformanceCounter(&m_Stop);
    }

    // return seconds
    double read()
    {
        return double(m_Stop.QuadPart - m_Start.QuadPart - m_Overhead) / m_Freq.QuadPart;
    }
};

class gpu_timer : private Timer
{
public:
	gpu_timer() : _accl(accelerator::default_accelerator) {}
	explicit gpu_timer(accelerator& accl) : _accl(accl) {}

	inline void start()
	{
		_accl.default_view.wait();
		Timer::start();
	}

	inline void stop()
	{
		_accl.default_view.wait();
		Timer::stop();
	}

	inline double read()
	{
		return Timer::read();
	}
	
private: 
	accelerator _accl;
};

