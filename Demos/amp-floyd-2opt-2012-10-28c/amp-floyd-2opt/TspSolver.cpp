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

#include <cstdlib>				// For std::rand.
#include <iostream>				// For std::cin and std::cout.
#include <ctime>				// For std::clock.
#include <assert.h>

#include "helper_functions.hpp"
#include "problem.hpp"
#include "TspOptions.hpp"
#include "TspSolver.hpp"
#include "TspFloydSolver.hpp"
#include "Tsp2OptSolver.hpp"
#include "TspGASolver.hpp"
#include "TspTests.hpp"
#include "TspUtils.hpp"
#include "TspNearestNeighbor.hpp"

//----------------------------------------------------------------------------
// Namespaces
//----------------------------------------------------------------------------

using namespace concurrency;
using namespace hlp;
using namespace pbl;
using namespace slv;
using namespace std;
using namespace flw;
using namespace two;
using namespace opt;
using namespace gas;
using namespace tst;
using namespace utl;
using namespace nnb;

//----------------------------------------------------------------------------
// TspSolver constructor (empty)
//----------------------------------------------------------------------------

TspSolver::TspSolver()
{
    // initialize random seed
    srand ( static_cast<unsigned int>(time(NULL)) );
}

//----------------------------------------------------------------------------
// TspSolver destructor (empty)
//----------------------------------------------------------------------------

TspSolver::~TspSolver()
{
}

//----------------------------------------------------------------------------
// set_acc allows the user to set the accelerator at runtime.
//----------------------------------------------------------------------------

void TspSolver::set_acc(void)
{
	const auto is_cpu_acc = [ ](const accelerator& acc) { return acc.device_path == accelerator::cpu_accelerator; };
	
	auto acc_vec = accelerator::get_all();
	acc_vec.erase(remove_if(begin(acc_vec), end(acc_vec),is_cpu_acc), end(acc_vec)); // Filter out cpu_accelerator and count valid accelerators.

	if( acc_vec.size() == 1 )
		acc = acc_vec.at(0);
	else
	{
		for (auto i = 0u; i != acc_vec.size(); ++i) {
			wcout << i << ". " << acc_vec.at(i).description << '\n';
		}
	
		cout << "Select choice: ";
		size_t choice = acc_vec.size() + 1;
		while (cin >> choice) {
			if (choice < acc_vec.size()) {
				acc = acc_vec.at(choice);
				break;
			}
			else {
				continue;
			}
		}
	}

	acc = hlp::disable_tdr(acc);
	
	wcout << "Currently chosen accelerator: " << acc.description << endl;
}

//----------------------------------------------------------------------------
// set_params is part of the Solver Sample API.
//----------------------------------------------------------------------------

// Why not using set_params for passing the command line options?
// i.e. set_params(int argc, const char* argv[] )

void TspSolver::set_params(void)
{
}

//----------------------------------------------------------------------------
// This set_params version is used for setting TspOptions.
// Called by TspRunner and ???
//----------------------------------------------------------------------------

void TspSolver::set_params(opt::TspOptions& opt)
{
	options = opt;
}

//----------------------------------------------------------------------------
// solve is part of the Solver Sample API.
// Called by TspRunner and ???
//----------------------------------------------------------------------------

