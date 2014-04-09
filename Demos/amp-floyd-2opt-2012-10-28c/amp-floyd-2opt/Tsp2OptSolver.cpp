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

#include "Tsp2OptSolver.hpp"
#include "TspUtils.hpp"
#include "TspTests.hpp"

#include <iostream>
#include <algorithm>
#include <amp.h>               
#include <amp_math.h>
#include <amp_graphics.h>

#include <assert.h>

//----------------------------------------------------------------------------
// Namespaces
//----------------------------------------------------------------------------

using namespace concurrency;
using std::vector;

using namespace opt;
using namespace utl;
using namespace pbl;
using namespace two;
using namespace tst;

// FLT_MAX
#define MAX_LENGTH (3.402823466e+38F)

// Reduce macros
#define REDUCE_OP(lhs,rhs) { (lhs) = op((lhs),(rhs)); }
#define REDUCE_RESULT(mem,op,lhs,rhs) { REDUCE_OP((mem[(lhs)]),(mem[(lhs)+(rhs)])); tidx.barrier.wait_with_tile_static_memory_fence(); }
#define REDUCE_RESULT_WITH_STEP(mem,op,lhs,rhs) { if( direct3d::step( mem[lhs].minchange,mem[lhs+rhs].minchange) == 0 ) { best_values[lhs] = best_values[lhs+rhs]; } tidx.barrier.wait_with_tile_static_memory_fence(); }

//----------------------------------------------------------------------------
// Struct for recording the best tour for kernel2opt and two_opt_gpu.
//----------------------------------------------------------------------------

struct best2_out {

	int i;
	int j;
	cost_type minchange;
	
};


//----------------------------------------------------------------------------
// Returns appropriate 2-opt functionfor selected -use-two-opt-* 
//----------------------------------------------------------------------------

TwoOptFunction two::getTwoOptFunction(const opt::TspOptions& options, 
									  const std::vector<opt::cost_type>& w, 
									  int n)
{
	TwoOptFunction twoOptfn = nullptr;

	if( isSymmetrical(w,n) )
	{
		if( options.use_two_opt_cpu )
			twoOptfn = two_opt_cpu;
		else if( options.use_two_opt_gpu )
		{
			if( n > TWO_OPT_GPU_MAX_CITIES )
				twoOptfn = two_opt_cpu;
			else
				twoOptfn = two_opt_gpu;
		}
		else if( options.use_two_opt_tuc_cpu )
			twoOptfn = two_opt_tuc_cpu;
		else if( options.use_two_opt_tuc_gpu )
			twoOptfn = two_opt_tuc_gpu;
	}

	return twoOptfn;
}


//----------------------------------------------------------------------------
// two_opt() is based on this Pascal code:
// 
// Operations Research Software Exchange Program
// http://optimierung.mathematik.uni-kl.de/old/ORSEP/contents.html
//
// PASCAL Codes of Discrete Optimizatio Algorithms
// EJOR 40/1 (1989), 120-127
// ftp://www.mathematik.uni-kl.de/pub/Math/ORSEP/SYSLO.ZIP
// 
//----------------------------------------------------------------------------

/*
    2-Optimal Approximate Algorithm for
    the Traveling Salesman Problem

    Procedure  TWOOPT

    1. Application

    Procedure TWOOPT  finds a 2-optimal solution to the symmetric 
    traveling salesman problem for a network on N nodes, given as an 
    NxN weight matrix W of integers. It starts with the initial tour 
    (specified by array ROUTE) and improves the tour repeatedly by 
    exchanging two edges at a time. The edge exchange continues until 
    no better solution can be found by exchanging a pair of edges of 
    the current solution. The final solution, which is a local 
    optimum, is given by the array ROUTE.
    A 2-optimal algorithms was proposed in [1] and procedure 
    TWOOPT appeared first in [2].

    2. Procedure Parameters

    PROCEDURE TWOOPT(
                     N            :INTEGER;
                     VAR W        :ARRNN;
                     VAR ROUTE    :ARRN;
                     VAR TWEIGHT  :INTEGER);
    3. Data Types

    TYPE ARRN  = ARRAY[1..N] OF INTEGER;
    ARRNN = ARRAY[1..N,1..N] OF INTEGER;

    4. Global Constans

    N  - number of nodes in the given network.

    5. Input

    N  - number of nodes in the given network,
    W[1..N,1..N]  - weight matrix of the given network,
    ROUTE[1..N]  - array specifing the initial traveling 
    salesman route,

    TWEIGHT  - total weight of the initial route.

    6. Output

    ROUTE[1..N]  - array giving the final route,
    TWEIGHT  - total weight of the route.

    7. References

    [1] S.Lin, B.W.Kernighan, An effective heuristic algorithm for 
    the traveling-salesman problem, Oper.Res. 21(1973), 
    498-516.
    [2] M.M.Syslo, N.Deo, J.S.Kowalik, Discrete Optimization 
    Algorithms with  Pascal Programs, Prentice-Hall, Englewood 
    Cliffs 1983.

    Author of Implementation : M.M.Syslo
    Authors of Documentation : Janina Bossy and M.M.Syslo

*/

