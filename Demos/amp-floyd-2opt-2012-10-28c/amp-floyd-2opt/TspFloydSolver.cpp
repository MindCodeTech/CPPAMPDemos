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

#include "TspFloydSolver.hpp"
#include "TspFindMinimum.hpp"
#include "TspNearestNeighbor.hpp"
#include "problem.hpp"

#include <assert.h>             
#include <iostream>
#include <algorithm>
#include <functional>
#include <assert.h>

// C++ AMP header file
#include <amp.h>                
#include <amp_math.h>

//----------------------------------------------------------------------------
// Namespaces
//----------------------------------------------------------------------------

using namespace concurrency;
using std::vector;
using namespace opt;
using namespace utl;
using namespace flw;
using namespace pbl;
using namespace fmn;
using namespace nnb;


//----------------------------------------------------------------------------
// Based on: Transitive Closure Sample in C++ AMP
// http://blogs.msdn.com/b/nativeconcurrency/archive/2011/11/08/transitive-closure-sample-in-c-amp.aspx
// 
// File: TransitiveClosure.cpp
//
// Contains the implementation of algorithms which explores connectivity between 
// nodes in a graph and determine shortest path.
// This is based on paper http://www.seas.upenn.edu/~kiderj/research/papers/APSP-gh08-fin-T.pdf
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Based on: Transitive Closure Sample in C++ AMP
// http://blogs.msdn.com/b/nativeconcurrency/archive/2011/11/08/transitive-closure-sample-in-c-amp.aspx
// 
// Stage1 - determine connectivity between vertexs' within a TILE - primary
//----------------------------------------------------------------------------

static void floyd_stage1_kernel(tiled_index<TILE_SIZE, TILE_SIZE> tidx, array<cost_type, 2> &graph, int passnum,
						 array<int, 2> &next) restrict(amp)
{
    index<2> local_idx = tidx.local;
    
    // Load primary block into shared memory (primary_block_buffer)
    tile_static cost_type primary_block_buffer[TILE_SIZE][TILE_SIZE];
    index<2> idx(passnum * TILE_SIZE + local_idx[0], passnum * TILE_SIZE + local_idx[1]);

    primary_block_buffer[local_idx[0]][local_idx[1]] = graph[idx];

    tidx.barrier.wait();

	int z = 0;
	const int i = local_idx[0];
	const int j = local_idx[1];
    const cost_type ij = primary_block_buffer[i][j];

    // Now perform the actual Floyd-Warshall algorithm on this block
    for (unsigned int k = 0; k < TILE_SIZE; ++k)
    {
		if( i != j && k != i && k != j )
		{
			const cost_type ik = primary_block_buffer[i][k];
			const cost_type kj = primary_block_buffer[k][j];
			const cost_type ikkj = ik + kj;
       		
			if( ikkj > 0 && ikkj < ij )
			{
				primary_block_buffer[i][j] = ikkj;
				// @@@
				// if( next[idx] == NO_PATH )
					next[idx] = k;
			}
		}
        tidx.barrier.wait();
    }

	graph[idx] = primary_block_buffer[local_idx[0]][local_idx[1]];
}

//----------------------------------------------------------------------------
// Based on: Transitive Closure Sample in C++ AMP
// http://blogs.msdn.com/b/nativeconcurrency/archive/2011/11/08/transitive-closure-sample-in-c-amp.aspx
// 
// Stage2 - determine connectivity between vertexs' between 2 TILE - primary 
// and current - current is along row or column of primary
//----------------------------------------------------------------------------

