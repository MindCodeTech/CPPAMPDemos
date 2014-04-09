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

#include "TspTests.hpp"
#include "TspUtils.hpp"
#include "TspSolver.hpp"

#include <iostream>
#include <assert.h>

using namespace utl;
using namespace opt;
using namespace tst;


//----------------------------------------------------------------------------
// Dumps a tour for debugging purposes
// Used by validateTour().
//----------------------------------------------------------------------------

static void dumpVector( const char* name, 
				 const std::vector<int>& tour)
{
	for( size_t i = 0; i < tour.size(); i++ )
    {
        std::cout << name << "[" << i << "] = " << tour[i] << std::endl;
    }
}

//----------------------------------------------------------------------------
// Validates a tour.
// Used by main().
//----------------------------------------------------------------------------

bool tst::validateTour( const std::vector<int>& tour, 
                   int n, bool printErrors )
{
	// check sizes
	if( tour.size() != n )
	{
		if( printErrors )
		{
			dumpVector( "validateTour", tour );
			std::cout << "validateTour: incorrect tour length " << tour.size() << " instead of " << n << std::endl;
		}
		return false;
	}

	// initialize 'tmp' with NO_PATH
    std::vector<int> tmp(n);
	std::generate( tmp.begin(), tmp.end(), []() { return NO_PATH_INT; } );
    
	// iterate over the whole tour
	for( int i = 0; i < n; i++ )
    {
		const int idx = tour[i];

		// check index out of bounds
		if( idx > n )
        {
			if( printErrors )
			{
				dumpVector( "validateTour", tour );
				std::cout << "validateTour: index out of bounds " << idx << " at " << i << std::endl;
			}
            return false;
        }
            
        // check for duplicate entries
        if( tmp[idx] != -1 )
        {
			if( printErrors )
			{
	            dumpVector( "validateTour", tour );
		        dumpVector( "validateTour", tmp );
				std::cout << "validateTour: duplicate entry " << idx << " at " << i << std::endl;
			}
            return false;
        }

		// store idx to 'tmp' for empty entry detection.
        tmp[idx] = idx;
    }
    
	// by now all entries in 'tmp' should have been visited.
	for( int i = 0; i < n; i++ )
    {
        if( tmp[i] == -1 )
        {
			if( printErrors )
			{
				dumpVector( "validateTour", tour );
				std::cout << "validateTour: empty entry at " << i << std::endl;
			}
            return false;
        }
    }
    
    return true;
}


// emulates direct3d::atomic_exchange
int atomic_exchange( int*dest, int value )
{
	int ret = *dest;
	*dest = value;
	return ret;
}

//----------------------------------------------------------------------------
// Tests parallel rotate
//----------------------------------------------------------------------------