void two::two_opt_cpu( std::vector<int>::iterator tour, 
						int n,
						const std::vector<opt::cost_type>& w,
						const opt::TspOptions& options )
{
	Timer timer;
	timer.Start();

	const int N = n;
    cost_type tweight = 0;
        
    int ahead,i,i1,i2,index,j,j1,j2,last,
		limit,next,s1,s2,t1,t2;
	cost_type max, max1;
    std::vector<int> ptr(N);

    const int n1 = N - 1;
	const int n2 = N - 2;
    
    // validateTour( tour, n );
    
	std::vector<int>::iterator route = tour;
    
    for( i=0; i < n1; i++ )
    {
        // std::cout << "ptr_init[" << route[i] << "]] = " << route[i+1] << std::endl;
        ptr[route[i]] = route[i+1];
    }
    
    ptr[route[n1]] = route[0];
  
    // dumpVector( "ptr", ptr.begin(), ptr.end() );
	const cost_type startLen = getTourLength(w, tour, N);
	cost_type len = startLen;

	do 
    {
        max = 0;
        i1 = 0;
        for( i = 0; i < n2; i++ )
        {
            if( i == 0 )
                limit = n1;
            else 
                limit = N;
            
            i2 = ptr[i1];
            j1 = ptr[i2];
            
            for( j = i+2; j < limit; j++ )
            {
                j2 = ptr[j1];
                max1 = w[i2*n+i1]+w[j2*n+j1] -
                       (w[j1*n+i1]+w[j2*n+i2]);
                
                
                if( max1 > max )
                {           
                    // better pair of edges has been found
                    s1 = i1;
                    s2 = i2;
                    t1 = j1;
                    t2 = j2;
                    max = max1;
                }
                j1 = j2;
            }
            i1 = i2;
        }
        
        if( max > 0 )
        {   
			/*
			std::cout << "Pairs: [" 
				<< s1 << "," <<  s2 << "," 
				<< t1 << "," << t2 << "] = " << max
				<< std::endl;
			*/

            // swap pair of edges
            ptr[s1] = t1;
            next = s2;
            last = t2;
            
            // reverse appropriate links
            do                    
            {
                ahead = ptr[next];
                ptr[next] = last;
                last = next;
                next = ahead;
            }
            while( next != t2 );
            
            // route is now shorter
            tweight -= max;           
        }

		len -= max;
    }
    while( max != 0 && !options.isTimeUp() );
    
    index = 1;
    for( i = 0; i < N; i++ )
    {
        route[i] = index;
        index = ptr[index];
    }     

	timer.Stop();
	if( options.verbose && startLen > len )
		std::cout << "2-opt-cpu: " << len << " (" << timer.Elapsed() << "ms)" << std::endl;

} /* twoopt  */


//----------------------------------------------------------------------------
// Rotates the order of the elements in the range [first,last), 
// in such a way that the element pointed by middle becomes the new first element.
// Source: http://www.cplusplus.com/reference/algorithm/rotate/
//----------------------------------------------------------------------------

static void amp_rotate( array< int, 1>& tour, int first, int middle, int last) restrict(amp)
{
	int f = first;
	int m = middle;
	int next = middle;
	while(f != next) 
	{
		// swap first with next
		int* next_ptr = &(tour[next]);
		int* first_ptr = &(tour[f]);
		atomic_exchange( next_ptr, atomic_exchange(first_ptr, *next_ptr) );
		f++;
		next++;
		if (next == last) 
		{
			next = m;
		} 
		else if (f == m) 
		{
			m = next;
		}
	}
}

//----------------------------------------------------------------------------
// Reverses the order of the elements in the range [first,last).
// Source: http://www.cplusplus.com/reference/algorithm/reverse/
//----------------------------------------------------------------------------