static void floyd_stage2_kernel(tiled_index<TILE_SIZE, TILE_SIZE> tidx, array<cost_type, 2> &graph, int passnum,
						 array<int, 2> &next) restrict(amp)
{
    index<2> tile_idx = tidx.tile;
    index<2> local_idx = tidx.local;
    
    // Load primary block into shared memory (primary_block_buffer)
    tile_static cost_type primary_block_buffer[TILE_SIZE][TILE_SIZE];
    index<2> idx(passnum * TILE_SIZE + local_idx[0], passnum * TILE_SIZE + local_idx[1]);
    primary_block_buffer[local_idx[0]][local_idx[1]] = graph[idx];

    // Load the current block into shared memory (curr_block_buffer)
    tile_static cost_type curr_block_buffer[TILE_SIZE][TILE_SIZE];
    unsigned int group_id0, group_id1;
    if (tile_idx[0] == 0)
    {
        group_id0 = passnum;
        if (tile_idx[1] < passnum)
        {
            group_id1 = tile_idx[1];
        }
        else
        {
            group_id1 = tile_idx[1] + 1;
        }
    }
    else
    {
        group_id1 = passnum;
        if (tile_idx[1] < passnum)
        {
            group_id0 = tile_idx[1];
        }
        else
        {
            group_id0 = tile_idx[1] + 1;
        }
    }

    idx[0] = group_id0 * TILE_SIZE + local_idx[0];
    idx[1] = group_id1 * TILE_SIZE + local_idx[1];
    curr_block_buffer[local_idx[0]][local_idx[1]] = graph[idx];

    tidx.barrier.wait();

    // Now perform the actual Floyd-Warshall algorithm on this block

	/*
		From http://www.seas.upenn.edu/~kiderj/research/papers/APSP-gh08-fin-T.pdf

		"This can be shown by noticing that for
		current blocks that share the row of the primary block, k
		ranges from pstart to pend, j ranges from pstart to pend, and i
		ranges from cstart to cend, where cstart and cend are the start
		and end indices for the current block in the x direction."

		In other words:
		ij = [cstart →cend, pstart → pend]
		ik = [cstart →cend, pstart → pend]
		kj = [pstart → pend, pstart → pend] 	
	*/

	const int i = local_idx[0];
	const int j = local_idx[1];
	const cost_type ij = curr_block_buffer[i][j];

	for (unsigned int k = 0; k < TILE_SIZE; ++k)
	{
		if( i != j && k != i && k != j )
		{
			if (tile_idx[0] == 0)
			{
				const cost_type ikkj = primary_block_buffer[i][k] + curr_block_buffer[k][j];             
				if( ikkj > 0 && ikkj < ij )
				{
					curr_block_buffer[i][j] = ikkj;
					// @@@ if( next[idx] == NO_PATH )
						next[idx] = group_id0 * TILE_SIZE + k;
				}
			}
			else
			{
				const cost_type ikkj = curr_block_buffer[i][k] + primary_block_buffer[k][j];           
				if( ikkj > 0 && ikkj < ij )
				{
					curr_block_buffer[i][j] = ikkj;
					// @@@ if( next[idx] == NO_PATH )
						next[idx] = group_id1 * TILE_SIZE + k;
				}
			}
		}
		tidx.barrier.wait();
	}

    graph[idx] = curr_block_buffer[i][j];
}

//----------------------------------------------------------------------------
// Based on: Transitive Closure Sample in C++ AMP
// http://blogs.msdn.com/b/nativeconcurrency/archive/2011/11/08/transitive-closure-sample-in-c-amp.aspx
// 
// Stage3 - determine connectivity between vertexs' between 3 TILE 
// 1. primary block, 2. block made of row af current and column of primary 
// 3. block made of column of current and row of primary
//----------------------------------------------------------------------------

