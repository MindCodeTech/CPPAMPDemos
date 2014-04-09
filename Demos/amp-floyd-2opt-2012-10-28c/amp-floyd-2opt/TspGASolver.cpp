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

#include "TspGASolver.hpp"
#include "Tsp2OptSolver.hpp"
#include "TspUtils.hpp"
#include "TspTests.hpp"
#include "problem.hpp"

#include <iostream>
#include <assert.h>

//----------------------------------------------------------------------------
// Namespaces
//----------------------------------------------------------------------------

using namespace utl;
using namespace pbl;
using namespace opt;
using namespace gas;
using namespace two;
using namespace tst;

//----------------------------------------------------------------------------
// OX, cross-over function.
// 
// Based on: A highly-parallel TSP solver for a GPU computing platform
// http://dl.acm.org/citation.cfm?id=1945726
// http://www.springerlink.com/content/n20570265p081734/fulltext.pdf
// 
// s1[0 .. n-1] and s2[0 .. n-1]: given two individual s
// d[0 .. n-1]: buffer for a generated individual
// Set cut1 and cut2 randomly so that
// 0 <= cut1, cut2 < n and cut1 != cut2;
//----------------------------------------------------------------------------

static void OX( int n, 
				std::vector<int>::const_iterator s1,
				std::vector<int>::const_iterator s2,
				std::vector<int>::iterator d)
{
	
	int cut1 = rand();
    int cut2 = cut1;
    while( cut2 == cut1 )
        cut2 = rand() % n;
    
    int pos;
	int j;
    
    for( j = 0; j < n ; j++) 
        d[ j ] = s1 [ j ];
    
    for( j = cut1; j < cut2; j++)
    {
        for( pos = 0; pos < n ; pos++) 
        {
            if( s2 [ j ] == d[ pos ] ) break;
        }
        int next = pos + 1;
        while ( next != cut2 )
        {
            if( pos == n) pos = 0;
            if ( next == n) next = 0;
            d[ pos++] = d[ next++];
        }
    }
    for( j = cut1; j < cut2 ; j++) 
        d[ j ] = s2 [ j ] ;
}

//----------------------------------------------------------------------------
// Based on: A highly-parallel TSP solver for a GPU computing platform
// http://dl.acm.org/citation.cfm?id=1945726
// http://www.springerlink.com/content/n20570265p081734/fulltext.pdf
//
// The funny thing is that this article focusses on parallelizing OX()
// while 90% of the running time is spent in two_opt, which the author
// did not optimize.
//
// A year later the same authors acknowledge the importance of optimizing two_opt in 
// a different article and offered a parallel version of two_opt that pre-calculates move 
// costs and then picks the ones with the lowest move costs.
// (See Fast QAP Solving by ACO with 2-opt Local Search on a GPU,
// http://ieeexplore.ieee.org/xpl/articleDetails.jsp?arnumber=5949702)
//
//----------------------------------------------------------------------------

static cost_type ga_solver( const std::vector<cost_type>& w, int n, 
							 std::vector<int>& tour, const TspOptions& inOptions,
							 int populationSize, int maxGeneration,
							 std::vector<int>& s1,
							 std::vector<cost_type>& p1,
							 TwoOptFunction fn,
							 cost_type oldLength)
{
	cost_type bestSolution = oldLength;

	// s1[0 .. n-1] and p1[0 .. n-1]:
	// p1[i] is the length of tour s1[i]
	// s2[0 .. n-1]: buffer for individual s generated by
	// OX and 2-opt
	// p2[0 .. n-1]: p2[i] is the length of tour s2[i]
        
	if( p1.empty() )
	{
		s1 = std::vector<int>(populationSize*n);
		p1 = std::vector<cost_type>(populationSize);
		for( int i = 0; i < populationSize; i++ )
		{
			// create random tour by shuffling index positions
			std::vector<int>::iterator s1begin = s1.begin() + i*n;
			std::vector<int>::iterator s1end = s1begin + n;
			init_array_with_shuffled_indices(s1begin, s1end, n);

			// calculate accumulated distances and store in p1
			const cost_type length = getTourLength( w, s1begin, n );
			p1[i] = length;
                        
			// We don't know what's acceptable:
			// if( p1[i] is acceptable ) return i; // found solution is s1[i]

			if( bestSolution > length )
			{
				bestSolution = length;
				std::cout << "tsp-ga [0]: " << length << " in 0ms" << std::endl;
			}
		}
	}
	
	// msg( options, "Mutate populations... " );

	// two dimensional array of city indices (columns) and populations (rows)
    std::vector<int> s2(populationSize * n);

	// one dimensional array of tour lengths
    std::vector<cost_type> p2(populationSize);

	// use custom options with verbose = false
	TspOptions options = inOptions;
	options.verbose = false;

	bool isTimeUp = false;

	for( int generation = 0; generation < maxGeneration && !isTimeUp; generation++ ) 
    {
		Timer timer;
		for( int i = 0; i < populationSize && !isTimeUp; i++ )
		{
			// j = a random integer such that
			// 0 <= j < populationSize and j != i ;            
            int j;
            do
            {
                j = rand() % populationSize;
            }
            while( j == i );

            // s2[i] = OX( s1[i], s1[j] );
            OX( n,
				s1.begin() + i*n,
				s1.begin() + j*n,
                s2.begin() + i*n );

			timer.Start();
			fn( s2.begin() + i*n, n, w, options ); 
			timer.Stop();

			if( inOptions.verbose )
			{
				const cost_type len = getTourLength(w, s2.begin(), n);
				if( bestSolution > len )
				{
					bestSolution = len;
					std::cout << "tsp-ga [" << generation << "]: " 
							  << len << " in " << timer.Elapsed() << "ms" << std::endl;
					std::cout.flush();
				}
			}

			isTimeUp = options.isTimeUp();
        }
	    
        for( int i = 0; i < populationSize && !isTimeUp; i++ ) 
        {
            /*
            p2 [ i ] = evaluate( s2[ i ] ) ;
            if (p2[ i ] < p1 [ i ] ) {
                s1 [ i ] = s2 [ i ] ;
                p1 [ i ] = p2 [ i ] ;
            }
            if (p1 [ i ] is acceptable ) return i ; // found solution is s1[i]
            */
            
			std::vector<int>::const_iterator s2begin = s2.begin() + i*n;
			std::vector<int>::const_iterator s2end = s2begin + n;
            const cost_type length = getTourLength( w, s2begin, n ) ;
            
            p2[ i ] = length;
            if( p2[i] < p1 [i] ) 
            {
                std::copy( s2begin, s2end, s1.begin() + (i*n) );
				p1[ i ] = p2[ i ] ;
            }
            // if (p1 [ i ] is acceptable ) return i ; // found solution is s1[i]
			isTimeUp = options.isTimeUp();
        }
    }

	// find minimum...
	// std::minimum_element is still the fastest.
	// See findMinimum.cpp for parallel versions.
	if( bestSolution < oldLength )
	{
		// copy tour - but only if it's better than the one that got passed in.
		std::vector<cost_type>::const_iterator minFound = std::min_element(p1.begin(), p1.end());
		cost_type min = *minFound;
		if( min < oldLength )
		{
			const int minIdx = static_cast<int>(minFound - p1.begin());

			// locate best tour
			std::vector<int>::const_iterator s1begin = s1.begin() + (minIdx*n);
			std::vector<int>::const_iterator s1end = s1begin + n;
			assert( min == getTourLength(w, s1begin, n) );

			tour.clear();
			tour.reserve(n);
			std::copy(s1begin, s1end, std::back_inserter(tour) );
		}
	}

	return bestSolution; // solution is not found
}