static void amp_reverse( array< int, 1>& tour, int first,  int last) restrict(amp)
{
	while ((first!=last)&&(first!=--last))
	{
		int* next_ptr = &(tour[first++]);
		int* first_ptr = &(tour[last]);
		atomic_exchange( next_ptr, atomic_exchange(first_ptr, *next_ptr) );
	}
}

//----------------------------------------------------------------------------
// Based on:
// Accelerating 2-opt and 3-opt Local Search Using GPU in the Travelling Salesman Problem
// http://www.researchgate.net/publication/229476110_Accelerating_2-opt_and_3-opt_Local_Search_Using_GPU_in_the_Travelling_Salesman_Problem
//
// This function is the parallel version.
//----------------------------------------------------------------------------

static void kernel2opt( tiled_index<TILE_SIZE> tidx,
						const array<cost_type, 1>& w, 
						int n, 
						array< int, 1>& t, 
						array<cost_type, 1>& result ) restrict(amp)
{
	int threadIdx_x = tidx.local[0];

	// 2-opt move index
	tile_static int tour[TWO_OPT_GPU_MAX_CITIES];
	tile_static best2_out best_values[TILE_SIZE];

	int i,j;

	for( i = threadIdx_x; i < n; i += TILE_SIZE)
	{
		tour[i] = t[i];
	}
	tidx.barrier.wait_with_tile_static_memory_fence();

	best2_out best;
	best.i = 0;
	best.j = 0;
	best.minchange = MAX_LENGTH;

	int start = threadIdx_x == 0 ? TILE_SIZE : threadIdx_x;
	for( i = start; i < (n-1); i += TILE_SIZE) 
	{
		for(j = 0; j < (i-1); j ++) 
		{
			const int c2 = tour[i-1];
			const int c3 = tour[i];
			const int c7 = tour[j];
			const int c8 = tour[j+1];

			// Delta(length) = dist(C2,C7) + dist(C3,C8) - dist(C2,C3) - dist(C7,C8)

			// direct3d::mad x*y+z
			const cost_type c2c7 = w[direct3d::mad(c2,n,c7)];
			const cost_type c3c8 = w[direct3d::mad(c3,n,c8)];
			const cost_type c2c3 = w[direct3d::mad(c2,n,c3)];
			const cost_type c7c8 = w[direct3d::mad(c7,n,c8)];

			// Delta(length) = dist(C2,C7) + dist(C3,C8) - dist(C2,C3) - dist(C7,C8)
			const cost_type delta = c2c7 + c3c8 - c2c3 - c7c8;

			if( delta < -0.5f && delta < best.minchange )
			{
				best.i = i;
				best.j = j;
				best.minchange = delta;
			}
		}
	}		 

	best_values[threadIdx_x] = best;
	tidx.barrier.wait_with_tile_static_memory_fence();

	// unrolled for performance
	REDUCE_RESULT_WITH_STEP(best_values,op,threadIdx_x, 512);
	REDUCE_RESULT_WITH_STEP(best_values,op,threadIdx_x, 128);
	REDUCE_RESULT_WITH_STEP(best_values,op,threadIdx_x,  64);
	REDUCE_RESULT_WITH_STEP(best_values,op,threadIdx_x,  32);
	REDUCE_RESULT_WITH_STEP(best_values,op,threadIdx_x,  16);
	REDUCE_RESULT_WITH_STEP(best_values,op,threadIdx_x,   8);
	REDUCE_RESULT_WITH_STEP(best_values,op,threadIdx_x,   4);
	REDUCE_RESULT_WITH_STEP(best_values,op,threadIdx_x,   2);
	REDUCE_RESULT_WITH_STEP(best_values,op,threadIdx_x,   1);

	// run only once
	if( tidx.global[0] == 0 )
	{
		best = best_values[0];

		// trim result
		if( best.i < 0 || best.j < 0 )
		{
			// no results found
			best.minchange = 0;
		}
		else if( best.i > best.j )
		{
			// make sure i < j
			best.i = best_values[0].j;
			best.j = best_values[0].i;
		}

		if( best.minchange < 0 )
		{
			// wrong: std::reverse(route + min.i, route + min.j + 1);	
			// correct: "inside-out" reversal, rotate to i + 1, then reverse j to n.
			amp_rotate( t, 0, best.i + 1, n);
			amp_reverse(t, best.j - best.i - 1, n);	
		}

		result[0] = best.minchange;
	}

}