static void floyd_stage3_kernel(tiled_index<TILE_SIZE, TILE_SIZE> tidx, array<cost_type, 2> &graph, int passnum,
						 array<int, 2> &next) restrict(amp)
{
    index<2> tile_idx = tidx.tile;
    index<2> local_idx = tidx.local;
    
    unsigned int group_id0, group_id1;
    if (tile_idx[0] < passnum)
    {
        group_id0 = tile_idx[0];
    }
    else
    {
        group_id0 = tile_idx[0] + 1;
    }

    if (tile_idx[1] < passnum)
    {
        group_id1 = tile_idx[1];
    }
    else
    {
        group_id1 = tile_idx[1] + 1;
    }

    // Load block with same row as current block and same column as primary block into shared memory (shBuffer1)
    tile_static cost_type shbuffer1[TILE_SIZE][TILE_SIZE];
    index<2> idx(group_id0 * TILE_SIZE + local_idx[0], passnum * TILE_SIZE + local_idx[1]);
    shbuffer1[local_idx[0]][local_idx[1]] = graph[idx];

    // Load block with same column as current block and same row as primary block into shared memory (shBuffer2)
    tile_static cost_type shBuffer2[TILE_SIZE][TILE_SIZE];

    idx[0] = passnum * TILE_SIZE + local_idx[0];
    idx[1] = group_id1 * TILE_SIZE + local_idx[1];
    shBuffer2[local_idx[0]][local_idx[1]] = graph[idx];

    //  Load the current block into shared memory (shbuffer3)
    tile_static cost_type curr_block_buffer[TILE_SIZE][TILE_SIZE];
    idx[0] = group_id0 * TILE_SIZE + local_idx[0];
    idx[1] = group_id1 * TILE_SIZE + local_idx[1];
    curr_block_buffer[local_idx[0]][local_idx[1]] = graph[idx];

    tidx.barrier.wait();

	const int i = local_idx[0];
	const int j = local_idx[1];

	const cost_type ij = curr_block_buffer[i][j];

	// Now perform the actual Floyd-Warshall algorithm on this block
	for (unsigned int k = 0; k < TILE_SIZE; ++k)
	{
		if( i != j && k != i && k != j )
		{
			const cost_type ikkj = shbuffer1[i][k] + shBuffer2[k][j];         
			if( ikkj > 0 && ikkj < ij )
			{
				curr_block_buffer[i][j] = ikkj;
				// @@@ if( next[idx] == NO_PATH )
					next[idx] = group_id0 * TILE_SIZE + k;
			}
		}

		tidx.barrier.wait();
	}

    graph[idx] = curr_block_buffer[i][j];
}

//----------------------------------------------------------------------------
// Based on: Transitive Closure Sample in C++ AMP
// http://blogs.msdn.com/b/nativeconcurrency/archive/2011/11/08/transitive-closure-sample-in-c-amp.aspx
// 
// C++ AMP (GPU) implementation entry point
//----------------------------------------------------------------------------

static void floyd_amp_tiled(std::vector<cost_type> &graph, int n, std::vector<int> &next)
{
	const int num_vertices = n;
	extent<2> graph_domain(num_vertices, num_vertices);
    array<cost_type, 2> graph_buf(graph_domain, graph.begin());
    array<int, 2> next_buf(graph_domain, next.begin());

    assert((num_vertices % TILE_SIZE) == 0);
    extent<2> stage1_compute(extent<2>(TILE_SIZE, TILE_SIZE));
    assert(((num_vertices-TILE_SIZE) % TILE_SIZE) == 0);
    extent<2> stage2_compute(extent<2>(2*TILE_SIZE, num_vertices - TILE_SIZE));
    extent<2> stage3_compute(extent<2>(num_vertices - TILE_SIZE, num_vertices - TILE_SIZE));

	// use parallel for num_vertices/TILE_SIZE
    for ( int k = 0; k < num_vertices/TILE_SIZE; ++k)
    {
        parallel_for_each(stage1_compute.tile<TILE_SIZE, TILE_SIZE>(), [k, &graph_buf, &next_buf] (tiled_index<TILE_SIZE, TILE_SIZE> ti) restrict(amp) {
            floyd_stage1_kernel(ti, graph_buf, k, next_buf); 
        });
        parallel_for_each(stage2_compute.tile<TILE_SIZE, TILE_SIZE>(), [k, &graph_buf, &next_buf] (tiled_index<TILE_SIZE, TILE_SIZE> ti) restrict(amp) {
            floyd_stage2_kernel(ti, graph_buf, k, next_buf);
        });
        parallel_for_each(stage3_compute.tile<TILE_SIZE, TILE_SIZE>(), [k, &graph_buf, &next_buf] (tiled_index<TILE_SIZE, TILE_SIZE> ti) restrict(amp) {
            floyd_stage3_kernel(ti, graph_buf, k, next_buf);
        });
    }

	graph = graph_buf;
	next = next_buf;
}

//----------------------------------------------------------------------------
// Matrices with dimensions that are not multiples of TILE_SIZE*TILE_SIZE 
// need to be 'padded'. The 'outNext' vector (which keeps the information 
// for path reconstructions) gets initialized accordingly.
//----------------------------------------------------------------------------