//----------------------------------------------------------------------------
// Uses a genetic algorithm (GA) with 2-opt optimization and 
// returns the best known solution.
// The tour to the corresponding solution is stored in 'tour'.
//----------------------------------------------------------------------------

cost_type gas::tsp_ga( std::vector<int>& tour, 
					 const Tsp_container& tspContainer, 
					 const TspOptions& options,
					 Solutions& solutions,
					 TwoOptFunction fn)
{
	const std::vector<cost_type>& weightMatrix = tspContainer.pbl_adj_mtx;
	const int n = tspContainer.problem_dim;

	// only symmetrical TSPs can be calculated this way.
	assert( isSymmetrical(weightMatrix,n) );


	// const int solutions_size = static_cast<int>(solutions.size());
	// int populationSize = (solutions_size < 10) ? solutions_size : 10;
    // int maxGeneration = 100; 

	// The paper suggests using populationSize = 60 with maxGeneration = 1000 and
	// starts with a list of 60 randomized tours. This implementation modifies 
	// that approach and accepts pre-optimized tours via 'solutions'.
	// The TwoOptFunction 'fn' is usually two_opt_tuc_cpu.
	const int populationSize = 60;	// better: options.ga_populationSize;
	const int maxGeneration = 1000;	// better: options.ga_maxGeneration;

	typedef std::pair< cost_type,std::vector<int> > SolutionsPair;
	// two dimensional array of city indices (columns) and populations (rows)
    std::vector<int> s1(populationSize*n);
	
	// one dimensional array of tour lengths
    std::vector<cost_type> p1;
	p1.reserve(populationSize);

	if( !solutions.empty() )
	{
		std::vector<int>::iterator s1iter = s1.begin();
		std::map<cost_type, std::vector<int>>::iterator iter;
		for (iter = solutions.begin(); iter != solutions.end() && s1iter != s1.end(); ++iter) 
		{
			p1.push_back( iter->first );
			std::copy( iter->second.begin(), iter->second.end(), s1iter );
			s1iter += n;
 		}
	}

	const cost_type oldLength = tour.empty() ? COST_TYPE_MAX : getTourLength(weightMatrix,tour.begin(),n);
	Timer timer;
	timer.Start();
	const cost_type len = ga_solver(weightMatrix,n, tour, options, 
									populationSize, maxGeneration,s1, p1, fn, 
									oldLength );
	timer.Stop();

	if( options.verbose && len < oldLength )
	{
		std::cout << "tsp-ga: " << len << " (" << timer.Elapsed() << "ms)" << std::endl;
		std::cout.flush();
	}

    return len;
}

//----------------------------------------------------------------------------
// Returns the total length of a tour using distance matrix 'w'.
// TspSolverFunction version.
//----------------------------------------------------------------------------

opt::cost_type gas::tsp_ga( std::vector<int>& tour, 
					const pbl::Tsp_container& tsp, 
					const opt::TspOptions& options)
{
	cost_type len = COST_TYPE_MAX;

	// TwoOptFunction twoOptfn = getTwoOptFunction(options, tsp.pbl_adj_mtx, tsp.problem_dim);

	// bparadie, 2012-10-27: let's use the most precize 2-opt solver we have (two_opt_cpu)
	// for data sets with n < 1000, otherwise use the fastest 2-opt solver (two_opt_tuc_cpu).
	TwoOptFunction twoOptfn;
	if(tsp.problem_dim < 1000)
		twoOptfn = two_opt_cpu;
	else
		twoOptfn = two_opt_tuc_cpu;

	if( twoOptfn )
	{
		Solutions solutions;
		len = tsp_ga( tour, tsp, options, solutions, twoOptfn );
	}
	return len;
}