//----------------------------------------------------------------------------
// Based on:
// Accelerating 2-opt and 3-opt Local Search Using GPU in the Travelling Salesman Problem
// http://www.researchgate.net/publication/229476110_Accelerating_2-opt_and_3-opt_Local_Search_Using_GPU_in_the_Travelling_Salesman_Problem
//
// This function is the parallel version.
//----------------------------------------------------------------------------

void two::two_opt_gpu( std::vector<int>::iterator route, 
						int n,
						const std::vector<opt::cost_type>& weightMatrix,
						const opt::TspOptions& options )
{
	Timer timer;
	timer.Start();

	assert( n <= TWO_OPT_GPU_MAX_CITIES);

	const cost_type startLen = getTourLength(weightMatrix, route, n );
	cost_type len = startLen;

	//reset ids of edges selected to swap
	extent<1> resultExtent(1);

	vector<cost_type>min_raw(1);
	min_raw[0] = 0;
	array<cost_type, 1> min(resultExtent, min_raw.begin());

	// one dimensional array of n * n weights.
	extent<1> weightMatrixExtent(n*n);
	array<cost_type, 1> w(weightMatrixExtent, weightMatrix.begin());

	// copy the route from CPU to GPU
	extent<1> citiesExtent(n);
	array< int, 1> tour(citiesExtent,route);
	std::vector<int> paddedTour;

	// manual padding is faster than using tile<TILE_SIZE>().pad()
	// plus extent.contains(idx) in the kernel code.
	if( (n % TILE_SIZE) != 0 )
	{
		// round up to the next tile
		const int paddedSize = TILE_SIZE * ((n / TILE_SIZE)+1);
		
		std::vector<int> newTour(paddedSize);
		std::copy( route, route+n, newTour.begin() );
		std::fill( newTour.begin()+n, newTour.end(), NO_PATH_INT );

		paddedTour.swap(newTour);

		extent<1> paddedCitiesExtent(paddedSize);
		tour = array< int, 1>(paddedCitiesExtent,paddedTour.begin());
	}
	
	do 
	{		
		
		parallel_for_each(tour.extent.tile<TILE_SIZE>(), [=,&tour,&w, &min](tiled_index<TILE_SIZE> tidx) restrict(amp)
		{
			kernel2opt(tidx, w, n, tour, min);
		});

		// copy result from GPU to CPU
		copy( min, min_raw.begin() );

		len += min_raw[0];
	} 
	while (min_raw[0] < -0.5 && !options.isTimeUp() );

	// copy tour from GPU to CPU
	if( paddedTour.empty() )
		copy( tour, route );
	else
	{
		copy( tour, paddedTour.begin() );
		std::copy( paddedTour.begin(), paddedTour.begin()+n, route );
	}

	timer.Stop();
	if( startLen > len )
		std::cout << "2opt-gpu: " << len << " in " << timer.Elapsed() << "ms." << std::endl;
}


//----------------------------------------------------------------------------
// Based on:
// Hardware Implementation of 2-Opt Local Search Algorithm for the Traveling Salesman Problem
// http://dl.acm.org/citation.cfm?id=1263915
// http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=4228483&tag=1
//
// 'pe' stands for 'processing engine'
//----------------------------------------------------------------------------

void two_opt_tuc_gpu_pe(index<1> idx,
			const array< int, 1>& tour, 
             const array_view<const cost_type, 2>& w,
			 array< int, 1>& results,
			 int offset ) restrict(amp)
{
	int c3idx = idx[0];

	// We process the segment's cities and caclulate 
	// whether its 2-Opt move is length-reducing or not by 
	// evaluating the corresponding cities, i.e.
	//		0,1 with 48,49 
	//		1,2 with 47,48 
	// if 'segment' is 50 and offset is 0.
	// If offset is 1 we'll end up with
	//		0,1 with 49,50 
	//		1,2 with 48,49
	// Using an offset is faster than rotating the underlying tour.

	if( c3idx > 0 )
	{
		const int segments = results.extent.size();
	
		// Each segment keeps the left hand side of pairs.
		// The end of a segment is found by doubling the result's size.
		const int endSegments = segments * 2;
		const int c7idx = endSegments-c3idx+offset-1;

		// The variable names c2, c3, c7, c8 are from an
		// example of the paper this method is based on. 
		// Using the same names as used in the example
		// helped me getting less confused of what this
		// algorithm is trying to do.

		const int c2 = tour[c3idx-1]; 
		const int c3 = tour[c3idx]; 

		const int c7 = tour[c7idx]; 
		const int c8 = tour[c7idx+1]; 

		// Calculate 'delta' for the current segment boundary:
		// Delta(length) = dist(C2,C7) + dist(C3,C8) - dist(C2,C3) - dist(C7,C8)
		cost_type delta = w(c2,c7)+w(c3,c8) -
						  w(c2,c3)-w(c7,c8);

		// Negative deltas reduce the tour length
		// and are recorded as TRUE.
		if(delta < 0) 
			results[c3idx] = TRUE;
		else 
			results[c3idx] = FALSE;
	}
}

