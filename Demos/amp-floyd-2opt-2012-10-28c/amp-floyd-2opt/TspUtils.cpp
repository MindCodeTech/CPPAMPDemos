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

#include "TspUtils.hpp"
#include "TspTests.hpp"
#include "TspFloydSolver.hpp"

#include <iostream>
#include <fstream>
#include <assert.h>

//----------------------------------------------------------------------------
// Namespaces
//----------------------------------------------------------------------------

using namespace utl;
using namespace opt;
using namespace tst;
using namespace flw;

//----------------------------------------------------------------------------
// Source: Parallel Reduction using C++ AMP
// http://blogs.msdn.com/b/nativeconcurrency/archive/2012/03/08/parallel-reduction-using-c-amp.aspx
//
// Helper function comparing floating point numbers within a given relative
// difference.
//----------------------------------------------------------------------------

bool utl::fp_equal(float a, float b, float max_rel_diff)
{
    float diff = std::fabs(a - b);
    a = std::fabs(a);
    b = std::fabs(b);
    return diff <= std::max(a, b) * max_rel_diff;
}

//----------------------------------------------------------------------------
// Helper function for rounding a float.
//----------------------------------------------------------------------------

float utl::roundFloat(float r) 
{
    return (r > 0.0f) ? floor(r + 0.5f) : ceil(r - 0.5f);
}

//----------------------------------------------------------------------------
// Returns the total length of a tour using distance matrix 'w'.
//----------------------------------------------------------------------------

cost_type utl::getTourLength( const std::vector<cost_type>& w, const std::vector<int>::const_iterator& tour, int n )
{	
	// connect the dots by adding up distances 
	// provided by 'w'.
	cost_type len = 0;
	const int p0 = tour[0];
	int p1 = p0;
    for( int i = 1; i < n; i++ )
	{
		const int p2 = tour[i];
        len +=  w[p1*n +p2];
		p1 = p2;
	}
	
	// return to origin. 
    len += w[p1*n + p0];
	return len;
}

//----------------------------------------------------------------------------
// Returns the total length of a partial tour using distance matrix 'w'.
//----------------------------------------------------------------------------

cost_type utl::getPartialTourLength( const std::vector<cost_type>& w, const std::vector<int>& tour, int n, bool close )
{	
	// connect the dots by adding up distances 
	// provided by 'w'.
	cost_type len = 0;
	const int p0 = tour[0];
	const size_t tourSize = tour.size();
	int p1 = p0;
    for( size_t i = 1; i < tourSize; i++ )
	{
		const int p2 = tour[i];
        len +=  w[p1*n +p2];
		p1 = p2;
	}
	
	// return to origin.
	if( close )
		len += w[p1*n + p0];
	return len;
}

//----------------------------------------------------------------------------
// Set w[i][column] to COST_TYPE_MAX and visited[column] to true.
// Used by tsp_floyd_gpu().
//----------------------------------------------------------------------------

void utl::markVisited( std::vector<cost_type>& w, int n, int column, std::vector<bool>& visited )
{
	if( !visited[column] )
	{
		// Setting a column to COST_TYPE_MAX is equivalent to marking a column as visited
		// as findTour extracts the minimum of a row (i.e. via std::min_element()).
		// A column that carries COST_TYPE_MAX will be naturally excluded.
		for( int i = 0; i < n; i++ )
			w[i*n+column] = COST_TYPE_MAX;

		// The same information is cached in 'visited' for performance reasons.
		visited[column] = true;
	}
}

//----------------------------------------------------------------------------
// Writes out a tour that corresponds to a solution.
//----------------------------------------------------------------------------

void utl::writeSolution( std::ostream& ostr, 
						const std::vector<int>& tour, 
						const pbl::Tsp_container& tsp, 
						const std::string& comment )
{

	/*
	For example:

		NAME : gr120.opt.tour
		COMMENT : Optimal tour for gr120 (6942)
		TYPE : TOUR
		DIMENSION : 120
		TOUR_SECTION
		1
		2
		3
		...
		-1
		EOF
	*/
	ostr << "NAME : " << tsp.problem_nam << std::endl;
	ostr << "COMMENT : " << comment << std::endl;
	ostr << "TYPE : TOUR" << std::endl;
	ostr << "DIMENSION : " << tsp.problem_dim << std::endl;
	ostr << "TOUR_SECTION" << std::endl;

	for( size_t i = 0; i < tour.size(); i++ )
    {
        ostr << tour[i] << std::endl;
    }
       
	ostr << "-1" << std::endl;
	ostr << "EOF" << std::endl;
}