void testRotateParallel(std::vector<int>& tour, int n, 
						int first, int middle, int last )
{
	// const int n = 13;//76; // 8*2;
	if( tour.empty() )
	{
		tour.reserve(n);
		for( int i = 0; i < n; i++ )
			tour.push_back(i);
	}

	std::vector<int> beforeTour(n);
	std::copy(tour.begin(), tour.end(), beforeTour.begin());

	const int savedFirst = first;
	const int savedMiddle = middle;
	const int savedLast = last;

	// Method #2
	int next = middle;
	while(first != next) 
	{
		// swap first with next
		int* next_ptr = &(tour[next]);
		int* first_ptr = &(tour[first]);

		for( size_t i = 0; i < tour.size(); i++ )
			std::cout << tour[i] << " ";
		std::cout << std::endl;
		std:: cout << "Swapping: #1: " << *first_ptr << " <-> " << *next_ptr << std::endl << std::endl;

		atomic_exchange( next_ptr, atomic_exchange(first_ptr, *next_ptr) );

		first++;
		next++;
		if (next == last) 
		{
			next = middle;
		} 
		else if (first == middle) 
		{
			middle = next;
		}
	}

	std::vector<int> method1(n);
	std::copy(tour.begin(), tour.end(), method1.begin());
	
	// Method #2
	first = savedFirst;
	middle = savedMiddle;
	last = savedLast;
	int idx[1];
	std::copy(beforeTour.begin(), beforeTour.end(), tour.begin());

	for( int i = 0; i < n; i++ )
	{
		idx[0] = i;
		if( idx[0] >= first && (idx[0]+1) < last )
		{
			// swap current with next
			int next;
			if( idx[0] >= middle )
				next = idx[0]+1;
			else
				next = middle;

			int* next_ptr = &(tour[next]);
			int* first_ptr = &(tour[idx[0]]);

			for( size_t i = 0; i < tour.size(); i++ )
				std::cout << tour[i] << " ";
			std::cout << std::endl;
			std:: cout << "Swapping #2: " << *first_ptr << " <-> " << *next_ptr << std::endl << std::endl;

			atomic_exchange( next_ptr, atomic_exchange(first_ptr, *next_ptr) );
		}

	}

	std::vector<int> method2(n);
	std::copy(tour.begin(), tour.end(), method2.begin());


	std::vector<int> validateTour(beforeTour.size());
	std::copy(beforeTour.begin(), beforeTour.end(), validateTour.begin());

	first = savedFirst;
	middle = savedMiddle;
	last = savedLast;
	std::rotate(validateTour.begin() + first, validateTour.begin() + middle, validateTour.begin() + last);
	
	for( size_t i = 0; i < validateTour.size(); i++ )
	{
		if( validateTour[i] != method1[i] )
			std::cerr << "Error (method #1): at " << i << " " << validateTour[i] << " != " << method1[i] << std::endl;
		if( validateTour[i] != method2[i] )
			std::cerr << "Error (method #2): at " << i << " " << validateTour[i] << " != " << method2[i] << std::endl;
	}

}


//----------------------------------------------------------------------------
// Tests indexing matrixes
//----------------------------------------------------------------------------


int row_index(int i, int M)
{
    const int ii = (M*(M+1))/2-1-i;
    const int K = static_cast<int>(floor((sqrt(8*ii+1)-1)/2));
    return M-1-K;
}

int column_index(int i, int M)
{
    const int ii = (M*(M+1))/2-1-i;
    const int K = static_cast<int>(floor((sqrt(8*ii+1)-1)/2));
    return i - M*(M+1)/2 + ((K+1)*(K+2))/2;
}

void validateIndexMatrix(const std::vector<int>& tmp, int n )
{
	for( int i = 0; i < n; i++ )
	{
		for( int j = 0; j < n; j++ )
		{
			if( j >= i )
			{
				if( tmp[i*n+j] != 0 )
					std::cerr << "Error at " << i << ", " << j << " = " << tmp[i*n+j] << " (expecting 0)." << std:: endl;
			}
			else
			{
				if( tmp[i*n+j] != 1 )
					std::cerr << "Error at " << i << ", " << j << " = " << tmp[i*n+j] << " (expecting 1)."<< std:: endl;
			}
		}
	}
}

