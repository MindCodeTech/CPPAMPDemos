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
#include "TspUtils.hpp"
#include "problem.hpp"

namespace flw
{
	//----------------------------------------------------------------------------
	// Calculate Floyd-Warshall using AMP and return the best known solution.
	// The tour to the corresponding solution is stored in 'tour'.
	//
	// tsp_floyd_gpu is signifianctly faster than tsp_floyd_cpu even for n < 2000.
	//----------------------------------------------------------------------------

	opt::cost_type tsp_floyd_gpu( std::vector<int>& tour, 
								const pbl::Tsp_container& tspContainer, 
								const opt::TspOptions& options  );

	//----------------------------------------------------------------------------
	// Calculates Floyd-Warshall using the standard, serial method and return the best known solution.
	// The tour to the corresponding solution is stored in 'tour'.
	// 
	// tsp_floyd_cpu is slower than tsp_floyd_gpu and only wins for n < 400.
	//----------------------------------------------------------------------------

	opt::cost_type tsp_floyd_cpu( std::vector<int>& tour, 
								const pbl::Tsp_container& tspContainer, 
								const opt::TspOptions& options  );

	//----------------------------------------------------------------------------
	// Floyd-Warshall core function (serial version).
	// http://en.wikipedia.org/wiki/Floyd_Warshall
	//----------------------------------------------------------------------------

	void tsp_calculate_floyd_cpu(std::vector<opt::cost_type>& w, std::vector<int>& next, int n);

	//----------------------------------------------------------------------------
	// Adds vertices between two edges 'i' and 'j' using matrix 'w' 
	// calculated from Floyd-Warshall with path reconstruction.
	// The path reconstruction information is provided by 'next'.
	// 'addVertices' marks nodes as visited as it recursively scans for 
	// vertices of vertices.
	//
	// For more detail see:
	// http://en.wikipedia.org/wiki/Floyd-Warshall#Path_reconstruction
	//
	// Used by tsp_floyd_gpu.
	//----------------------------------------------------------------------------

	opt::cost_type addVertices( const std::vector<opt::cost_type>& weightMatrix,
						   std::vector<opt::cost_type>& w, 
						   const std::vector<int>& next, 
						   int n, int i, int j,
						   std::vector<int>& outTour,
						   std::vector<bool>& visited);
} // namespace flw