void utl::printSolution( const std::vector<int>& tour, int nodesPerLine )
{
	for( size_t i = 0; i < tour.size(); i++ )
    {
        std::cout << tour[i] << " ";

		// break the line at 'nodesPerLine'
		if( i > 0 && ((i+1) % nodesPerLine) == 0 )
			std::cout << std::endl;
    }
	std::cout << std::endl;
}

//----------------------------------------------------------------------------
// Returns true if 'w' is symmetrical, i.e [i,j] == [j,i]
//----------------------------------------------------------------------------

bool utl::isSymmetrical(const std::vector<cost_type>& w, int n)
{
	for( int i = 0; i < n; i++ )
	{
		for( int j = 0; j < n; j++ )
			if( w[i*n+j] != w[j*n+i] )
				return false;
	}
	return true;
}

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

void utl::init_array_with_shuffled_indices( std::vector<int>::iterator outBegin, std::vector<int>::iterator outEnd, int n )
{
	// const int n = static_cast<int>(outEnd - outBegin);
	std::vector<int>::iterator& out = outBegin;

	// a[0] ← source[0]
	out[0] = 0;

	// for i from 1 to n − 1 do
	for( int i = 1; i < n; i++ )
	{
		// j ← random integer with 0 ≤ j ≤ i
		const int j = rand() % (i+1);

		if( i != j )
		{
			// a[i] ← a[j]
			out[i] = out[j];
		}
					
		// a[j] ← source[i]
		out[j] = i;
	}
}

//----------------------------------------------------------------------------
// Set w[i][i] to COST_TYPE_MAX 
//----------------------------------------------------------------------------

void utl::infinitize( std::vector<cost_type>& w, int n )
{
	// Technically the distance between two cities, which are
	// the same, is always 0. For our purposes we need to mark
	// those points on the diagonal [i,i] as visited by 
	// setting the value to COST_TYPE_MAX.
	//
	// Setting a column to COST_TYPE_MAX is equivalent to marking a column as visited
	// as findTour extracts the minimum of a row (i.e. via std::min_element()).
	// A column that carries COST_TYPE_MAX will be naturally excluded.

	for( int i = 0; i < n; i++ )
		w[i*n+i] = COST_TYPE_MAX;
}

//----------------------------------------------------------------------------
// Tests converting an ATSP weight matrix to the corresponding TSP weight matrix.
// see http://en.wikipedia.org/wiki/Travelling_salesman_problem#Solving_by_conversion_to_symmetric_TSP
//----------------------------------------------------------------------------

void utl::convertTSPtoATSPMatrix(const std::vector<cost_type>& w, int n, std::vector<cost_type>&out)
{
	cost_type tmp;
    cost_type substituteForZero = 0.0f; // 1.0f;
	cost_type localMax = COST_TYPE_MAX; // 1000000.0f;

	// It is probably just faster to initialize w2 as we go.
	// But this version is safer, because I don't miss any field 
	// when initializing w2.
	std::vector<cost_type> w2(n*2*n*2);
	std::generate(w2.begin(), w2.end(), [localMax]() { return localMax; });

	if( localMax != COST_TYPE_MAX )
		infinitize( w2, n*2 );

    const int n2 = n*2;
	const int n2n = n2*n;
    for( int i = 0; i < n; i++ )
    {
        const int in2 = i*n2;
        const int in2n = in2+n;

        for( int j = 0; j < n; j++ )
        {
                const int in2j = in2 + j;
                const int in2jn = in2n + j;

                // Upper-left (A:A to C:C) quadrant: infinitize
                // w2[i*n*2+j] = 1;
                // w2[in2j] = COST_TYPE_MAX;

                // Lower-left (A':A to C':C) quadrant: copy of w
                // w2[n*2*n+i*n*2+j] = 2;
                tmp = w[i*n+j];
                // w2[n2n+in2j] = tmp == COST_TYPE_MAX ? 0 : (tmp == 0 ? 0.00001f : tmp);
				if(tmp == 0) tmp = substituteForZero;
				w2[n2n+in2j] = tmp == COST_TYPE_MAX ? 0 : tmp;

                // Upper-right (A:A' to C:C') quadrant: inverse copy of w
                // w2[i*n*2+n+j] = 3;
                tmp = w[j*n+i];
				if(tmp == 0) tmp = substituteForZero;
                // w2[in2jn] = tmp == COST_TYPE_MAX ? 0 : (tmp == 0 ? 0.00001f : tmp);
				w2[in2jn] = tmp == COST_TYPE_MAX ? 0 : tmp;

                // Lower-right (A':A' to C':C') quadrant: infinitize
                // w2[n*2*n+i*n*2+n+j] = 4;
                // w2[n2n+in2jn] = COST_TYPE_MAX;
        }
    }


	/*
	// @@@ DEBUG
    // print out w2
	std::ofstream matFile;
	matFile.open ("C:\\Users\\bparadie\\Desktop\\convertTSPtoATSPMatrix.txt");
    for( int i = 0; i < n*2; i++ )
    {
        for( int j = 0; j < n*2; j++ )
        {
            tmp = w2[i*n*2+j];
            if(tmp == COST_TYPE_MAX)
                matFile << "x,";
            else
                matFile << tmp << ",";
        }
        matFile << std::endl;
    }
	matFile.close();
	*/

	out.swap(w2);

	assert( isSymmetrical(out,n*2) );
}


