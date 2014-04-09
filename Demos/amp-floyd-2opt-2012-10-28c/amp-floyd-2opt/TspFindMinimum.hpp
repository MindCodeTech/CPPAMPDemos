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


namespace fmn
{
	//----------------------------------------------------------------------------
	// Finds the next column using these rules:
	// 1. Return columns with vertices if there are any.
	// 2. Return column with the smallest weight.
	//
	// This method is slow and does not yield better results - use findMinimum_cpu.
	//----------------------------------------------------------------------------

	int findNext( const std::vector<opt::cost_type>& w,
				int start,
				int n,
				std::vector<int>& next,
				const std::vector<bool>& visited);

	//----------------------------------------------------------------------------
	// Finds the minimum in a row using AMP. 
	// About 10x slower than findMinimum_cpu.
	//----------------------------------------------------------------------------

	int findMinimum_gpu( 
				const std::vector<opt::cost_type>& w,
				int start,
				int n,
				std::vector<int>& next,
				const std::vector<bool>& visited);

	//----------------------------------------------------------------------------
	// Find the minimum using std::min_element.
	// Returns column.
	// Uses std::min_element and is very fast.
	//----------------------------------------------------------------------------

	int findMinimum_cpu( const std::vector<opt::cost_type>& w,
				int start,
				int n,
				std::vector<int>& next,
				const std::vector<bool>& visited);

} // namespace fmn