//----------------------------------------------------------------------------
// Based on:
// Hardware Implementation of 2-Opt Local Search Algorithm for the Traveling Salesman Problem
// http://dl.acm.org/citation.cfm?id=1263915
// http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=4228483&tag=1
//
// This unit swaps tour entries.
//----------------------------------------------------------------------------

void two_opt_tuc_gpu_swap(index<1> idx,
			array< int, 1>& tour, 
             const array_view<const cost_type, 2>& w,
			 const array< int, 1>& results,
			 int offset ) restrict(amp)
{
	int c3idx = idx[0];
	const int segments = results.extent.size();
	const int endSegments = segments * 2;
	const int c7idx = endSegments-c3idx+offset-1;

	// If we made it to here we should have yes/no decisions
	// in 'results' and 'localResults' for the segment 
	// reversals under consideration.
	int yes = 0;

	for( int j = 1; j <= c3idx; j++ )
	{
		if( results[j] )
		{
			yes++;
		}
	}				
				
	// This is a really clever idea of the paper:
	// Only odd numbers of reversals need to be considered, 
	// because double reversals cancel each other out.
	// This allows us to run multiple 2-opt moves per round.

	if( (yes % 2) != 0 )
	{
		// swap!
		/*
		int tmp = tour[c3idx];
		tour[c3idx] = tour[c7idx];
		tour[c7idx] = tmp;
		*/

		int* lhs = &(tour[c3idx]);
		int* rhs = &(tour[c7idx]);
		atomic_exchange(rhs, atomic_exchange(lhs, *rhs) );
	}
}

//----------------------------------------------------------------------------
// Based on:
// Hardware Implementation of 2-Opt Local Search Algorithm for the Traveling Salesman Problem
// http://dl.acm.org/citation.cfm?id=1263915
// http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=4228483&tag=1
//
// This function is the parallel version.
//----------------------------------------------------------------------------