//----------------------------------------------------------------------------
// Helper function for dumping out tours
//----------------------------------------------------------------------------

static void dumpTour( std::vector<int>& tour, int n, bool debug)
{
	// print out tour3 for debugging purposes
	if( debug )
	{
		for( int i = 0; i < n; i++ )
		{
			std::cout << "[" << i << "]: " << tour[i] << std::endl;
		}
		std::cout << std::endl;
	}

}

//----------------------------------------------------------------------------
// Helper function used by convertATSPToTSPTour
//----------------------------------------------------------------------------

static bool findNode(const std::vector<int>& tour3, int start, int node)
{
	// protection against loops
	std::vector<bool> visited(tour3.size());
	std::generate(visited.begin(), visited.end(), [](){ return false;} );

	int next = tour3[start];
	while(next != -1)
	{
		if( next == node )
			return true;
		if( visited[next] )
			return true;
		visited[next] = true;
		next = tour3[next];
	}
	return false;
}

//----------------------------------------------------------------------------
// Helper function used by convertATSPToTSPTour
//----------------------------------------------------------------------------

static int findRootNode(const std::vector<int>& parents, int p1, int p2)
{
	// protection against loops
	std::vector<bool> visited(parents.size());
	std::generate(visited.begin(), visited.end(), [](){ return false;} );

	const int parent = p2 == -1 ? -1 : parents[p2];
	int defRoot = p1;
	while( parents[defRoot] != -1 )
	{
		if( parent == p1 )
			return -1;
		if( defRoot == p2 )
			return -1;
		if( visited[defRoot] )
			return true;
		visited[defRoot] = true;
		defRoot = parents[defRoot];
	}
	return defRoot;
}

//----------------------------------------------------------------------------
// Helper function used by convertATSPToTSPTour
//----------------------------------------------------------------------------

static bool findNodeInChain(const std::vector<int>& tour3, const std::vector<int>& parents, int start, int node)
{
	if( start == node )
		return true;
	if( findNode(tour3, start, node) )
		return true;
	if( !findRootNode(parents, start, node) )
		return true;
	return false;
}


//----------------------------------------------------------------------------
// Helper function used by convertATSPToTSPTour
//----------------------------------------------------------------------------

static bool connectNodes(	std::vector<int>& tour3, 
				std::vector<int>& parents,
				std::vector<bool>& visited,
				std::vector<int>& deferred,
				int p1,
				int p2,
				bool checkForCycles)
{
	if( checkForCycles )
	{
		const int p2root = findRootNode(parents, p2, p1);
		if( p2root == -1 )
			return false;

		int next = p1;
		while( parents[next] != -1 )
		{
			if( findNode( tour3, p2root, next ) )
				return false;
			next = parents[next];
		}
	}

	assert( p1 != -1 );
	assert( p2 != -1 );

	if( tour3[p1] != -1 && parents[tour3[p1]] != -1 )
		parents[tour3[p1]] = -1;

	tour3[p1] = p2;
	visited[p2] = true;
	parents[p2] = p1;
	deferred[p1] = -1;
	return true;
}

//----------------------------------------------------------------------------
// Helper function used by convertATSPToTSPTour
//----------------------------------------------------------------------------

