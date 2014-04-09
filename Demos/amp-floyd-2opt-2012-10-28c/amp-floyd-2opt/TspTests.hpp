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

namespace tst
{
	//----------------------------------------------------------------------------
	// Validates a tour
	//----------------------------------------------------------------------------

	bool validateTour( const std::vector<int>& tour, int n, bool printErrors = true );

	//----------------------------------------------------------------------------
	// Tests validateTour() and getTourLength() against known data from pr76.tsp
	//----------------------------------------------------------------------------

	opt::cost_type test_pr76( const std::vector<opt::cost_type>& w, int n);

	//----------------------------------------------------------------------------
	// Tests reverse in place for vectors.
	//----------------------------------------------------------------------------

	void testReverseInPlace(std::vector<int>& tour, int n, 
						int first, int left, int right, int last);

	//----------------------------------------------------------------------------
	// Tests converting an ATSP weight matrix to a TSP weight matrix.
	// see http://en.wikipedia.org/wiki/Travelling_salesman_problem#Solving_by_conversion_to_symmetric_TSP
	//----------------------------------------------------------------------------

	void convertATSPToTSP();

} // namespace tst