TspSolver::TspSolverFunction TspSolver::setupOptions(const Tsp_container& tsp, 										  
										  const std::vector<cost_type>& w, int n)
{
	TspSolverFunction solverFn = nullptr;

	description.clear();

	// it would be nice if Tsp_container told me whether the TSP is symmetrical...
	options.isSymmetrical = isSymmetrical(w,n);

	// These are special settings for the Beyond3d contest.
	const bool useTweaksForContest = true;
	if(useTweaksForContest)
	{
		// let's display something while we are crunching..,
		options.verbose = true;

		const unsigned int largeDataSet = 1000;
		if( !options.isSymmetrical )
		{
			// ATSP!

			// Timeout values adjusted so they can be compared with Veikko's ATSP solver
			if( options.timeOutInMS == -1 )
			{
				if( n > 600 )
				{
					options.timeOutInMS = -1;
				}
				else if( n >= 450 )
				{
					// Group: 450,500 
					options.timeOutInMS = 220 * 1000;
				}
				else if( n >= 400 )
				{
					// Group: 400,450 
					options.timeOutInMS = 160 * 1000;
				}
				else if( n >= 350 )
				{
					// Group: 350,400
					options.timeOutInMS = 100 * 1000;
				}
				else if( n >= 300 )
				{
					// Group: 300,350 
					options.timeOutInMS = 75 * 1000;
				}
				else
				{
					// Group: 100,200 
					options.timeOutInMS = 15 * 1000;
				}
			}
			// nearest-neighbor
			options.no_floyd = true;
			// options.use_cpu = false;
			options.use_two_opt_cpu = false;
			options.use_two_opt_gpu = false;
			options.use_two_opt_tuc_cpu = false;
			options.use_two_opt_tuc_gpu = true;
		}
		else
		{
			// TSP!

			// Timeout values adjusted so they can be compared with Veikko's TSP solver
			if( options.timeOutInMS == -1 )
			{
				if( n > 8000 )
				{
					options.timeOutInMS = -1;
				}
				else if( n >= 7000 )
				{
					// Group: 7300,7500 and above
					options.timeOutInMS = 150 * 1000;
				}
				else if( n >= 4000 )
				{
					// Group: 4400,4500 
					options.timeOutInMS = 100 * 1000;
				}
				else if( n >= 2000 )
				{
					// Group: 2000,2100 
					options.timeOutInMS = 40 * 1000;
				}
				else if( n >= 1000 )
				{
					// Group: 1000,1100 
					options.timeOutInMS = 16 * 1000;
				}
				else if( n >= 500 )
				{
					// Group: 500,600 
					options.timeOutInMS = 6 * 1000;
				}
				else
				{
					// Group: 200,300 
					options.timeOutInMS = 5 * 1000;
				}
			}

			if( n >= largeDataSet )
			{
				// nearest-neighbor, 2opt-tuc-gpu
				options.no_floyd = true;
				options.use_two_opt_cpu = false;
				options.use_two_opt_gpu = false;
				options.use_two_opt_tuc_cpu = false;
				options.use_two_opt_tuc_gpu = true;
			}
			else
			{
				// TSP: floyd, nearest-neighbor, 2opt-cpu
				options.no_floyd = false;
				options.use_two_opt_cpu = true;
				options.use_two_opt_gpu = false;
				options.use_two_opt_tuc_cpu = false;
				options.use_two_opt_tuc_gpu = false;
			}
		}
	}

	if( options.no_floyd )
	{
		if( options.isSymmetrical )
		{
			description.append("nearest-neighbor");
			solverFn = tsp_nearest_neighbor;
		}
		else
		{
			description.append("2-opt");
			solverFn = tsp_two_opt;
		}
	}
	else
	{
		// Start the solving loop.
		if( options.use_cpu || !options.isSymmetrical )
		{
			// tsp_floyd_cpu calls findNearestNeighbor, which calls 
			// either two_opt_tuc_cpu or two_opt_cpu
			description.append("floyd-cpu, nearest-neighbor");
			solverFn = tsp_floyd_cpu;
		}
		else
		{
			// tsp_floyd_gpu calls findNearestNeighbor, which calls 
			// either two_opt_tuc_gpu or two_opt_gpu
			description.append("floyd-gpu, nearest-neighbor");
			solverFn = tsp_floyd_gpu;
		}
	}

	if( options.use_ga )
	{
		solverFn = tsp_ga;
	}
	else
	{
		if( options.use_two_opt_cpu )
			description.append(", 2opt-cpu");
		else if( options.use_two_opt_gpu )
		{
			if( n > TWO_OPT_GPU_MAX_CITIES )
				description.append(", 2opt-cpu");
			else
				description.append(", 2opt-gpu");
		}
		else if( options.use_two_opt_tuc_cpu )
			description.append(", 2opt-tuc-cpu");
		else if( options.use_two_opt_tuc_gpu )
			description.append(", 2opt-tuc-gpu");
	}

	if(options.timeOutInMS == -1 ) 
		description.append(", with no time out set. " );
	else
	{
		description.append(", timing out after " );
		description.append( std::to_string(options.timeOutInMS/1000.0) );
		description.append(" seconds." );
	}

	return solverFn;
}

//----------------------------------------------------------------------------
// Displays results and saves tour into file
//----------------------------------------------------------------------------

void TspSolver::displaySummary(const std::vector<int>& tour, 
							   const std::vector<cost_type>& w,
							   int n, const Tsp_container& tsp )
{
	// In my opinion the tour should always be validated at the end.
	validateTour( tour, n );

	// display summary
	const cost_type min_cost = getTourLength(w, tour.begin(), n);
	const double elapsed = options.wallClock.Elapsed(); 
	std::cout << std::endl;
	std::cout << "For problem " << tsp.problem_nam << " minimum tour cost is: " << min_cost << std::endl;
	std::cout << "Solve took " << (elapsed / 1000) << " seconds." << std::endl;
	if( options.isTimeUp() )
	{
		std::cout << "(Interrupted after " << (options.timeOutInMS / 1000) << " seconds)" << std::endl;
	}
	
	// dump solution to screen
	if( options.print_solution )
	{
		utl::printSolution(tour);
	}

	// summarize results in 'comment'
	std::string comment;

	comment.append( "Best known minimum tour cost for ");
	comment.append( tsp.problem_nam );
	comment.append( " is " );
	comment.append( std::to_string(static_cast<int>(min_cost)) );
	comment.append( " using " );
	comment.append( getDescription() );
	comment.append( " in " );
	comment.append( std::to_string(elapsed) );
	comment.append( " seconds." );

	// write out solution to *.tour file
	std::string tourFileName( options.tsp_file );

	// 'tourFileName' is empty when called from solve() for multiple TSPs.
	if( tourFileName.empty() )
	{
		tourFileName = tsp.problem_nam;
	}

	tourFileName.append( ".tour" );
	std::ofstream tourFile;
	tourFile.open ( tourFileName );
	utl::writeSolution(tourFile, tour, tsp, comment);
	tourFile.close();

	std::cout << "Solution written to '" << tourFileName << "'."<< std::endl;
	std::cout << "-----------------------------------------------" << std::endl;
}

