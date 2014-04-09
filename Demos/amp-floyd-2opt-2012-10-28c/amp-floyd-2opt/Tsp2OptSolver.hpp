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
#include "TspOptions.hpp"
#include "problem.hpp"

// The upper limit on problem sizing is 8192 nodes;
// But unfortunately MAX_CITIES = 8192 triggers:
//		error C3565: The total amount of tile_static memory (32960 bytes) exceeds the 
//		limit of 32768 bytes when compiling the call graph for the concurrency::parallel_for_each
#define TWO_OPT_GPU_MAX_CITIES (8000) // (8192)

namespace two
{
	//----------------------------------------------------------------------------
	// All 2-opt functions are of type TwoOptFunction.
	//----------------------------------------------------------------------------
	typedef void (*TwoOptFunction) ( std::vector<int>::iterator tour, 
									 int n,
									 const std::vector<opt::cost_type>& w,
									 const opt::TspOptions& options );

	//----------------------------------------------------------------------------
	// Returns appropriate 2-opt functionfor selected -use-two-opt-* 
	//----------------------------------------------------------------------------

	TwoOptFunction getTwoOptFunction(const opt::TspOptions& options, 
					const std::vector<opt::cost_type>& w, 
					int n);

	//----------------------------------------------------------------------------
	// Based on: 
	// 2-Optimal Approximate Algorithm for the Traveling Salesman Problem
	// (M.M.Syslo)
	// ftp://www.mathematik.uni-kl.de/pub/Math/ORSEP/SYSLO.ZIP
	// http://optimierung.mathematik.uni-kl.de/old/ORSEP/contents.html
	// 
	// This method is fast for n < 1000 but is significantly slower than
	// two_opt_tuc_cpu for n > 2000 while producing more precise results.
	//----------------------------------------------------------------------------

	void two_opt_cpu( std::vector<int>::iterator tour, 
						int n,
						const std::vector<opt::cost_type>& w,
						const opt::TspOptions& options );


	//----------------------------------------------------------------------------
	// Based on:
	// Accelerating 2-opt and 3-opt Local Search Using GPU in the Travelling Salesman Problem
	// http://www.researchgate.net/publication/229476110_Accelerating_2-opt_and_3-opt_Local_Search_Using_GPU_in_the_Travelling_Salesman_Problem
	//
	// This function is the parallel version of two_opt_cpu.
	// Unfortunately this implementation is too slow - use two_opt_tuc_cpu, or two_opt_cpu.
	//----------------------------------------------------------------------------

	void two_opt_gpu( std::vector<int>::iterator tour, 
						int n,
						const std::vector<opt::cost_type>& w,
						const opt::TspOptions& options );

	//----------------------------------------------------------------------------
	// Based on:
	// Hardware Implementation of 2-Opt Local Search Algorithm for the Traveling Salesman Problem
	// http://dl.acm.org/citation.cfm?id=1263915
	// http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=4228483&tag=1
	//
	// This method is amazingly fast especialy for n > 2000 but does produces results 
	// that are less precize two_opt_tuc_cpu.
	//----------------------------------------------------------------------------

	void two_opt_tuc_cpu( std::vector<int>::iterator tour, 
						int n,
						const std::vector<opt::cost_type>& w,
						const opt::TspOptions& options );

	//----------------------------------------------------------------------------
	// Based on:
	// Hardware Implementation of 2-Opt Local Search Algorithm for the Traveling Salesman Problem
	// http://dl.acm.org/citation.cfm?id=1263915
	// http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=4228483&tag=1
	//
	// This function is the parallel version of two_opt_tuc_cpu.
	// Unfortunately this implementation is too slow - use two_opt_tuc_cpu.
	//----------------------------------------------------------------------------

	void two_opt_tuc_gpu( std::vector<int>::iterator tour, 
						int n,
						const std::vector<opt::cost_type>& w,
						const opt::TspOptions& options );

	//----------------------------------------------------------------------------
	// Finds a tour using a 2-Opt Local Search Algorithm
	//
	// (TspSolverFunction wrapper for TSPSolver)
	//----------------------------------------------------------------------------

	opt::cost_type tsp_two_opt( std::vector<int>& tour, 
								const pbl::Tsp_container& tspContainer, 
								const opt::TspOptions& options  );

 } // namespace two