void testIndexing( int n, int tile_size )
{
	for( int i = 0; i < n; i++ )
	{
		std::cout << column_index( i, n ) << " ";
	}
	std::cout << std::endl;

	// max_tiles, emulates pad()
	int max_tiles = n/tile_size;
	if( (n % tile_size) != 0 )
		max_tiles++;

	std::vector<int> tmp(n*n);
	std::fill( tmp.begin(), tmp.end(), 0 );
	for( int tile = 0; tile < max_tiles; tile++ )
	{
		for( int i = 0; i < tile_size; i++ )
		{
			const int global_i = tile*tile_size+i;
			if( global_i < n )
			{
				for( int j = 0; j < global_i; j++ )
				{
					tmp[global_i*n + j]++;
				}
			}
			// std::cout << "Calculations: " << global_i << " for idx " << tile << ", " << i << std::endl;
		}
	}

	std::cout << std::endl;
	validateIndexMatrix(tmp,n);

	// http://stackoverflow.com/questions/242711/algorithm-for-index-numbers-of-triangular-matrix-coefficients
	
	const int counter = (n-2)*(n-1)/2;
	int iterations = (counter/tile_size);
	if( (counter % tile_size) != 0 )
		iterations++;

	std::cout << "N = " << n << std::endl;
	std::cout << "Tiles = " << max_tiles << std::endl;
	std::cout << "Tile Size = " << tile_size << std::endl;
	std::cout << "Counter = " << counter << std::endl;
	std::cout << "Iterations = " << iterations << std::endl;


	std::fill( tmp.begin(), tmp.end(), 0 );
	for( int tile = 0; tile < max_tiles; tile++ )
	{
		for( int thread = 0; thread < tile_size; thread++ )
		{
			// std::cout << "CCC:" << (tile*tile_size+thread*n) << std::endl;
			const int global_i = tile*tile_size+thread;
			if( global_i < n )
			{
				if( global_i > 0 )
					tmp[global_i*n]++;

				//each thread performs iter inner iterations in order to reuse the shared memory
				for( int no = 0; no < iterations; no++) 
				{	
					// no * blockDim.x*gridDim.x + (threadIdx_x + blockIdx.x * blockDim.x)
					int id = no*n + global_i;
					if (id < counter) 
					{ 	 				
						//a nasty formula to calculate the right cell of a triangular matrix index based on the thread id
						const int i = static_cast<int>((3+ sqrt(8.0f*id+1.0f))/2); 
						const int j = id - (i-2)*(i-1)/2 + 1; 
						tmp[i*n + j]++;
						// std::cout << " [" << i << "," << j << "]";
					}
				}
				// std::cout << "Calculations: " << iterations << " for idx " << tile << ", " << thread << std::endl;
			}
		}
	}

	std::cout << std::endl;
	validateIndexMatrix(tmp,n);

	std::cout << std::endl;
}



//----------------------------------------------------------------------------
// Used by testTwoOptParallel below
//----------------------------------------------------------------------------

void testTwoOptMove(std::vector<int>& tour, int segments, std::vector<bool>& visited, int n, int offset = 0)
{
	// printSolution( tour );
	
	const int endSegments = segments * 2;
	for( int i = 1; i < segments; i++ )
	{	
		const int c2 = i-1;	// C2
		const int c3 = i; // C3
		const int c7 = endSegments-i+offset-1; // C7
		const int c8 = endSegments-i+offset; // C8

		std::cout << "[" << tour[c2] << "," << tour[c3] << "," <<  tour[c7] << "," <<  tour[c8] << "]" << std::endl;
		// visited[tour[c2]*n+tour[c7]] = visited[tour[c3]*n+tour[c8]] = true;
		// visited[tour[c2]*n+tour[c3]] = visited[tour[c7]*n+tour[c8]] = true;
		visited[tour[c3]*n+tour[c7]] = visited[tour[c7]*n+tour[c3]] = true;
		// std::swap_ranges( tour.begin() + i, tour.begin() + i + 1, tour.begin() + endSegments-i-1 );
	}
}

//----------------------------------------------------------------------------
// Tests parallel 2-opt implementation as proposed by 
// Hardware Implementation of 2-Opt Local Search Algorithm for the Traveling Salesman Problem
// http://dl.acm.org/citation.cfm?id=1263915
// http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=4228483&tag=1
//
// Modified method, that avoids rotation.
//----------------------------------------------------------------------------

