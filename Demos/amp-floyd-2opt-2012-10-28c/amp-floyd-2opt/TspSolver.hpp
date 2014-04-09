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
#include <string>

#include "solver.hpp"
#include "TspOptions.hpp"

namespace slv
{
	// This class provides glue code for the tsp runner framework
	// Used by main().

	class TspSolver : public Solver
	{
	public:
		TspSolver();

		virtual void set_acc();
		virtual void set_params();
		virtual void solve(const pbl::Tsp_container& tsp);
		virtual void solve(const std::vector<pbl::Tsp_container>& tsps);

		virtual ~TspSolver();

		void getSolution( std::vector<int>& tour );
		void set_params(opt::TspOptions& options);
		const std::string& getDescription() const;

	private:
		concurrency::accelerator acc;
		std::vector<int> tour;
		opt::TspOptions options;
		std::string description;

		// pick solver function from options selections.
		typedef opt::cost_type (*TspSolverFunction)( std::vector<int>& tour, 
				const pbl::Tsp_container& tsp, 
				const opt::TspOptions& options);

		TspSolverFunction setupOptions( const pbl::Tsp_container& tsp, 										  
										const std::vector<opt::cost_type>& w, int n);
		void displaySummary(const std::vector<int>& tour, 
			const std::vector<opt::cost_type>& w,
			int n, const pbl::Tsp_container& tsp );

	};

} // namespace slv
