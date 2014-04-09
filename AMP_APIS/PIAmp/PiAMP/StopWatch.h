class StopWatch
{
private:
    LARGE_INTEGER liStart;
    LARGE_INTEGER liEnd;
    LARGE_INTEGER liPerfFreq;

	bool running;

public:
    StopWatch(void)
		: running(false)
    {
        QueryPerformanceFrequency(&liPerfFreq);
    }

    void Start()
    {
		running = true;

        liStart.QuadPart = 0;
        QueryPerformanceCounter(&liStart);
    }

    void Stop()
    {
		running = false;

		liEnd.QuadPart = 0;
        QueryPerformanceCounter(&liEnd);
    }

    double GetDuration()
    {
		if (running)
		{
			liEnd.QuadPart = 0;
			QueryPerformanceCounter(&liEnd);
		}

		auto result = (liEnd.QuadPart - liStart.QuadPart) / double(liPerfFreq.QuadPart);
		return result;
    }

};