static int reduceTour(const std::vector<cost_type>&w, 
				const std::vector<int>&tour, 
				std::vector<int>& tour3, 
				std::vector<int>& parents,
				std::vector<bool>& visited,
				std::vector<int>& deferred,
				int n,
				int p1,
				int p2,
				bool debug)
{
    const cost_type d = w[p1*n*2+p2];
			
	if( d != COST_TYPE_MAX )
	{
		int lhs, rhs;

		if( p1 > p2 )
		{
			// Lower-left quadrant
			assert( p2 < n );
			lhs = p1-n;
			rhs = p2;
		}
		else
		{
			// Upper-right quadrant
			assert( p1 < n );
			lhs = p2-n;
			rhs = p1;
		}

		if( lhs != rhs )
		{
			if( d == 0.0f )
			{
				// let the weight carrying nodes have first dips!
				deferred[lhs] = rhs;
			}
			else if(tour3[rhs] != -1)
			{
				// collision!
				deferred[lhs] = rhs;
			}
			else
			{
				if( !connectNodes( tour3, parents, visited, deferred, lhs, rhs, true ) )
					deferred[lhs] = rhs;
			}

			if(debug)
				std::cout << p1 << " -> " << p2 << " = " << d << " [" << lhs << "]: " << rhs << std::endl;
		}
	}
	else
	{
		if(debug)
			std::cout << p1 << " -> " << p2 << " = " << d << std::endl;
	}
    p1 = p2;

	return p1;
}

//----------------------------------------------------------------------------
// Helper function for resolving deferred nodes
//----------------------------------------------------------------------------

static bool resolveDeferred(
				std::vector<int>& tour3, 
				std::vector<int>& parents,
				std::vector<bool>& visited,
				std::vector<int>& deferred,
				int def,
				bool debug,
				const std::vector<cost_type>&weigthMatrix,
				const std::vector<cost_type>& floyd,
				const std::vector<int>& next,
				int n)
{
	const int p2 = deferred[def];
	assert( p2 != -1 );

	if( findNode( tour3, p2, def ) )
		return false;


	// bad, but our only choice
	//
	// before:
	//			parent -> p2
	//			def -> p2
	// after:
	//			parent -> def -> p2
	//
	// But we get taxed for parent -> def
	//
	const int parent = parents[p2];
	if( parent == -1 )
		return false;

	assert( tour3[parent] == p2 );

	if( debug )
	{
		std::cout << "Collision: " << parent << " -> " << p2 << " and "
									<< def << " -> " << p2 << std::endl;
	}
	
	const int defRoot = findRootNode(parents, def, p2);
	if( defRoot != -1 )
	{
		/*
		bparadie, 2012-10-28: doesn't make any difference...

		const bool useFloyd = !floyd.empty();
		if(useFloyd)
		{
			std::vector<int> outTour;
			std::vector<bool> visited2 = visited;

			std::vector<cost_type>w = floyd;
			const cost_type cost = addVertices( weigthMatrix, w, next, n, parent, defRoot, outTour, visited2 );

			if( cost != 0 )
				outTour.clear();
			if(!outTour.empty())
				outTour.clear();
		}
		*/

		if( !connectNodes(tour3, parents, visited, deferred, parent, defRoot, true) )
			return false;
		if( !connectNodes(tour3, parents, visited, deferred, def, p2, true) )
			return false;
	}

	if( debug )
	{
		std::cout << "Collision: " << parent << " -> " << p2 << " and "
									<< def << " -> " << p2 << std::endl;
		std::cout << "Resolved: " << parent << " -> " << defRoot << " -> ... -> "
									<< def << " -> " << p2 << std::endl;
		dumpTour( tour3, tour3.size(), true );
	}
	return true;
}

//----------------------------------------------------------------------------
// Helper function for resolving "strange loops"
//----------------------------------------------------------------------------