void two::two_opt_tuc_gpu( std::vector<int>::iterator tourRaw, 
							int n,
							const std::vector<opt::cost_type>& weightMatrix,
							const opt::TspOptions& options )
{
	Timer timer;
	timer.Start();

	// make sure that segments are correct
	// even for odd numbers.
	int segments = (n/4 % 2) ? ((n+1)/4) : (n/4);
	if( ((n/2)%2) != 0 ) 
		segments++;

	const int endSegments = segments*2;

	// 'best' keeps the best tour
	extent<1> cities(n);
	array< int, 1> best(cities,tourRaw);

	// 'tour' is evaluated every round
	std::vector<int> t(n);
	std::copy( tourRaw, tourRaw+n, t.begin() );
	const cost_type startLen = getTourLength( weightMatrix, t.begin(), n );
	cost_type min = startLen;
	array< int, 1> tour(cities,t.begin());

	// 'results' is really a bool array of TRUE/FALSE values.
	extent<1> resultsExtent(segments);
	std::vector<int> resultsRaw(segments);
	array< int, 1> results(resultsExtent,resultsRaw.begin());

	// two dimensional array of n * n cities
	extent<2> citiesByCities(n, n);
	array_view<const cost_type, 2> w(citiesByCities,weightMatrix);

	// separating 'counter' from 'next'
	bool next = true;
	int counter = 0;
	int total_counter = 0;

	// iterate until there are no improvements after n-1 rounds.
	while( next && !options.isTimeUp() )
	{

		// calculate candidates comparing [0..segments-1] against [segments..endsegments]
		parallel_for_each(results.extent, [=,&tour, &results](index<1> idx) restrict(amp)
		{
			two_opt_tuc_gpu_pe(idx, tour, w, results, 0);
		});

		// swap appropriate pairs from [0..segments-1] with [segments..endsegments]
		parallel_for_each(results.extent, [=,&tour, &results](index<1> idx) restrict(amp)
		{
			two_opt_tuc_gpu_swap(idx, tour, w, results, 0);
		});

		// calculate candidates comparing [0..segments-1] against [segments+1..endsegments+1]
		parallel_for_each(results.extent, [=,&tour, &results](index<1> idx) restrict(amp)
		{
			two_opt_tuc_gpu_pe(idx, tour, w, results, 1);
		});

		// swap appropraite pairs from [0..segments-1] with [segments+1..endsegments+1]
		parallel_for_each(results.extent, [=,&tour, &results](index<1> idx) restrict(amp)
		{
			two_opt_tuc_gpu_swap(idx, tour, w, results, 1);
		});

		// copy from GPU to CPU
		copy( tour, t.begin() );

		// unlike "text-book" 2-opt algorithms we cannot calculate the 
		// tour length on the fly. Instead we have to calculate the tour length
		// each round.
		const cost_type len = getTourLength( weightMatrix, t.begin(), n );

		if( (min-len) >= 1 )
		{
			// save better tour and length.
			std::copy( t.begin(), t.end(), tourRaw );
			min = len;

			// reset counter
			counter = 0;
			next = true;
		}
		else
		{
			// iterate until there are no improvements after n-1 rounds.
			counter++;
			next = counter < (n-1);
		}

		if( next )	
		{
			// global left shift
			std::rotate( t.begin(), t.begin() + 1, t.begin() + n );
			tour = array< int, 1>(cities,t.begin());
		}
	}

	timer.Stop();
	if( options.verbose && startLen > min )
		std::cout << "2-opt-tuc-gpu: " << min << " (" << timer.Elapsed() << "ms)" << std::endl;
}

//----------------------------------------------------------------------------
// Based on:
// Hardware Implementation of 2-Opt Local Search Algorithm for the Traveling Salesman Problem
// http://dl.acm.org/citation.cfm?id=1263915
// http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=4228483&tag=1
//
// 'pe' stands for 'processing engine'
//----------------------------------------------------------------------------

void two_opt_tuc_cpu_pe(std::vector<int>::iterator tour, 
             int n,
             const std::vector<cost_type>& w,
			 int segments,
			 int offset)
{
	const int endSegments = segments*2;
	std::vector<bool> results(segments);
	std::fill( results.begin(), results.end(), false );

	// We iterate over the segment's cities and caclulate 
	// whether its 2-Opt move is length-reducing or not by 
	// evaluating the corresponding cities, i.e.
	//		0,1 with 48,49 
	//		1,2 with 47,48 
	// if 'segment' is 50 and offset is 0.
	// If offset is 1 we'll end up with
	//		0,1 with 49,50 
	//		1,2 with 48,49
	// Using an offset is faster than rotating the underlying tour.

	for( int i = 1; i < segments; i++ )
	{
		const int c2idx = i-1;
		const int c3idx = i;
		const int c8idx = endSegments+offset-i;
		const int c7idx = c8idx-1;

		// The variable names c2, c3, c7, c8 are from an
		// example of the paper this method is based on. 
		// Using the same names as used in the example
		// helped me getting less confused of what this
		// algorithm is trying to do.

		const int c2 = tour[c2idx];	// C2
		const int c3 = tour[c3idx]; // C3

		const int c7 = tour[c7idx]; // C7
		const int c8 = tour[c8idx]; // C8

		// Delta(length) = dist(C2,C7) + dist(C3,C8) - dist(C2,C3) - dist(C7,C8)
		cost_type delta = w[c2*n+c7]+w[c3*n+c8] -
						  w[c2*n+c3]-w[c7*n+c8];

		// Negative deltas reduce the tour length
		// and are recorded as 'true'.
		if( delta < 0 )
			results[i] = true;
	}

	// If we made it to here we should have yes/no decisions
	// in 'results' for the segment reversals under consideration.

	int yes = 0;
	for( int i = 1; i < segments; i++ )
	{
		if( results[i] )
		{
			yes++;
		}

		// This is a really clever idea of the paper:
		// Only odd numbers of reversals need to be considered, 
		// because double reversals cancel each other out.
		// This allows us to run multiple 2-opt moves per round.

		// swap?
		if( (yes % 2) != 0 )
		{
			// std::cout << "Swapping " << i + 1 << " with " << endSegments-i-1 << " = " << 
			// 		*(tour + i + 1) << " <-> " << *(tour + endSegments-i-1) << std::endl;
			std::swap_ranges( tour + i, tour + i + 1, tour + endSegments-i+offset-1 );
		}
	}
}