//----------------------------------------------------------------------------
// solve is part of the Solver Sample API.
// Called by TspRunner and ???
//----------------------------------------------------------------------------

void TspSolver::solve(const Tsp_container& tsp)
{		
	const int n = tsp.problem_dim;
	const std::vector<cost_type>& w = tsp.pbl_adj_mtx;

	// sanity check that only kicks in if you use pr76.tsp
	if( options.validate && tsp.problem_nam.compare("pr76") == 0 )
		test_pr76( w, n );
	
	TspSolverFunction solverFn = setupOptions(tsp, w, n);

	// sanity check.
	if( solverFn == nullptr )
	{
		std::cerr << "Error: cannot find appropriate solver function." << std::endl << std::endl;
		options.help();
		return;
	}

	std::cout << "amp-floyd-2opt: solving " << tsp.problem_nam <<  " with " << description << std::endl;

	options.wallClock.Start();

	// solverFn(tour, tsp, options);

	if( options.isSymmetrical )
	{
		// run the solver function;
		solverFn(tour, tsp, options);

		// bparadie, 2012-10-27: if there is time left use our expensive GA algorithm!
		if( !options.isTimeUp() )
		{
			std::cout << "amp-floyd-2opt: using tsp-ga for the remaining " << options.remainingTime() << " ms." << std::endl;
			while( !options.isTimeUp() )
				tsp_ga(tour, tsp, options);
		}
	}
	else
	{
		// prepare atsp
		std::vector<int> atspTour;
		pbl::Tsp_container atsp = tsp;
		atsp.problem_dim = n*2;
		infinitize(atsp.pbl_adj_mtx,n);

		// "inflate" TSP matrix, 
		// see http://en.wikipedia.org/wiki/Travelling_salesman_problem#Solving_by_conversion_to_symmetric_TSP
		std::vector<cost_type>out;
		convertTSPtoATSPMatrix(atsp.pbl_adj_mtx, n, out);
		atsp.pbl_adj_mtx.swap(out);

		// run the solver function;
		solverFn(atspTour, atsp, options);

		// This is a shot in the dark. The results from the contest came back and
		// my ATSP is failing. Maybe it's the tsp-ga part?
		/*
		// bparadie, 2012-10-27: if there is time left use our expensive GA algorithm!
		if( !options.isTimeUp() )
		{
			std::cout << "amp-floyd-2opt: using tsp-ga for the remaining " << options.remainingTime() << " ms." << std::endl;
			while( !options.isTimeUp() )
				tsp_ga(atspTour, atsp, options);
		}
		*/

		// "deflate" TSP tour, 
		// see http://en.wikipedia.org/wiki/Travelling_salesman_problem#Solving_by_conversion_to_symmetric_TSP
		const bool succeeded = convertTSPToATSPTour(atsp.pbl_adj_mtx, atspTour, tour, n, options.verbose);

		// There are many cases where convertTSPToATSPTour() fails.
		// In those cases we will display the results and let the user know that we failed
		// to reconstruct the ATSP tour the TSP tour.
		if( !succeeded )
		{
			std::cout << "***" << std::endl;
			std::cout << "ERROR: Unable to reconstruct the ATSP tour from the TSP tour." << std::endl;
			std::cout << "The length of the solution is accurate, but the tour is not."  << std::endl;
			std::cout << "***" << std::endl;

			displaySummary(atspTour, atsp.pbl_adj_mtx, n*2, tsp);
			return;
		}

		if( options.validate )
		{
			validateTour( atspTour, n*2 );
			validateTour( tour, n );
			cost_type len1 = getTourLength(atsp.pbl_adj_mtx,atspTour.begin(),n*2);
			cost_type len2 = getTourLength(tsp.pbl_adj_mtx,tour.begin(),n);
			// assert( roundFloat(len1) == roundFloat(len2) );
		}
	}

	options.wallClock.Stop();
	displaySummary(tour, w, n, tsp);
}

//----------------------------------------------------------------------------
// solve is part of the Solver Sample API.
// Solves multple TSPs.
//----------------------------------------------------------------------------

void TspSolver::solve(const vector<Tsp_container>& tsps)
{
	for (const auto& problem : tsps) {
		solve(problem);
	}
}

//----------------------------------------------------------------------------
// Returns a description of the methods used during the run, which
// will get added to the Comment section of a tour saved by writeSolution()
//----------------------------------------------------------------------------

const std::string& TspSolver::getDescription() const
{
	return description;
}

//----------------------------------------------------------------------------
// Returns the solution.
//----------------------------------------------------------------------------

void TspSolver::getSolution( std::vector<int>& outTour )
{
	outTour = tour;
}

	