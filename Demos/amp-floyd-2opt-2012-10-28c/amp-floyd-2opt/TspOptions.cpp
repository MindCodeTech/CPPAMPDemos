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
// Includes
//----------------------------------------------------------------------------

#include "TspOptions.hpp"
#include "Tsp2OptSolver.hpp"

#include <iostream>	// std::cout

//----------------------------------------------------------------------------
// Namespaces
//----------------------------------------------------------------------------

using namespace opt;
using namespace two;

//----------------------------------------------------------------------------
// Constructor, sets defaults.
//----------------------------------------------------------------------------

TspOptions::TspOptions()
{
	// initialize options with the defaults
	use_defaults = true;
	use_cpu = false;
	use_gpu = true;
	verbose = false;
	validate = false;
	print_solution = false;
	use_two_opt_cpu = false;
	use_two_opt_gpu = false;
	use_two_opt_tuc_cpu = true;
	use_two_opt_tuc_gpu = false;
	use_ga = false;
	no_floyd = false;
	isSymmetrical = true;
	timeOutInMS = -1;
}

//----------------------------------------------------------------------------
// Assignment operator
//----------------------------------------------------------------------------

TspOptions& TspOptions::operator=( const TspOptions& other )
{
	// initialize options with the defaults
	use_defaults = other.use_defaults;
	use_cpu = other.use_cpu;
	use_gpu = other.use_gpu;
	verbose = other.verbose;
	validate = other.validate;
	print_solution = other.print_solution;
	use_two_opt_cpu = other.use_two_opt_cpu;
	use_two_opt_gpu = other.use_two_opt_gpu;
	use_two_opt_tuc_cpu = other.use_two_opt_tuc_cpu;
	use_two_opt_tuc_gpu = other.use_two_opt_tuc_gpu;
	use_ga = other.use_ga;
	no_floyd = other.no_floyd;
	tsp_file = other.tsp_file;
	isSymmetrical = other.isSymmetrical;
	timeOutInMS = other.timeOutInMS;
	wallClock = other.wallClock;

	return *this;
}

//----------------------------------------------------------------------------
// Prints out the help with options.
// Used by main().
//----------------------------------------------------------------------------

int TspOptions::help() const
{
	const TspOptions& options = *this;

	std::cout << "Usage: tsp-ga [options] tsp-file" << std::endl;	
	std::cout << std::endl;
	std::cout << "For fast execution, but less precize results try:" << std::endl;
	std::cout << "\t tsp-ga -verbose -use-gpu -use-two-opt-tuc-cpu tsp-file" << std::endl;	
	std::cout << std::endl;
	std::cout << "For more precize results, but slow execution try:" << std::endl;
	std::cout << "\t tsp-ga -verbose -use-gpu -use-two-opt-cpu tsp-file" << std::endl;	

	std::cout << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "\t -use-cpu"
			  << "\t use cpu calculation." 
			  << " (Default=" << (options.use_cpu ? "true" : "false") << ")" << std::endl;
	std::cout << "\t -use-gpu"
			  << "\t use gpu calculation." 
			  << " (Default=" << (options.use_gpu ? "true" : "false") << ")" << std::endl;
	std::cout << "\t -use-both"
			  << "\t use gpu and cpu calculation."
			  << " (Default=" << ((options.use_cpu && options.use_gpu) ? "true" : "false") << ")" << std::endl;
	std::cout << "\t -verbose"
			  << "\t verbose output."
			  << " (Default=" << (options.verbose ? "true" : "false") << ")" << std::endl;
	std::cout << "\t -validate"
			  << "\t validate results."
			  << " (Default=" << (options.validate ? "true" : "false") << ")" << std::endl;
	std::cout << "\t -print-solution"
			  << "\t prints solution."
			  << " (Default=" << (options.print_solution ? "true" : "false") << ")" << std::endl;
	std::cout << "\t -use-two-opt-cpu"
			  << "\t uses text-book 2-opt algorithm on CPU"
			  << " (Default=" << (options.use_two_opt_cpu ? "true" : "false") << ")" << std::endl;
	std::cout << "\t -use-two-opt-gpu"
			  << "\t uses text-book 2-opt algorithm on GPU"
			  << " (Default=" << (options.use_two_opt_gpu ? "true" : "false") << ")" << std::endl;
	std::cout << "\t -use-two-opt-tuc-cpu"
			  << "\t uses TUC 2-opt algorithm on CPU"
			  << " (Default=" << (options.use_two_opt_tuc_cpu ? "true" : "false") << ")" << std::endl;
	std::cout << "\t -use-two-opt-tuc-gpu"
			  << "\t uses TUC 2-opt algorithm on GPU"
			  << " (Default=" << (options.use_two_opt_tuc_gpu ? "true" : "false") << ")" << std::endl;
	std::cout << "\t -use-ga"
			  << "\t use genetic algorithm (GA)." 
			  << " (Default=" << (options.use_ga ? "true" : "false") << ")" << std::endl;
	std::cout << "\t -no-floyd"
			  << "\t use Floyd-Warshall." 
			  << " (Default=" << (options.no_floyd ? "true" : "false") << ")" << std::endl;	
	std::cout << "\t -timeout <n>"
			  << "\t let solver time out at n milliseconds." 
			  << " (Default=" << (options.timeOutInMS == -1 ? 
					"(no timeout)" : std::to_string(options.timeOutInMS)) << ")" << std::endl;	

	std::cout << std::endl;
	return -1;
}

//----------------------------------------------------------------------------
// Prints out the help with options.
// Used by main().
//----------------------------------------------------------------------------