void testTwoOptParallel(std::vector<int>& tour, int n)
{
	// const int n = 13;//76; // 8*2;
	if( tour.empty() )
	{
		tour.reserve(n);
		for( int i = 0; i < n; i++ )
			tour.push_back(i);
	}

	int segments = (n/4 % 2) ? ((n+1)/4) : (n/4);
	if( ((n/2)%2) != 0 ) 
		segments++;

	const int endSegments = segments * 2;

	std::cout << "Segments = " << segments << std::endl;
	std::cout << "End segments = " << endSegments << std::endl;

	std::vector<bool> visited(n*n);
	std::fill(visited.begin(), visited.end(), false);

	const size_t first = 0;
	const size_t last = n;
	const size_t middle = n-1;

	for( int j = 0; j < n; j++ )
	{
		std::cout << "-----" << j << "-----" << std::endl;

		for( int k = 0; k < 2 /*n/2*/; k++ )
			testTwoOptMove(tour, segments, visited, n, k);

		std::rotate( tour.begin() + first, tour.begin() + middle, tour.begin() + last );
	}

	for( int i = 0; i < n; i++ )
	{
		for( int j = 0; j < n; j++ )
			if( i != j && !visited[i*n+j] )
				std::cout << "Not visited:[" << i << "," << j << "]" << std::endl; 
	}

	visited.clear();
}

//----------------------------------------------------------------------------
// Tests parallel 2-opt implementation as proposed by 
// Hardware Implementation of 2-Opt Local Search Algorithm for the Traveling Salesman Problem
// http://dl.acm.org/citation.cfm?id=1263915
// http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=4228483&tag=1
//
// Original method.
//----------------------------------------------------------------------------

void testTwoOptParallelOriginal(std::vector<int>& tour, int n)
{
	// const int n = 13;//76; // 8*2;
	if( tour.empty() )
	{
		tour.reserve(n);
		for( int i = 0; i < n; i++ )
			tour.push_back(i);
	}


	int segments = (n/4 % 2) ? ((n+1)/4) : (n/4);
	if( ((n/2)%2) != 0 ) 
		segments++;

	const int endSegments = segments * 2;

	std::cout << "Segments = " << segments << std::endl;
	std::cout << "End segments = " << endSegments << std::endl;

	std::vector<bool> visited(n*n);
	std::fill(visited.begin(), visited.end(), false);

	for( int j = 0; j < n; j++ )
	{
		std::cout << "-----" << j << "-----" << std::endl;
		testTwoOptMove(tour, segments, visited, n);

		size_t first = segments;
		size_t last = n;
		size_t middle = segments + 1;
		std::rotate( tour.begin() + first, tour.begin() + middle, tour.begin() + last );
		std::cout << "---" << std::endl;
		testTwoOptMove(tour, segments, visited, n);

		// first = segments;
		middle = n - 1;
		// last = n;
		std::rotate( tour.begin() + first, tour.begin() + middle, tour.begin() + last );

		// "For the third and final step, we perform a right circular 
		// shift to all 200 cities."
		
		first = 0;
		// middle = n - 1;
		// last = n;
		std::rotate( tour.begin() + first, tour.begin() + middle, tour.begin() + last );
	}

	for( int i = 0; i < n; i++ )
	{
		for( int j = 0; j < n; j++ )
			if( i != j && !visited[i*n+j] )
				std::cout << "Not visited:[" << i << "," << j << "]" << std::endl; 
	}

	visited.clear();
}

//----------------------------------------------------------------------------
// Tests validateTour() and getTourLength() against known data from pr76.tsp
//----------------------------------------------------------------------------

cost_type tst::test_pr76( const std::vector<cost_type>& w, int n)
{   
	// 'tour_data' contains the information from
	// http://www.iwr.uni-heidelberg.de/groups/comopt/software/TSPLIB95/tsp/pr76.tsp.gz
    int tour_data[] = { 
        0,22,21,20,24,23,45,44,43,47,
        46,68,67,69,66,49,48,50,65,64,
        70,71,72,63,62,61,60,40,59,58,
        57,56,55,54,51,52,53,41,42,27,
        26,25,19,18,30,29,28,31,32,34,
        33,39,38,37,35,36,17,16,15,14,
        73,13,12,11,10,9,8,7,6,5,
        4,3,2,1,74,75
    };
    
	// copy tour_data to tour.
	// (Using std::generate with lambda might be more elegant...)
	std::vector<int> tour(n);
	for( int i = 0; i < n; i++ )
		tour[i] = tour_data[i];

	// 'validateTour' and 'getTourLength' are used by main()
	validateTour(tour,n);
    cost_type len = getTourLength(w, tour.begin(), n );

	// pr76 is reported to have 108159 as the best known solution.
	// See http://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/STSP.html
    std::cout << "pr76=" << len << " (=108159?)" << std::endl;

	// let's stop if the numbers don't match.
	assert( static_cast<unsigned int>(len) == 108159 );

    return len;
}