static int padMatrix(const std::vector<cost_type>& w, int n, std::vector<cost_type> &outMatrix, std::vector<int> &outNext)
{
	// check whether the matrix needs padding
	if( (n % TILE_SIZE) == 0 )
	{
		// we are good, copy over as is.
		outMatrix = w;

		// initialize outNext for path reconstructions.
		std::vector<int> tmp(n*n);
		std::fill( tmp.begin(), tmp.end(), NO_PATH_INT);
		outNext.swap(tmp);

		// the dimensions have not changed
		return n;
	}

	// The code below does the 'padding'.

	// round up to the next tile
	const int num_vertices = TILE_SIZE * ((n / TILE_SIZE)+1);

	/*
	This version is probably slightly slower.
	But easier to read...

	std::vector<cost_type>tmp(num_vertices*num_vertices);
	for( int i = 0; i < num_vertices; i++ )
		for( int j = 0; j < num_vertices; j++ )
		{
			if( i < n && j < n )
				tmp[i*num_vertices+j] = w[i*n+j];
			else
				tmp[i*num_vertices+j] = (COST_TYPE_MAX);
		}
	*/

	// 'tmp' will become outMatrix.
	std::vector<cost_type>tmp(num_vertices*num_vertices);

	// iterate over the rows of the new 'padded' matrix 
	for( int i = 0; i < num_vertices; i++ )
	{
		int in = i*n;
		auto it = tmp.begin()+i*num_vertices;

		if( i < n )
		{
			// this row is in the zone that is covered by 'w'.
			auto wbegin = w.begin() + in;
			auto wend = wbegin + n;
			std::copy(wbegin,wend,it);

			// pad: fill the tail end columns with COST_TYPE_MAX
			auto begin = it + num_vertices;
			auto end = begin + num_vertices - n;
			std::fill(begin, end, COST_TYPE_MAX); 
		}
		else
		{
			// pad: fill the whole row with COST_TYPE_MAX
			std::fill(it, it+num_vertices, COST_TYPE_MAX); 
		}
	}

	// apply changes to 'outMatrix'
	outMatrix.swap(tmp);
	
	// initialize outNext for path reconstructions.
	std::vector<int>tmp_next(outMatrix.size());
	std::fill(tmp_next.begin(), tmp_next.end(), NO_PATH_INT);

	// apply changes to 'outNext'
	outNext.swap(tmp_next);

	// return new dimension that is a multiple of TILE_SIZE.
	return num_vertices;
}

//----------------------------------------------------------------------------
// A matrix previously padded via padMatrix() needs to be 'unpadded'.
// Returns true if unpadding was performed.
//----------------------------------------------------------------------------

static bool unpadMatrix( std::vector<cost_type>& w, std::vector<int>& next, int n, int padded_n )
{
	// check whether unpadding is even necessary
	if( n == padded_n )
		return false;

	// 'tmp_w' will be saved in 'w'.
	std::vector<cost_type> tmp_w(n*n);

	// 'tmp_w' will be saved in 'w'.
	std::vector<int> tmp_next(next.size());
	std::copy( next.begin(), next.end(), tmp_next.begin() );

	// iterate over all items of 'tmp_w'
	for( int i = 0; i < n; i++ )
	{
		for( int j = 0; j < n; j++ )
		{
			// copy the value over
			tmp_w[i*n+j] = w[i*padded_n+j];

			// in 'w' all vertices are described as indices 
			// in terms of 'w' dimensions.
			// Those references need to be translated to indices 
			// of the smaller n-dimension matrix 'tmp_next'.
			const int padded_k = next[i*padded_n+j];
			if( padded_k != NO_PATH )
			{
				const int padded_i = padded_k / padded_n;
				const int padded_j = padded_k % padded_n;
				if( padded_i < n && padded_j < n )
					tmp_next[i*n+j] = next[padded_i * n + padded_j];
			}
		}
	}

	// apply changes
	w.swap(tmp_w);
	next.swap(tmp_next);

	// yes, we unpadded!
	return true;
}