static bool resolveStrangeLoops(
				std::vector<int>& tour3, 
				std::vector<int>& parents,
				std::vector<bool>& visited,
				std::vector<int>& deferred,
				int def,
				bool isLastLoop,
				bool debug)
{
	bool resolved = false;
	if( isLastLoop )
	{
		for( size_t i = 0; i < visited.size(); i++ )
		{
			if( !visited[i] )
			{
				assert( def != i );

				connectNodes( tour3, parents, visited, deferred, def, i, false );
				// tour3[def] = i;
				resolved = true;


				if( debug )
				{
					std::cout << "Resolved last strange loop: " 
								<< def << " -> " << i << std::endl;
				}
				break;
			}
		}

		// assert(resolved);

		if( !resolved )
		{
			std::generate( visited.begin(), visited.end(), []() { return false; } );
			for( size_t i = 0; i < tour3.size(); i++ )
			{
				visited[tour3[i]] = true;
			}

			for( size_t i = 0; i < visited.size(); i++ )
			{
				if( !visited[i] )
				{
					assert( def != i );
					tour3[def] = i;
					resolved = true;

					if( debug )
					{
						std::cout << "Resolved last strange loop: " 
									<< def << " -> " << i << std::endl;
					}
					break;
				}
			}
		}
	}
	else
	{
		if( tour3[def] != -1 )
			return true;

		const int defRoot = findRootNode(parents, def, -1);
		assert( defRoot != -1 );

		for( size_t i = 0; i < tour3.size(); i++ )
		{
			// we might be lucky and discover that our defRoot is already in a chain
			if( tour3[i] != -1 && !findNodeInChain(tour3, parents, defRoot, tour3[i] ) ) 
			{
				const int rhsRoot = findRootNode(parents, tour3[i], defRoot);
				if( rhsRoot != -1 )
				{
					if( connectNodes( tour3, parents, visited, deferred, def, rhsRoot, true) )
					{
						if(debug)
						{
							std::cout << "Resolved strange loop: " 
									<< def << " -> " << rhsRoot << " -> ... -> " << tour3[i] << std::endl;
						}
						return true;
					}
				}
			}
		}
	}	
	return resolved;
}

//----------------------------------------------------------------------------
// Tests converting a tour based an ATSP weight matrix to the corresponding TSP tour.
// see http://en.wikipedia.org/wiki/Travelling_salesman_problem#Solving_by_conversion_to_symmetric_TSP
//----------------------------------------------------------------------------

