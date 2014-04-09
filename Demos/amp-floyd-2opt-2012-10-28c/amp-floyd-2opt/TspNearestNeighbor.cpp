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

#include "TspNearestNeighbor.hpp"
#include "TspUtils.hpp"
#include "Tsp2OptSolver.hpp"
#include "TspGASolver.hpp"
#include "TspFindMinimum.hpp"
#include "TspFloydSolver.hpp"

#include <iostream>
#include <map>
#include <math.h>
#include <assert.h>

//----------------------------------------------------------------------------
// Namespaces
//----------------------------------------------------------------------------

using namespace opt;
using namespace nnb;
using namespace utl;
using namespace two;
using namespace gas;
using namespace fmn;
using namespace flw;

//----------------------------------------------------------------------------
// Finds a tour using the nearest-neighbor algorithm.
// See http://en.wikipedia.org/wiki/Nearest_neighbour_algorithm
//----------------------------------------------------------------------------

cost_type nnb::findNearestNeighbor( const std::vector<cost_type>& weightMatrix, 
							std::vector<int>& next, int n, 
							std::vector<int>& outTour, 
							const pbl::Tsp_container& tsp,
							const TspOptions& options,
							FindNearestNeighborPickNextFunction fnPicker )
{
	// initialize local vars.
	cost_type all_min = COST_TYPE_MAX;
	cost_type nearest_neighbor_min = COST_TYPE_MAX;
	std::vector<int> localTour;    
	localTour.reserve(n);
	outTour.reserve(n);
	outTour.clear();

	// Let's terminate after randomly scanning 70% of the rows.
	const bool isSymmetrical = options.isSymmetrical;
	const double rowCoverage = 1.0; // isSymmetrical ? 0.7 : 1.0;
	const int maxTours = static_cast<int>(n * rowCoverage);

	int counter = 0;
	std::vector<int> rows(n);
	std::generate( rows.begin(), rows.end(), [&counter]() { return counter++; } );

	// @@@
	if( isSymmetrical )
		init_array_with_shuffled_indices( rows.begin(), rows.end(), n );
	else
		std::cout << "nearest-neighbor: rows are not randomized for ATSP data sets!" << std::endl;

	Solutions solutions;

	TwoOptFunction twoOptfn = getTwoOptFunction( options, weightMatrix, n );

	// let's do at least one round
	bool isTimeUp = false;

	// iterate over all tours.
	for( int i = 0; i < maxTours && !isTimeUp; i++ )
    {
		// set initial row.
        int row = rows[i];

		// initialize local 'w' with weightMatrix and set 
		// w[i][i] to COST_TYPE_MAX ("infinitize")
		std::vector<cost_type>w(weightMatrix);
		infinitize(w,n);

		// initialize local 'visited' with false.
		std::vector<bool> visited(n);
		std::fill(visited.begin(), visited.end(), false);
			
		// initialize localTour, local_min, and row
		localTour.clear();
		cost_type local_min = 0;

		// set the start and mark this row as visited
		localTour.push_back(row);
		markVisited(w,n,row,visited);

		// measure the loop times
		Timer timer;
		timer.Start();
		
		int counter = 0;

		while( local_min < nearest_neighbor_min && !isTimeUp )
		{            
			const int minCol = fnPicker(w,row*n,n,next,visited);

			if( minCol != NO_PATH )
			{
				const cost_type rowMin = w[row*n+minCol];
				assert( minCol >= 0 && minCol < n );
				assert( !visited[minCol] );

				if( options.validate )
				{
					std::vector<cost_type>::const_iterator begin = w.begin() + (row*n);
					std::vector<cost_type>::const_iterator end = begin + n;
					const std::vector<cost_type>::const_iterator it2 = std::min_element(begin, end);
					const int minCol2 = static_cast<int>(it2 - begin);
					const cost_type rowMin2 = *it2;
					assert( rowMin >= 0 );
					assert( rowMin == rowMin2  );
					assert( minCol == minCol2  );
					assert( it2 == std::min_element(begin,end) );
				}

				// add vertices, accumulate weight and store in local_min.
				const cost_type addedCosts = addVertices( weightMatrix, w, next, n, row, minCol, localTour, visited );
				if( addedCosts == COST_TYPE_MAX )
					break;
				assert(addedCosts >= 0);

				// accumulate weight and store in local_min.
				if( addedCosts >= 0.5f )
					local_min += addedCosts;
				else
					local_min += rowMin;

				assert(local_min >= 0);
				
				// optimization: terminate early if local_min already exceeds nearest_neighbor_min.
				if(local_min >= nearest_neighbor_min)
					break;

				if( !visited[minCol] )
				{
					// save column to potential solution's tour
					localTour.push_back( minCol );

					// finally, mark this column as visited.
					markVisited(w,n,minCol,visited);
				}

				// @@@ DEBUG
				/*
				std::cout << "local_min[" << row << "," << minCol << "]: " << local_min << " (+" << addedCosts << ")" << std::endl;
				if( roundFloat(local_min) != roundFloat(getPartialTourLength(weightMatrix, localTour, n, false)) )
				{
					cost_type xxx = roundFloat(getPartialTourLength(weightMatrix, localTour, n, false));
					printSolution(localTour);
					getPartialTourLength(weightMatrix, localTour, n, false);
				}
				*/
				assert(roundFloat(local_min) == roundFloat(getPartialTourLength(weightMatrix, localTour, n, false)));
			}
			else 
			{
				// close path using the original, unmodified weightMatrix
				local_min += weightMatrix[row*n+localTour[0]]; 

				// make sure the tour's size matches 'n'
				assert( localTour.size() == n );

				if( options.validate )
				{
					const cost_type tmp = getTourLength(weightMatrix, localTour.begin(), n);
					assert( fp_equal(local_min, tmp, 0.05f) );
				}

				break;
			}

			// move on to next row, which is the unvisited column with
			// the smallest weight.
			row = minCol;

			// make sure we don't loop forever
			assert( ++counter < n );

			isTimeUp = !outTour.empty() && (isTimeUp || options.isTimeUp());
		}   
		timer.Stop();

		// consolidate local_min with local_min
		if( nearest_neighbor_min > local_min && !isTimeUp )
		{
			// we found a new, better tour!
			if( options.verbose )
				std::cout << "nearest-neighbor-cpu: " << local_min 
						  << " (" << timer.Elapsed() << "ms @ " << i << ")"<< std::endl;

			if( options.validate )
			{
				const cost_type tmp = getTourLength(weightMatrix, localTour.begin(), n);
				assert( fp_equal(local_min, tmp, 0.05f) );
			}
			
			nearest_neighbor_min = local_min;


			if( twoOptfn )
			{
				// two_opt_tuc_cpu is the fastest and even beats two_opt_cpu for n > 2000.
				// Here are some results for rl5915 (556045):
				// two_opt_tuc_cpu: 615460 in 4990.05 seconds.
				// two_opt_tuc_gpu: 615460 in 6928.18 seconds.
				// two_opt_cpu:		592401 in 6300.73 seconds.
				//
				// This raises the question:
				// What is better, 615460 in 4990 secs, or 592401 in 6300 secs?
				// 615460 - 592401 = 23059
				// 4990 - 6300 = -1310.
				// 23059 / 1310 = 17.60.
				// So on average for rl5915 it takes about 17.60 seconds to 
				// improve the result by 1 point.
				// Is that a price willing to take?
				// If so, try:
				//				amp-floyd-2opt.exe -use-parallel -use-two-opt-cpu

				twoOptfn(localTour.begin(),n,tsp.pbl_adj_mtx, options);
				local_min = getTourLength(weightMatrix, localTour.begin(), n);
			}

			if( all_min > local_min )
			{
				// save to global.
				all_min = local_min;
				// outTour.swap(localTour);
				outTour = localTour;

				if( options.validate )
				{
					const cost_type tmp = getTourLength(weightMatrix, outTour.begin(), n);
					assert( fp_equal(local_min, tmp, 0.05f) );
				}
			}
		}

		// store local tour
		if( twoOptfn && options.use_ga && localTour.size() == n )
			solutions.insert( SolutionsPair(local_min, localTour) );

		isTimeUp = !outTour.empty() && (isTimeUp || options.isTimeUp());
    }

	if( options.validate )
	{
		const cost_type tmp = getTourLength(weightMatrix, outTour.begin(), n);
		assert( fp_equal(all_min, tmp, 0.05f) );
	}

	// use_ga's default is 'false'
	// Unfortunately my GA does not significantly improve pre-optimized tours.
	if( twoOptfn && options.use_ga )
		tsp_ga( outTour, tsp, options, solutions, twoOptfn );

	// that's our solution
	return all_min;
}

