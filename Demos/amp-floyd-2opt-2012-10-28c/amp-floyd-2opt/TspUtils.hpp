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

#include "TspOptions.hpp"
#include "problem.hpp"
#include <ostream>

namespace utl
{
	//----------------------------------------------------------------------------
	// Returns the total length of a tour using distance matrix 'w'.
	//----------------------------------------------------------------------------

	opt::cost_type getTourLength( const std::vector<opt::cost_type>& w, const std::vector<int>::const_iterator& tour, int n );

	//----------------------------------------------------------------------------
	// Returns the total length of a partial tour using distance matrix 'w'.
	//----------------------------------------------------------------------------

	opt::cost_type getPartialTourLength( const std::vector<opt::cost_type>& w, const std::vector<int>& tour, int n, bool close );

	//----------------------------------------------------------------------------
	// Prints out a tour that corresponds to a solution.
	//----------------------------------------------------------------------------

	void printSolution( const std::vector<int>& tour, int nodesPerLine = 10 );

	//----------------------------------------------------------------------------
	// Writes out a tour that corresponds to a solution.
	//----------------------------------------------------------------------------

	void writeSolution( std::ostream& ostr, const std::vector<int>& tour, const pbl::Tsp_container& tsp, const std::string& comment );

	//----------------------------------------------------------------------------
	// Source: Parallel Reduction using C++ AMP
	// http://blogs.msdn.com/b/nativeconcurrency/archive/2012/03/08/parallel-reduction-using-c-amp.aspx
	//
	// Helper function comparing floating point numbers within a given relative
	// difference.
	//----------------------------------------------------------------------------

	bool fp_equal(float a, float b, float max_rel_diff);

	//----------------------------------------------------------------------------
	// Helper function for rounding a float.
	//----------------------------------------------------------------------------

	float roundFloat(float r);

	//----------------------------------------------------------------------------
	// Returns true if 'w' is symmetrical, i.e [i,j] == [j,i]
	//----------------------------------------------------------------------------

	bool isSymmetrical(const std::vector<opt::cost_type>& w, int n);

	//----------------------------------------------------------------------------
	// Source: http://en.wikipedia.org/wiki/Fisher-Yates_Shuffle
	//
	// Fisher-Yates shuffle, as implemented by Durstenfeld, "inside-out" version:
	//	To initialize an array a of n elements to a randomly shuffled copy of source, both 0-based:
	//		a[0] ← source[0]
	//		for i from 1 to n − 1 do
	//		  j ← random integer with 0 ≤ j ≤ i
	//		  a[i] ← a[j]
	//		  a[j] ← source[i]
	// 
	// Instead of initializing out_s1[i,j] to j we can use 
	// the "inside-out" algorithm using i for source[i].
	//----------------------------------------------------------------------------

	void init_array_with_shuffled_indices( std::vector<int>::iterator outBegin, std::vector<int>::iterator outEnd, int n );

	//----------------------------------------------------------------------------
	// Set w[i][i] to COST_TYPE_MAX 
	//----------------------------------------------------------------------------

	void infinitize( std::vector<opt::cost_type>& w, int n );

	//----------------------------------------------------------------------------
	// Set w[i][column] to COST_TYPE_MAX and visited[column] to true.
	// Used by tsp_floyd_gpu().
	//----------------------------------------------------------------------------

	void markVisited( std::vector<opt::cost_type>& w, int n, int column, std::vector<bool>& visited );

	//----------------------------------------------------------------------------
	// Tests converting an ATSP weight matrix to the corresponding TSP weight matrix.
	// see http://en.wikipedia.org/wiki/Travelling_salesman_problem#Solving_by_conversion_to_symmetric_TSP
	//----------------------------------------------------------------------------

	void convertTSPtoATSPMatrix(const std::vector<opt::cost_type>& w, int n,
								std::vector<opt::cost_type>&out);

	//----------------------------------------------------------------------------
	// Tests converting a tour based on a TSP weight matrix to the corresponding ATSP tour.
	// see http://en.wikipedia.org/wiki/Travelling_salesman_problem#Solving_by_conversion_to_symmetric_TSP
	//----------------------------------------------------------------------------

	bool convertTSPToATSPTour(	const std::vector<opt::cost_type>&w, 
								const std::vector<int>&tour, 
								std::vector<int>& outTour, int n,
								bool verbose);

} // namespace utl
