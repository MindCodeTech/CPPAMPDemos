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

#pragma once

#include <vector>
#include <float.h>
#include "timer.hpp"
#include "problem.hpp"	// Tsp_container

namespace opt
{
	//----------------------------------------------------------------------------
	// Type definitions 
	//----------------------------------------------------------------------------

	typedef float cost_type;
	typedef std::vector<cost_type> Tour;

	//----------------------------------------------------------------------------
	// Constants
	//----------------------------------------------------------------------------

	const cost_type COST_TYPE_MAX = FLT_MAX;
	const size_t NO_PATH = -1;
	const int NO_PATH_INT = -1;

	// The tile size used in this project.
	#define TILE_SIZE 16

	//----------------------------------------------------------------------------
	// 'tsp_options' for the application's settings.
	//----------------------------------------------------------------------------

	class TspOptions
	{
		bool use_defaults;

	public:
		
		explicit TspOptions();

		// Use tsp_floyd_cpu()
		// Default is false.
		bool use_cpu;

		// Use tsp_floyd_gpu()
		// Default is true.
		bool use_gpu;

		// Verbose output
		// Default is false.
		bool verbose;

		// Validate results (slower)
		// Default is false.
		bool validate;
		// Print solution at the end
		// Default = false
		bool print_solution;


		// Uses text-book 2-Opt Algorithm on CPU 
		// Default is false
		bool use_two_opt_cpu;

		// Uses text-book 2-Opt Algorithm on GPU 
		// Default is false
		bool use_two_opt_gpu;

		// Uses TUC 2-Opt Algorithm on CPU 
		// Default is true
		bool use_two_opt_tuc_cpu;

		// Uses TUC 2-Opt Algorithm on GPU 
		// Default is false
		bool use_two_opt_tuc_gpu;

		// Uses GA Algorithm 
		// Default is false
		bool use_ga;

		// Skips Floyd-Warshall Algorithm 
		// Default is false
		bool no_floyd;

		// Returns true if the TSP problem is symmetrical
		// Default is true, will get set by TspRunner::run()
		bool isSymmetrical;

		// TSP file
		std::string tsp_file;

		// parses the command line args
		bool parse( int argc, const char * argv[] );

		// prints out help
		int help() const;

		// assignment operator
		TspOptions& operator=( const TspOptions& other );

		// time out value in milliseconds, -1 means no timeout
		double timeOutInMS;

		// wall clock
		Timer wallClock;

		// Returns true if the wallClock has passed timeOutInMS.
		bool isTimeUp() const;

		// Returns the remaining time until time is up.
		double remainingTime() const;
	};

} // Namespace opt