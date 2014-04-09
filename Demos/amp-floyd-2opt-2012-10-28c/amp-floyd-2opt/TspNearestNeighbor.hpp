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
#include "TspUtils.hpp"
#include "TspOptions.hpp"

namespace nnb
{
	//----------------------------------------------------------------------------
	// FindPickNextFunction is used by findTour()
	//----------------------------------------------------------------------------

	typedef int (*FindNearestNeighborPickNextFunction) ( 
					const std::vector<opt::cost_type>& w,
					int start,
					int n,
					std::vector<int>& next,
					const std::vector<bool>& visited);


	//----------------------------------------------------------------------------
	// Finds a tour using the nearest-neighbor algorithm.
	//
	// TspMinimum offers three FindNearestNeighborPickNextFunctions:
	// findNext, findMinimum_gpu, and findMinimum_cpu.
	// Only findMinimum_cpu delivers acceptable performance.
	//----------------------------------------------------------------------------

	opt::cost_type findNearestNeighbor( const std::vector<opt::cost_type>& weightMatrix, 
								std::vector<int>& next, int n, 
								std::vector<int>& outTour, 
								const pbl::Tsp_container& tsp,
								const opt::TspOptions& options,
								FindNearestNeighborPickNextFunction fnPicker );

	//----------------------------------------------------------------------------
	// Finds a tour using the nearest-neighbor algorithm.
	//
	// (TspSolverFunction wrapper for TSPSolver)
	//----------------------------------------------------------------------------

	opt::cost_type tsp_nearest_neighbor( std::vector<int>& tour, 
								const pbl::Tsp_container& tspContainer, 
								const opt::TspOptions& options  );
}