//----------------------------------------------------------------------------
// Compares two n*n matrices and reports and error if they differ.
// Used by testCreateWeightMatrix().
//----------------------------------------------------------------------------

bool compareMatrices( const std::vector<cost_type>& m1, const std::vector<cost_type>& m2, int n )
{
	for( int i = 0; i < n ; i++ )
    {
		for( int j = 0; j < n; j++ )
		{
			if( !fp_equal(m1[i*n+j], m2[i*n+j], 0.5f) )
			{
				std::cerr << "Error: " << std::endl;
				std::cerr << "m1[" << i << "," << j << "] = m1[" << i*n+j << "] = " << m1[i*n+j] << std::endl;
				std::cerr << "m2[" << i << "," << j << "] = m2[" << i*n+j << "] = " << m2[i*n+j] << std::endl;
				return false;
			}
		}
    }
	return true;
}

// emulates:
// correct: "inside-out" reversal, rotate to i, then reverse j to n.
// amp_rotate( t, 0, best.i + 1, cities);
// amp_reverse(t, best.j - best.i - 1, cities);	
// unrotate...

// Test cases::
// std::vector<int> tour;
// tst::testReverseInPlace(tour, 10, 0, 0, 6, 10); // [0,1,2,3,4,5,6,9,8,7]
// tst::testReverseInPlace(tour, 10, 0, 7, 9, 10); // [6,5,4,3,2,1,0,7,8,9]
// tst::testReverseInPlace(tour, 10, 0, 1, 7, 10); // [8,1,2,3,4,5,6,7,0,9]
// tst::testReverseInPlace(tour, 10, 0, 6, 8, 10);	// [4,3,2,1,0,9,6,7,8,5]

void tst::testReverseInPlace(std::vector<int>& tour, int n, 
						int first, int left, int right, int last)
{
	// const int n = 13;//76; // 8*2;
	if( tour.empty() )
	{
		tour.reserve(n);
		for( int i = 0; i < n; i++ )
			tour.push_back(i);
	}

	std::vector<int> beforeTour(n);
	std::copy(tour.begin(), tour.end(), beforeTour.begin());

	std::vector<int> method1(n);
	std::copy(tour.begin(), tour.end(), method1.begin());

	/*
	if( first == left )
		std::reverse(tour.begin()+right, tour.end() );
	else if( right == last )
		std::reverse(tour.begin(), tour.begin()+left );
	else
	*/
	{
		int lhs = left;
		int rhs = right;

		int* next_ptr;
		int* first_ptr;

		// swapping elements between first and left and right and last
		while( rhs != lhs )
		{
			if( lhs-- <= first )
				lhs = last-1;

			if( rhs == lhs )
				break;

			if( rhs++ >= last-1 )
				rhs = first;

			if( rhs == lhs )
				break;

			if( rhs < lhs )
			{
				next_ptr = &(method1[rhs]);
				first_ptr = &(method1[lhs]);
			}
			else
			{
				next_ptr = &(method1[lhs]);
				first_ptr = &(method1[rhs]);
			}
			atomic_exchange( next_ptr, atomic_exchange(first_ptr, *next_ptr) );
		}
	}

	std::vector<int> validateTour(beforeTour.size());
	std::copy(beforeTour.begin(), beforeTour.end(), validateTour.begin());
	std::rotate(validateTour.begin() + first, validateTour.begin() + left, validateTour.begin() + last);
	std::reverse(validateTour.begin() + right + 1, validateTour.begin() + last);
	std::rotate(validateTour.begin() + first, validateTour.begin() + last - left, validateTour.begin() + last);
	
	for( size_t i = 0; i < validateTour.size(); i++ )
	{
		if( validateTour[i] != method1[i] )
			std::cerr << "Error (method #1): at " << i << " " << validateTour[i] << " != " << method1[i] << std::endl;
	}
}

