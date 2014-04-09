
#include "stopwatch.h"

#include <chrono>

using timers::stopwatch;


//
//Stopwatch public functions.
//
stopwatch::stopwatch(): start_point_(clock::time_point::max()), end_point_(clock::time_point::max()), is_running_(false)
{}


stopwatch::~stopwatch()
{}


void stopwatch::start()
{
	start_measuring();
}


void stopwatch::stop()
{
	stop_measuring();
}


void stopwatch::restart()
{
	start_measuring();
}

void stopwatch::reset()
{
	is_running_ = false;
	start_point_ = clock::time_point::max();
	end_point_ = clock::time_point::max();
}


stopwatch::duration stopwatch::elapsed() const
{
	return is_running_ ? clock::now() - start_point_ : end_point_ - start_point_;
}


//
//Stopwatch private functions.
//
void stopwatch::start_measuring()
{
	start_point_ = clock::now();
	is_running_ = true;
}


void stopwatch::stop_measuring()
{
	is_running_ = false;
	end_point_ = clock::now();
}