//----------------------------------------------------------------------------
// Floyd-Warshall core function (serial version).
// http://en.wikipedia.org/wiki/Floyd_Warshall
//----------------------------------------------------------------------------

void flw::tsp_calculate_floyd_cpu(std::vector<cost_type>& w, std::vector<int>& next, int n)
{
	std::generate( next.begin(), next.end(), []() { return NO_PATH_INT; } );

	// Floyd-Warshall with path reconstruction
    for (int k = 0; k < n; k++) 
        for (int i = 0; i < n; i++) 
            for (int j = 0; j < n; j++)
            {
				if( i != j && k != i && k != j )
				{
					const int in = i*n;
					const cost_type ij = w[in+j];
					const cost_type ik = w[in+k];
					const cost_type kj = w[k*n+j];

					assert( ij >= 0 && ik >= 0 && kj >= 0 );

					if( ij != COST_TYPE_MAX && 
						ik != COST_TYPE_MAX && 
						kj != COST_TYPE_MAX )
					{
						const cost_type ikkj = ik + kj;
                
						if( ikkj < ij )
						{
							w[in+j] = ikkj;
							next[in+j] = k;
						}
					}
				}
            }
}

//----------------------------------------------------------------------------
// Validates a weight matrix against a matrix computed by tsp_calculate_cpu().
// Returns true if there are no errors.
//----------------------------------------------------------------------------

static bool tsp_floyd_validate(const std::vector<cost_type>& weightMatrix, 
						const std::vector<cost_type>& w, 
						const std::vector<int>& next, int n)
{
	bool hasErrors = false;

	// caluclate Floyd-Warshall using the serial method.
	std::vector<cost_type> w2(weightMatrix);
	std::vector<int> next2(n*n);
	tsp_calculate_floyd_cpu(w2, next2, n);

	// visit every item in 'w' and 'next'
	// and compare them with those from 'w2' and 'next2'.
	for( int i = 0; i < n; i++ )
	{
		for( int j = 0; j < n; j++ )
		{
			// check whether weights are different
			if( !fp_equal( w[i*n+j], w2[i*n+j], 0.5 ) )
			{
				std::cout << "Error at dist " << i*n+j << ", [" << i << "," << j << "] = " << w[i*n+j] << " != " << w2[i*n+j] << std::endl;
				hasErrors = true;
			}

			/*

			bparadie, 2012-09-10:
			preserving the "next" information in the GPU floyd implementation is iffy.
			The floyd-gpu tours are correct but show different next chains.

			// check whether next vertices are different
			if( next[i*n+j] != next2[i*n+j] )
			{
				// we are only interested in unrecorded branches 
				if( next[i*n+j] == NO_PATH || next2[i*n+j] ==  NO_PATH )
				{
					std::cout << "Error at next " << i*n+j << ", [" << i << "," << j << "] = " << next[i*n+j] << " != " << next2[i*n+j] << std::endl;
					hasErrors = true;
				}
			}
			*/
		}
	}

	return hasErrors;
}

//----------------------------------------------------------------------------
// Calculates Floyd-Warshall using AMP and return the best known solution.
// The tour to the corresponding solution is stored in 'tour'.
//----------------------------------------------------------------------------

cost_type flw::tsp_floyd_gpu( std::vector<int>& tour, 
							const Tsp_container& tspContainer, 
							const TspOptions& options )
{	
	const std::vector<cost_type>& weightMatrix = tspContainer.pbl_adj_mtx;
	const int n = tspContainer.problem_dim;

    std::vector<cost_type> w;
	std::vector<int> next;

	// copy/pad weight matrix
	// and measure the time
	Timer timer;
	timer.Start();
	const int padded_n = padMatrix(weightMatrix,n,w,next);
	// infinitize(w,n);
	floyd_amp_tiled( w, padded_n, next);
	unpadMatrix(w,next,n,padded_n);
	timer.Stop();

	std::cout << "floyd-gpu: " << timer.Elapsed() << "ms." << std::endl;

	if( options.validate )
		tsp_floyd_validate( weightMatrix, w, next, n );

	// find the tour using nearest neighbor
	timer.Start();

	// bparadie, 2012-08-22:
	// It turned out, it is very hard to beat the serial std::min_element.
	// In findMinimum_gpu I tried reduction_cascade() and reduction_simple_1()
	// from the Parallel Reduction using C++ AMP article.
	// I couldn't get reduction_cascade() to work and reduction_simple_1() was
	// about 10x slower than std::min_element().

	// DISABLED: findMinimum_gpu is just too slow.
	// const cost_type len = findNearestNeighbor( w, next, n, tour, options, findMinimum_gpu );
	const cost_type len = findNearestNeighbor( w, next, n, tour, tspContainer, options, findMinimum_cpu );
	timer.Stop();

	if( options.verbose )
		std::cout << "nearest-neighbor: " << timer.Elapsed() << "ms." << std::endl;

	// return best known solution
    return len;
}