//----------------------------------------------------------------------------
// Finds a tour using the nearest-neighbor algorithm.
//
// (TspSolverFunction wrapper for TSPSolver's 
//----------------------------------------------------------------------------

opt::cost_type nnb::tsp_nearest_neighbor( std::vector<int>& tour, 
							const pbl::Tsp_container& tspContainer, 
							const opt::TspOptions& options  )
{
	Timer timer;
	const int n = tspContainer.problem_dim;

    std::vector<cost_type> w =  tspContainer.pbl_adj_mtx;
	infinitize(w,n);

	std::vector<int> next(n*n);
	std::fill( next.begin(), next.end(), NO_PATH_INT);

	// find the tour using nearest neighbor

	// bparadie, 2012-08-22:
	// It turned out, it is very hard to beat the serial std::min_element.
	// In findMinimum_gpu I tried reduction_cascade() and reduction_simple_1()
	// from the Parallel Reduction using C++ AMP article.
	// I couldn't get reduction_cascade() to work and reduction_simple_1() was
	// about 10x slower than std::min_element().

	// DISABLED: findMinimum_gpu is just too slow.
	// const cost_type len = findNearestNeighbor( w, next, n, tour, options, findMinimum_gpu );
	timer.Start();
	const cost_type len = findNearestNeighbor( w, next, n, tour, tspContainer, options, findMinimum_cpu );
	timer.Stop();

	if( options.verbose )
		std::cout << "nearest-neighbor: " << timer.Elapsed() << "ms." << std::endl;

	return len;
}