bool utl::convertTSPToATSPTour(const std::vector<cost_type>&w, 
							   const std::vector<int>&tour, 
							   std::vector<int>& outTour, 
							   int n,
							   bool verbose)
{
	// bparadie, 2012-10-27: What I am trying here has perhaps never been done before...
	// (Well, at least not in two evenings!)
	//
	// According to Roy Jonker and Ton Volgenant in "Transforming asymmetric into symmetric traveling salesman problems", 
	// an asymmetric matrix of size N can be converted into a symmetric matrix of size 2N.
	// The original ATSP matrix is in the lower left corner while a "flipped" version is placed in the upper right
	// corner of the enlarged matrix.
	// After the conversion you can indeed run any TSP solver on that converted matrix and astonishingly you will 
	// get accurate lengths back. But reconstructing the paths is not trivial, perhaps not even possible.
	// convertATSPToTSPTour() tries to do exactly that!
	const bool debug = false; // true;

	if(verbose)
	{
		std::cout << std::endl;
		std::cout << "Reconstrucing ATSP tour from TSP tour..." << std::endl;
	}

	assert( validateTour( tour, n*2 ) );

	// initialize tour3, which will contain the reconstructed path, with -1
    std::vector<int> tour3(n);
	std::generate( tour3.begin(), tour3.end(), []() { return -1; } );

	// visited keeps track of visited nodes.
	// i.e. after assigning tour3[p1] = p2;
	// it is 'p2' that gets marked as visited.
	std::vector<bool> visited(n);
	std::generate( visited.begin(), visited.end(), []() { return false; } );

	std::vector<bool> collected(n);
	collected = visited;

	// during iteration of the expanded tour collicions may happen.
	// This implementation uses a use first-come, first-serve rule.
	// i.e. if we want to assign tour3[p1] = p2;
	// and tour3[p1] has already a value assigned to it
	// we'll save p2 in deferred.
    std::vector<int> deferred = tour3;

	std::vector<int> parents = tour3;

	// these are the variables we'll use in the for loop below.
    int p0 = tour[0];
	int p1 = p0;
    int p2;
	cost_type d;

	if(verbose)
		std::cout << "atsp: Reducing tour, deferring nodes with collisions..." << std::endl;

	// This for loop takes a with convertTSPtoATSPMatrix() enlarged matrix 'w' 
	// with its corresponding best known solution 'tour' and attempts to 
	// reconstruct a tour that corresponds to the original matrix that was
	// used to create 'w'.
    for( size_t i = 1; i < tour.size(); i++ )
	{
	    p2 = tour[i];
		p1 = reduceTour(w, tour, tour3, parents, visited, deferred, n, p1, p2, debug );
	}

	p2 = p0;
	d = w[p1*n*2+p2];
	if(debug)
		std::cout << p1 << " -> " << p2 << " = " << d << std::endl;

	// reduce tour for the connecting edge
	if( d > 0 && d != COST_TYPE_MAX )
	{
		p1 = reduceTour(w, tour, tour3, parents, visited, deferred, n, p1, p2, debug );
	}

	// print out tour3 for debugging purposes
	dumpTour( tour3, n, debug );

	if(verbose && !deferred.empty())
		std::cout << "atsp: Connecting deferred nodes..." << std::endl;

	std::vector<int> deferredLeft;

	// place the deferred nodes.
	for( size_t i = 0; i < deferred.size(); i++ )
	{
		p2 = deferred[i];
		if( i != p2 && p2 >= 0 )
		{
			if( visited[p2] || tour3[i] != -1 || !connectNodes(tour3, parents, visited, deferred, i, p2, true ) )
			{
				deferredLeft.push_back(i);
			}
		}
	}

	// print out tour3 for debugging purposes
	dumpTour( tour3, n, debug );

	
	// super experimental stuff!
	std::vector<cost_type> floyd;
	std::vector<int> next;

	/*
	bparadie, 2012-10-28: Floyd doesn't help, doesn't make any difference...

	const bool useFloyd = !deferredLeft.empty(); // or false;
	if( useFloyd )
	{
		if(verbose && !deferredLeft.empty())
			std::cout << "atsp: Creating Floyd-Warshall matrix..." << std::endl;
		floyd = w;
		next = std::vector<int> (floyd.size());
		tsp_calculate_floyd_cpu(floyd, next, n);
	}
	*/

	if(verbose && !deferredLeft.empty())
		std::cout << "atsp: Resolving deferred nodes..." << std::endl;

	while(!deferredLeft.empty())
	{
		int def = deferredLeft.back();
		deferredLeft.pop_back();

		if( tour3[def] != -1 )
		{
			deferred[def] = -1;
		}
		else
		{
			resolveDeferred(tour3, parents, visited, deferred, def, debug, w, floyd, next, n*2 );
		}
	}

	// print out tour3 for debugging purposes
	dumpTour( tour3, n, debug );

	// collect all unassigned nodes in strangeLoops
	std::vector<int> strangeLoops;
	for(size_t i = 0; i < tour3.size(); i++ )
	{
		if( tour3[i] == -1 )
		{
			strangeLoops.push_back(i);
		}
	}

	if(verbose && !strangeLoops.empty() )
		std::cout << "atsp: Resolving strange loops..." << std::endl;

	// force strangeLoops nodes into the tour.
	// (This might result in tours that are not optimal)
	while(!strangeLoops.empty())
	{
		int def = strangeLoops.back();
		strangeLoops.pop_back();

		const bool isLastLoop = strangeLoops.empty();
		const bool resolved = resolveStrangeLoops( tour3, parents, visited, deferred, def, isLastLoop, debug );

		// print out tour3 for debugging purposes
		dumpTour( tour3, n, debug );

		if( !resolved )
		{
			if(verbose || debug)
				std::cout << "atsp: ERROR - strange loops could not be resolved." << std::endl;
			return false;
		}
	}

	// print out tour3 for debugging purposes
	dumpTour( tour3, n, debug );

	// unwind tour3 and into a "real" tour that conforms to TSPLIB.
	p1 = 0;
	std::vector<int> out(n);
	for( int i = 0; i < n; i++ )
	{
		assert( p1 >= 0 && p1 < n );

		// more robust
		if( p1 >= 0 && p1 < n )
		{
			p2 = tour3[p1];	
			if( collected[p1] )
			{
				if(verbose || debug)
					std::cout << "atsp: ERROR - Incorrect loop at [" << i << "]: " << p2 << std::endl;
				return false;
			}

			collected[p1] = true;
			if(debug)
				std::cout << "[" << i << "]: " << p2 << std::endl;
			out[i] = p2;
			p1 = p2;
		}
		else 
		{
			return false;
		}
	}

	// swap it out.
	outTour.swap(out);

	if( !validateTour( outTour, n, false ) )
	{
		if(verbose || debug)
			std::cout << "atsp: ERROR - invalid tour." << std::endl;
		return false;
	}

	if(verbose)
		std::cout << "atsp: Reconstrucing ATSP tour from TSP tour succeeded!" << std::endl;

	return true;
}
