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
#include "Tsp2OptSolver.hpp"
#include "problem.hpp"
#include <map>

namespace gas
{
	// see http://www.sgi.com/tech/stl/Map.html

	struct lt_cost_type
	{
		inline bool operator()(opt::cost_type a, opt::cost_type b) const
		{
			return a < b;
		}
	};

	typedef std::map<opt::cost_type, std::vector<int>, lt_cost_type> Solutions;

	typedef std::pair< opt::cost_type,std::vector<int> > SolutionsPair;


	//----------------------------------------------------------------------------
	// Returns the total length of a tour using distance matrix 'w'.
	//----------------------------------------------------------------------------

	opt::cost_type tsp_ga( std::vector<int>& tour, 
					 const pbl::Tsp_container& tspContainer, 
					 const opt::TspOptions& options,
					 Solutions& solutions,
					 two::TwoOptFunction fn);

	//----------------------------------------------------------------------------
	// Returns the total length of a tour using distance matrix 'w'.
	// TspSolverFunction version.
	//----------------------------------------------------------------------------

	opt::cost_type tsp_ga( std::vector<int>& tour, 
					 const pbl::Tsp_container& tspContainer, 
					 const opt::TspOptions& options);
} // gas