bool TspOptions::parse( int argc, const char * argv[] )
{
	TspOptions& options = *this;
	
	// we require at least one parameter, the tsp-file.
	if( argc < 2 )
		return false;


	bool printHelp = false;

	// parses argv except for argv[0] and argv[argc-1]
	// The tsp-file path is expected in argv[argc-1].

	for( int i = 1; i < (argc-1); i++ )
	{
		if( strcmp(argv[i], "-help") == 0 )
			printHelp = true; 
		else if( strcmp(argv[i], "-verbose") == 0 )
			options.verbose = true;
		else if( strcmp(argv[i], "-validate") == 0 ) 
			options.validate = true;
		else if( strcmp(argv[i], "-use-cpu") == 0 )
		{
			options.use_cpu = true;
			options.use_gpu = false;
		}
		else if( strcmp(argv[i], "-use-gpu") == 0 )
		{
			options.use_cpu = false;
			options.use_gpu = true;
		}
		else if( strcmp(argv[i], "-use-both") == 0 )
		{
			options.use_cpu = true;
			options.use_gpu = true;
		}
		else if( strcmp(argv[i], "-print-solution") == 0 )
		{
			options.print_solution = true;
		}
		else if( strcmp(argv[i], "-use-two-opt-cpu") == 0 )
		{
			options.use_two_opt_cpu = true;
			options.use_two_opt_gpu = false;
			options.use_two_opt_tuc_cpu = false;
			options.use_two_opt_tuc_gpu = false;
		}
		else if( strcmp(argv[i], "-use-two-opt-gpu") == 0 )
		{
			options.use_two_opt_cpu = false;
			options.use_two_opt_gpu = true;
			options.use_two_opt_tuc_cpu = false;
			options.use_two_opt_tuc_gpu = false;
		}
		else if( strcmp(argv[i], "-use-two-opt-tuc-cpu") == 0 )
		{
			options.use_two_opt_cpu = false;
			options.use_two_opt_gpu = false;
			options.use_two_opt_tuc_cpu = true;
			options.use_two_opt_tuc_gpu = false;
		}
		else if( strcmp(argv[i], "-use-two-opt-tuc-gpu") == 0 )
		{
			options.use_two_opt_cpu = false;
			options.use_two_opt_gpu = false;
			options.use_two_opt_tuc_cpu = false;
			options.use_two_opt_tuc_gpu = true;
		}
		else if( strcmp(argv[i], "-use-ga") == 0 )
		{
			options.use_ga= true;
		}
		else if( strcmp(argv[i], "-no-floyd") == 0 )
		{
			options.no_floyd = true;
		}
		else if( strcmp(argv[i], "-timeout") == 0 )
		{
			i++;
			if( i < (argc-1) )
				options.timeOutInMS = strtod(argv[i], nullptr);
		}
		else
		{
			std::cout << "Unknown option: " << argv[i] << std::endl << std::endl;
			return false;
		}
	}

	// For more details see:
    // Data: http://www.iwr.uni-heidelberg.de/groups/comopt/software/TSPLIB95/tsp/
    // Solutions: http://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/STSP.html
	//
	// Here are a few example solutions:
	//		pr76.tsp = 108159
	//		pr144.tsp = 58537
	//		pr439.tsp = 107217
	//		pr1002.tsp = 259045
	//		pr2392.tsp = 378032
	options.tsp_file = argv[argc-1];

	if( printHelp )
		help();

	// print options in verbose mode
	if( options.verbose )
	{
		std::cout << "Selected command line options:" << std::endl;
		std::cout << "\t use-cpu = " << (options.use_cpu ? "true" : "false") << std::endl;
		std::cout << "\t use-gpu = " << (options.use_gpu ? "true" : "false") << std::endl;
		std::cout << "\t verbose = " << (options.verbose ? "true" : "false") << std::endl;
		std::cout << "\t validate = " << (options.validate ? "true" : "false") << std::endl;
		std::cout << "\t print-solution = " << (options.print_solution ? "true" : "false") << std::endl;
		std::cout << "\t use-two-opt-cpu = " << (options.use_two_opt_cpu ? "true" : "false") << std::endl;
		std::cout << "\t use-two-opt-gpu = " << (options.use_two_opt_gpu ? "true" : "false") << std::endl;
		std::cout << "\t use-two-opt-tuc-cpu = " << (options.use_two_opt_tuc_cpu ? "true" : "false") << std::endl;
		std::cout << "\t use-two-opt-tuc-gpu = " << (options.use_two_opt_tuc_gpu ? "true" : "false") << std::endl;
		std::cout << "\t use-ga = " << (options.use_ga ? "true" : "false") << std::endl;
		std::cout << "\t no-floyd = " << (options.no_floyd ? "true" : "false") << std::endl;
		std::cout << "\t time-out = " << (options.timeOutInMS == -1 ? "(no timeout)" : std::to_string(options.timeOutInMS)) << std::endl;
		std::cout << std::endl;
	}

	return true;
}

//----------------------------------------------------------------------------
// Returns true if the wallClock has passed maximumRunningTime.
//----------------------------------------------------------------------------


bool TspOptions::isTimeUp() const
{
	if( timeOutInMS >= 0.0 && remainingTime() <= 0.0 )
		return true;
	
	return false;
}

//----------------------------------------------------------------------------
// Returns the remaining time until time is up.
//----------------------------------------------------------------------------

double TspOptions::remainingTime() const
{
	if( timeOutInMS >= 0.0 )
	{
		const double extraTimeToWrapEverythingUp = 50.0;
		const double elapsed = wallClock.Elapsed() + extraTimeToWrapEverythingUp;
		return timeOutInMS - elapsed;
	}
	return -1.0;
}