//----------------------------------------------------------------------------
// Tests converting an ATSP weight matrix to a TSP weight matrix.
// see http://en.wikipedia.org/wiki/Travelling_salesman_problem#Solving_by_conversion_to_symmetric_TSP
//----------------------------------------------------------------------------




void tst::convertATSPToTSP()
{
    const int n = 3;
    std::vector<cost_type> w(n*n);

    /*
                    A       B       C
            A               1       2
            B       6               3
            C       5       4
    */

    // A row
    w[0*n+0] = COST_TYPE_MAX;       // A:A
    w[0*n+1] = 1;                           // A:B
    w[0*n+2] = 2;                           // A:C

    // B row
    w[1*n+0] = 6;                           // B:A
    w[1*n+1] = COST_TYPE_MAX;       // B:B
    w[1*n+2] = 3;                           // B:C

    // C row
    w[2*n+0] = 5;                           // C:A
    w[2*n+1] = 4;                           // C:B
    w[2*n+2] = COST_TYPE_MAX;       // C:C


    // now convert the matrix

    /*              A       B       C       A'      B'      C'
            A                               0       6       5
            B                               1       0       4
            C                               2       3       0
            A'      0       1       2
            B'      6       0       3
            C'      5       4       0
    */

    cost_type tmp;

    // print out w
    for( int i = 0; i < n; i++ )
    {
            for( int j = 0; j < n; j++ )
            {
                    tmp = w[i*n+j];
                    if(tmp == COST_TYPE_MAX)
                            std::cout <<  "\t ";
                    else
                            std::cout <<  "\t" << tmp;
            }
            std::cout << std::endl;
    }
    std::cout << "---" << std::endl;

    std::vector<cost_type> w2;
	convertTSPtoATSPMatrix( w, n, w2 );

    // print out w2
    for( int i = 0; i < n*2; i++ )
    {
            for( int j = 0; j < n*2; j++ )
            {
                    tmp = w2[i*n*2+j];
                    if(tmp == COST_TYPE_MAX)
                            std::cout <<  "\t ";
                    else
                            std::cout <<  "\t" << tmp;
            }
            std::cout << std::endl;
    }

    // prepare Tsp_container
    pbl::Tsp_container tsp;
    tsp.problem_dim = n*2;
    tsp.problem_nam = "Test";
    tsp.pbl_adj_mtx = w2;

    // prepare options
    opt::TspOptions options;
    options.use_cpu = true;
    options.use_gpu = false;

    slv::TspSolver solver;
    solver.set_params(options);
    solver.solve(tsp);

    std::vector<int> tour;
    solver.getSolution(tour);

    // printSolution(tour);

    std::vector<int> tour2(n);
	convertTSPToATSPTour( w2, tour, tour2, n, true);

	cost_type w2len = getTourLength(w2, tour.begin(), static_cast<int>(tour.size()));
	std::cout << "Tour: " << w2len << std::endl;
    printSolution(tour2);

	cost_type wlen = getTourLength(w, tour2.begin(), static_cast<int>(tour2.size()));
    std::cout << "Tour2: " << wlen << std::endl;

	if( wlen != w2len )
	{
	    std::vector<int> tour3(n);
		tour2.swap(tour3);
		convertTSPToATSPTour( w2, tour, tour2, n, true);
	}

	assert( wlen == w2len );

    std::cout << std::endl;
}