//----------------------------------------------------------------------------
// Calculates Floyd-Warshall using the standard, serial method and return the 
// best known solution.
// The tour to the corresponding solution is stored in 'tour'.
//----------------------------------------------------------------------------

cost_type flw::tsp_floyd_cpu( std::vector<int>& tour, 
							const Tsp_container& tspContainer, 
							const TspOptions& options )
{	
	const std::vector<cost_type>& weightMatrix = tspContainer.pbl_adj_mtx;
	const int n = tspContainer.problem_dim;

	Timer timer;
	timer.Start();

	// copy weight matrix
    std::vector<cost_type> w(weightMatrix);
	infinitize(w,n);

	// next keeps k for path reconstruction
	std::vector<int> next(n*n);

	tsp_calculate_floyd_cpu(w,next,n);

	timer.Stop();
	std::cout << "floyd-cpu: " << timer.Elapsed() << "ms." << std::endl;

	timer.Start();

	// TspFindMinimum implements findMinimum_cpu, findMinimum_gpu, and findNext.
	// findMinimum_gpu is about 10x slower than findMinimum_cpu.
	cost_type len = findNearestNeighbor( w, next, n, tour, tspContainer, options, findMinimum_cpu );

	timer.Stop();

	if( options.verbose )
		std::cout << "Find tour took " << timer.Elapsed() << "ms." << std::endl;

	len = getTourLength(weightMatrix, tour.begin(), n);

    return len;
}


//----------------------------------------------------------------------------
// Adds vertices between two edges 'i' and 'j' using matrix 'w' 
// calculated from Floyd-Warshall with path reconstruction.
// The path reconstruction information is provided by 'next'.
// 'addVertices' marks nodes as visited as it recursively scans for 
// vertices of vertices.
//
// For more detail see:
// http://en.wikipedia.org/wiki/Floyd-Warshall#Path_reconstruction
//
// Used by tsp_floyd_gpu.
//----------------------------------------------------------------------------

cost_type flw::addVertices( const std::vector<opt::cost_type>& weightMatrix,
					   std::vector<cost_type>& w, 
					   const std::vector<int>& next, 
					   int n, int i, int j,
					   std::vector<int>& outTour,
					   std::vector<bool>& visited)
{
	// 'i' and 'j' must always be smaller than n.
	assert( i < n );
	assert( j < n );

	// translate [i,j] to one-dimensional 'idx'
	const int idx = i*n+j;

	// return, if there is no path at all
	if( w[idx] == NO_PATH )
		return COST_TYPE_MAX; 

	// vertex can either be NO_PATH, or an index
	// value that points to another node.
	const int intermediate = next[idx];

	// return, if there is an edge from i to j, with no vertices between.
	if( intermediate == NO_PATH || intermediate >= n || visited[intermediate] )
		return 0;

	// mark 'intermediate' as visited.
	markVisited(w,n,intermediate,visited);

	cost_type len = 0;

	// recursively scan from 'i' to 'intermediate'.
	len += addVertices(weightMatrix, w, next, n, i, intermediate, outTour, visited);

	// record the center of this iteration and 
	outTour.push_back(intermediate);
	len += weightMatrix[idx];

	// recursively scan from 'intermediate' to 'j'.
	len += addVertices(weightMatrix, w, next, n, intermediate, j, outTour, visited);

	return len;
}