//----------------------------------------------------------------------------
// Based on:
// Hardware Implementation of 2-Opt Local Search Algorithm for the Traveling Salesman Problem
// http://dl.acm.org/citation.cfm?id=1263915
// http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=4228483&tag=1
//
// This function emulates the parallel version.
//----------------------------------------------------------------------------

void two::two_opt_tuc_cpu( std::vector<int>::iterator tour, 
							int n,
							const std::vector<opt::cost_type>& w,
							const opt::TspOptions& options )
{
	Timer timer;
	timer.Start();

	// make sure that segments are correct
	// even for odd numbers.
	int segments = (n/4 % 2) ? ((n+1)/4) : (n/4);
	if( ((n/2)%2) != 0 ) 
		segments++;

	// separating 'counter' from 'next'
	bool next = true;
	int counter = 0;
	int total_counter = 0;

	std::vector<int> t(n);
	std::copy( tour, tour+n, t.begin() );
	const cost_type startLen = getTourLength( w, t.begin(), n );
	cost_type min = startLen;

	// iterate until there are no improvements after n-1 rounds.
	while( next && !options.isTimeUp() )
	{
		// calculate candidates comparing [0..segments-1] against [segments..endsegments]
		// and swap appropraite pairs from [0..segments-1] with [segments..endsegments]
		two_opt_tuc_cpu_pe(t.begin(), n, w, segments, 0);

		// calculate candidates comparing [0..segments-1] against [segments+1..endsegments+1]
		// and swap appropraite pairs from [0..segments-1] with [segments+1..endsegments+1]
		two_opt_tuc_cpu_pe(t.begin(), n, w, segments, 1);

		// unlike "text-book" 2-opt algorithms we cannot calculate the 
		// tour length on the fly. Instead we have to calculate the tour length
		// each round.
		const cost_type len = getTourLength( w, t.begin(), n );

		if( (min-len) >= 1 )
		{
			// save better tour and length.
			std::copy( t.begin(), t.end(), tour );
			min = len;

			// reset counter
			counter = 0;
			next = true;
		}
		else
		{
			// iterate until there are no improvements after n-1 rounds.
			counter++;
			next = counter < (n-1);
		}

		if( next )	
		{
			// global left shift.
			std::rotate( t.begin(), t.begin() + 1, t.begin() + n );
		}
		total_counter++;
	}

	timer.Stop();
	if( options.verbose && startLen > min )
	{
		assert( (getTourLength( w, tour, n ) == min) );
		std::cout << "2-opt-tuc-cpu: " << min << " (" << timer.Elapsed() << "ms)" << std::endl;
	}

}

//----------------------------------------------------------------------------
// Finds a tour using a 2-Opt Local Search Algorithm
//
// (TspSolverFunction wrapper for TSPSolver)
//----------------------------------------------------------------------------

cost_type two::tsp_two_opt( std::vector<int>& tour, 
							const Tsp_container& tsp, 
							const TspOptions& options )
{
	Timer timer;

	const int n = tsp.problem_dim;
    std::vector<cost_type> w =  tsp.pbl_adj_mtx;
	infinitize(w,n);

	TwoOptFunction twoOptfn = getTwoOptFunction( options, w, n );
	assert( twoOptfn != nullptr );

	cost_type bestSolution = COST_TYPE_MAX;
	if( twoOptfn )
	{
		std::vector<int> localTour(n);
		bool isTimeUp = false;
		while( !isTimeUp )
		{
			init_array_with_shuffled_indices(localTour.begin(), localTour.end(), n);

			timer.Start();
			twoOptfn(localTour.begin(), n, w, options);
			timer.Stop();

			const cost_type len = getTourLength(w, localTour.begin(), n);
			if( bestSolution > len || tour.empty() )
			{
				bestSolution = len;
				tour.swap(localTour);
				localTour = std::vector<int>(n);
				assert( validateTour(tour,n) );
				if( options.verbose )
					std::cout << "2-opt: " << len << " (" << timer.Elapsed() << "ms)" << std::endl;
			}

			isTimeUp = options.isTimeUp();
		}
	}

	return bestSolution;
}
