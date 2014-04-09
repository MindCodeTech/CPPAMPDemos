/***********************************************************************************************************
 * Copyright (c) 2012 Veikko Eeva <veikko@... see LinkedIn etc.>										   *
 *																										   *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software		   *
 * and associated documentation files (the "Software"), to deal in the Software without restriction,	   *
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,			   *
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is		   *
 * furnished to do so, subject to the following conditions:												   *
 * 																										   *
 * The above copyright notice and this permission notice shall be included in all copies or				   *
 * substantial portions of the Software.																   *
 *																										   *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT	   *
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, *
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE	   *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.												   *
 *																										   *
 ***********************************************************************************************************/

/***********************************************************************************************************
 * This code was written to participate to a competition organised by Beyond3D forums and supported by     *
 * AMD and Microsoft. See more at http://www.beyond3d.com/content/articles/121/. My sincere thanks         *
 * for the inducement to learn many new things.                                                            *
 *                                                                                                         * 
 ***********************************************************************************************************/

#pragma once

#include <cstdint>
#include <chrono>

//
//Special thanks to the authors of TC1/SC22/WG21 n2661 document: Howard E. Hinnant, Walter E. Brown, Jeff Garland and Marc Paterno.
//Document can be located at: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2661.htm.
//
//There are at least two issues in VS 2012 implementation, see at
//https://connect.microsoft.com/VisualStudio/feedback/details/719443/c-chrono-headers-high-resolution-clock-does-not-have-high-resolution
//and
//https://connect.microsoft.com/VisualStudio/feedback/details/752794/std-chrono-duration-cast-lacks-double-support.

namespace timers
{
	//Modeled according to C# StopWatch: http://msdn.microsoft.com/en-us/library/system.diagnostics.stopwatch.aspx.
	//TODO: Fix commenting to Doxygen or, gasp, MS specific if putting into wider use.
	//Note that as of yet this isn't thread-safe implementation...
	class stopwatch
	{
		public:
			typedef std::chrono::high_resolution_clock clock;
			typedef clock::time_point time_point;
			typedef clock::period period;
			typedef std::chrono::duration<std::uint64_t, period> duration;
			
			stopwatch();
			~stopwatch();

			//Starts, or resumes, measuring elapsed time for an interval.
			void start();
			
			//Stops measuring elapsed time for an interval.
			void stop();

			//Stops time interval measurement, resets the elapsed time to zero, and starts measuring elapsed time.
			void restart();
			
			//Stops time interval measurement and resets the elapsed time to zero.
			void reset();
			
			//Gets the total elapsed time measured by the current instance.
			duration elapsed() const;

		private:
			clock timer_;
			time_point start_point_;
			time_point end_point_;
			bool is_running_;

			void start_measuring();
			void stop_measuring();
	};

	//Some convenience stuff...
	namespace
	{		
		std::uint64_t to_nanoseconds(timers::stopwatch::duration const& duration) { return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count(); }
		double to_microseconds(timers::stopwatch::duration const& duration) { return to_nanoseconds(duration) * 1e-3; }
		double to_milliseconds(timers::stopwatch::duration const& duration) { return to_nanoseconds(duration) * 1e-6; }
		double to_seconds(timers::stopwatch::duration const& duration) { return to_nanoseconds(duration) * 1e-9; }
	}
}

