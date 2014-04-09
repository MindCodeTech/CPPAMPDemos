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

#include "TspFindMinimum.hpp"

#include <assert.h>             
#include <iostream>
#include <algorithm>
#include <functional>

// C++ AMP header file
#include <amp.h>                
#include <amp_math.h>

#undef min
#undef max
// #include <amp_algorithms.h>
// #include <amp_stl_algorithms.h>
// #include <amp_iterators.h>


//----------------------------------------------------------------------------
// Namespaces
//----------------------------------------------------------------------------

using namespace concurrency;
using std::vector;

using namespace opt;
using namespace utl;
using namespace fmn;

//----------------------------------------------------------------------------
// Finds the next column using these rules:
// 1. Return columns with vertices if there are any.
// 2. Return column with the smallest weight.
// 
// Unfortunately giving columns with vertices a higher priority did not 
// yield better results.
//----------------------------------------------------------------------------

int fmn::findNext( const std::vector<cost_type>& w,
			int start,
			int n,
			std::vector<int>& next,
			const std::vector<bool>& visited)
{
	// initialize local vars with maxima.
	int min = INT_MAX;;
	int min_next = INT_MAX;
	cost_type min_val = COST_TYPE_MAX;
	cost_type min_next_val = COST_TYPE_MAX;

	// iterate over weight matrix 'w'
	for( int i = 0; i < n; i++ )
	{
		const int idx = start + i;
		const int nxt = next[idx];

		// check for unvisited columns that have vertices
		if( nxt != NO_PATH && !visited[nxt] && min_next_val > w[nxt] )
		{
			min_next_val = w[idx];
			min_next = nxt;
		}

		// check for unvisited columns with the lowest weight.
		if( !visited[i] && min_val > w[idx] )
		{
			min_val = w[idx];
			min = i;
		}
	}

	// we have a column with vertices.
	if( min_next != INT_MAX )
		return min_next;

	// we have a column with smallest weight.
	if( min != INT_MAX )
		return min;

	// the whole row is marked as visited.
	return NO_PATH_INT;
}


//----------------------------------------------------------------------------
// Find the minimum using std::min_element.
// Returns column.
//
// It turned out, it is very hard to beat std::min_element.
// In findMinimum_gpu I tried reduction_cascade() and reduction_simple_1()
// from the Parallel Reduction using C++ AMP article.
// I couldn't get reduction_cascade() to work and reduction_simple_1() was
// about 10x slower than std::min_element().
//----------------------------------------------------------------------------

int fmn::findMinimum_cpu( const std::vector<cost_type>& w,
			int start,
			int n,
			std::vector<int>& next,
			const std::vector<bool>& visited)
{
	// using std::min_element, because we want to return the column.
	std::vector<cost_type>::const_iterator begin = w.begin() + start;
	std::vector<cost_type>::const_iterator end = begin + n;
	std::vector<cost_type>::const_iterator found = std::min_element(begin,end);

	// If the minimum is COST_TYPE_MAX that means that 
	// the whole row has been visited.
	if( *found == COST_TYPE_MAX )
		return -1;

	// return the column with the smallest weight.
	return static_cast<int>(found - begin);
}


//----------------------------------------------------------------------------
// Source: Parallel Reduction using C++ AMP
// http://blogs.msdn.com/b/nativeconcurrency/archive/2012/03/08/parallel-reduction-using-c-amp.aspx
//
// Helper macro to tell whether the argument is a positive integer
// power of two.
//----------------------------------------------------------------------------
#define IS_POWER_OF_2(arg) (arg > 1 && (arg & (arg - 1)) == 0)

//----------------------------------------------------------------------------
// Based on: Parallel Reduction using C++ AMP
// http://blogs.msdn.com/b/nativeconcurrency/archive/2012/03/08/parallel-reduction-using-c-amp.aspx
// See reduction_cascade() from the Reduction sample
//
// Here we take completely different approach by using algorithm cascading
// by combining sequential and parallel reduction.
//----------------------------------------------------------------------------

template <unsigned _tile_size>
std::vector<cost_type>::const_iterator reduction_cascade( std::vector<cost_type>::const_iterator& begin,
					std::vector<cost_type>::const_iterator& end, size_t tileCount )
{
	// bparadie, 2012-08-22:
	// The example from "Parallel Reduction using C++ AMP" discusses reduction
	// in the context of std::accumulation, which I felt could easily be applied
	// to std::min. Since I need the position of the minimum I extended the example
	// so "winning" indices would be tracked in a separate array (see a_idx).
	// This method works well in my reduction_simple_1() below and also in this 
	// reduction_cascade().
	//
	// Unfortunately I couldn't get this one to work. In many cases reduction_cascade() is
	// not returning the correct minimum. But recording index positions in a_idx
	// does work. The indices always matches the claimed minima.

	const int _tile_count = static_cast<int>(tileCount);
    assert(end-begin <= UINT_MAX);
    unsigned element_count = static_cast<unsigned>(end-begin);
    assert(element_count != 0); // Cannot reduce an empty sequence.

    static_assert(IS_POWER_OF_2(_tile_size), "Tile size must be a positive integer power of two!");
    assert(_tile_count > 0); // "Tile count must be positive!"

    unsigned stride = static_cast<int>(_tile_size * _tile_count * 2);

    // Reduce tail elements.
	std::vector<cost_type>::const_iterator tail_min = end;
    unsigned tail_length = element_count % stride;
    if(tail_length != 0)
    {
		// This should never happen, because we pre-pad the matrix.
		std::cout << "Warning: tail length is greater than 0." << std::endl;

		tail_min = std::min_element(end - tail_length, end);
        element_count -= tail_length;
        if(element_count == 0)
        {
            return tail_min;
        }
    }

	// Using arrays as a temporary memory.
    array<float, 1> a(element_count, begin);
    array<float, 1> a_partial_result(_tile_count);

	std::vector<int>a_idx_raw(element_count);
	for( unsigned i = 0; i < element_count; i++ )
		a_idx_raw[i] = i;
    array<int, 1> a_idx(element_count,a_idx_raw.begin());

    parallel_for_each(extent<1>(_tile_count * _tile_size).tile<_tile_size>(), 
		[=, &a, &a_partial_result,&a_idx] (tiled_index<_tile_size> tidx) restrict(amp)
    {
        // Use tile_static as a scratchpad memory.
        tile_static float tile_data[_tile_size];
        tile_static int tile_idx[_tile_size];
        unsigned local_idx = tidx.local[0];

        // Reduce data strides of twice the tile size into tile_static memory.
        unsigned input_idx = (tidx.tile[0] * 2 * _tile_size) + local_idx;
        tile_data[local_idx] = 0;
        do
        {
			// Or:
			// tile_data[local_idx] = min(a[input_idx], a[input_idx + _tile_size]); 
			if( a[input_idx] <= a[input_idx + _tile_size] )
			{
				tile_data[local_idx] = a[input_idx];
				tile_idx[local_idx] = a_idx[input_idx];
			}
			else
			{
				tile_data[local_idx] = a[input_idx + _tile_size];
				tile_idx[local_idx] = a_idx[input_idx + _tile_size];
			}

            input_idx += stride;
        } while (input_idx < element_count);

        tidx.barrier.wait();

        // Reduce to the tile result using multiple threads.
        for (unsigned stride = _tile_size / 2; stride > 0; stride /= 2)
        {
            if (local_idx < stride)
            {
				// Or:
				// tile_data[local_idx] = min(tile_data[local_idx], tile_data[local_idx + stride]);
				if( tile_data[local_idx] > tile_data[local_idx + stride] )
				{
					tile_data[local_idx] = tile_data[local_idx + stride];
					tile_idx[local_idx] = tile_idx[local_idx + stride];
				}

            }
            tidx.barrier.wait();
        }

        // Store the tile result in the global memory.
        if (local_idx == 0)
        {
            a_partial_result[tidx.tile[0]] = tile_data[0];
            a_idx[tidx.tile[0]] = tile_idx[0];
        }
    });

    // Reduce results from all tiles on the CPU.
    std::vector<cost_type> v_partial_result(_tile_count);
    copy(a_partial_result, v_partial_result.begin());

    std::vector<int> v_idx(element_count);
    copy(a_idx, v_idx.begin());

	// add tail_min if necessary
	if( tail_min != end )
	{
		v_partial_result.push_back( *tail_min );
		v_idx.push_back( static_cast<int>(tail_min - begin) );
	}

	// find minimum in the result set
    std::vector<cost_type>::const_iterator it = std::min_element(v_partial_result.begin(), v_partial_result.end());

	const int foundResultIdx = static_cast<int>(it-v_partial_result.begin());
	const int foundIdx = v_idx[foundResultIdx];

	return begin + foundIdx;
}

//----------------------------------------------------------------------------
// Based on: Parallel Reduction using C++ AMP
// http://blogs.msdn.com/b/nativeconcurrency/archive/2012/03/08/parallel-reduction-using-c-amp.aspx
// See reduction_simple_1() from the Reduction sample
//
// This is an implementation of the reduction algorithm using a simple
// parallel_for_each. Multiple kernel launches are required to synchronize
// memory access among threads in separate tiles.
//----------------------------------------------------------------------------

std::vector<cost_type>::const_iterator reduction_simple_1(
					std::vector<cost_type>::const_iterator& begin,
					std::vector<cost_type>::const_iterator& end)
{
	// bparadie, 2012-08-22:
	// The example from "Parallel Reduction using C++ AMP" discusses reduction
	// in the context of std::accumulation, which I felt could easily be applied
	// to std::min. Since I need the position of the minimum I extended the example
	// so "winning" indices would be tracked in a separate array.
	// This method works well in both implementations reduction_simple_1() and 
	// reduction_cascade().
	//
	// This version seems to work fine. But the performance is disappointing when
	// compared with std::min_element.

	assert(end-begin <= UINT_MAX);
    unsigned element_count = static_cast<unsigned>(end-begin);
    assert(element_count != 0); // Cannot reduce an empty sequence.
    if (element_count == 1)
    {
        return begin;
    }

    // Using array, as we mostly need just temporary memory to store
    // the algorithm state between iterations and in the end we have to copy
    // back only the first element.
    array<float, 1> a(element_count, begin);

    // Takes care of odd input elements – we could completely avoid tail sum
    // if we would require source to have even number of elements.
    float tail_min = (element_count % 2) ? *(end-1) : COST_TYPE_MAX;
    array_view<float, 1> av_tail_min(1, &tail_min);

    int tail_winner = element_count;
    array<int, 1> av_tail_winner(1, &tail_winner);

	std::vector<int> winners( element_count );
	for( unsigned i = 0; i < element_count; i++ )
		winners[i] = i;
    array<int, 1> av_winners(element_count, winners.begin());

    // Each thread reduces two elements.
	int round = 0;
    for( unsigned s = element_count / 2; s > 0; s /= 2, round++ )
    {
        parallel_for_each(extent<1>(s), [=, &a, &av_winners, &av_tail_winner] (index<1> idx) restrict(amp)
        {
			if( a[idx] > a[idx + s] )
			{
				a[idx] = a[idx + s];
				av_winners[idx] = av_winners[idx + s];
			}

			// Reduce the tail in cases where the number of elements is odd.
            if ((idx[0] == s - 1) && (s & 0x1) && (s != 1))
            {
				if( av_tail_min[0] > a[s - 1] )
				{
					av_tail_min[0] = a[s - 1];
					av_tail_winner[0] = av_winners[s - 1];
				}
            }
        });
    }

    // Copy the results back to CPU.
    std::vector<float> result(1);
    copy(a.section(0, 1), result.begin());
    av_tail_min.synchronize();
 
	std::vector<int> result_winners(1);
    copy(av_winners.section(0, 1), result_winners.begin());
    std::vector<int> result_tail_winner(1);
    copy(av_tail_winner.section(0, 1), result_tail_winner.begin());
   
	float foundMin;
	int foundIdx;
	if( result[0] <= tail_min )
	{
		foundMin = result[0];
		foundIdx = result_winners[0];
	}
	else
	{
		foundMin = tail_min;
		foundIdx = result_tail_winner[0];
	}

	assert( *(begin + foundIdx) == foundMin );
	assert( std::min_element(begin,end) == (begin + foundIdx) );

	/*
	Some sanity checks:

	if( *(begin + foundIdx) != foundMin )
	{
		std::cerr << "Error!" << std::endl;
		std::cerr << "foundMin = " << foundMin << std::endl;
		std::cerr << "foundIdx" << foundIdx << std::endl;
	}

	if( std::min_element(begin,end) != (begin + foundIdx) )
	{
		std::cerr << "Error!" << std::endl;
		std::cerr << "foundMin = " << foundMin << " vs " << *(std::min_element(begin,end)) << std::endl;
		std::cerr << "foundIdx" << foundIdx << " vs " << (std::min_element(begin,end) - begin) << std::endl;
	}
	*/
    return begin + foundIdx;
}

//----------------------------------------------------------------------------
// Based on: Parallel Reduction using C++ AMP
// http://blogs.msdn.com/b/nativeconcurrency/archive/2012/03/08/parallel-reduction-using-c-amp.aspx
//----------------------------------------------------------------------------

template <unsigned _tile_size>
std::vector<cost_type>::const_iterator findMinimumCascade( std::vector<cost_type>::const_iterator& begin,
					std::vector<cost_type>::const_iterator& end, size_t _tile_count )
{
	// bparadie, 2012-08-22:
	// findMinimumCascade() is just a wrapper that makes sure the row gets "padded"
	// before calling reduction_cascade (see tail_length > 0).

	assert(end-begin <= UINT_MAX);
    unsigned element_count = static_cast<unsigned>(end-begin);
    assert(element_count != 0); // Cannot reduce an empty sequence.

    static_assert(IS_POWER_OF_2(_tile_size), "Tile size must be a positive integer power of two!");
    assert(_tile_count > 0); // "Tile count must be positive!"

    const unsigned stride = static_cast<int>(_tile_size * _tile_count * 2);

	// pad tail ends
    unsigned tail_length = element_count % stride;
    if( tail_length > 0 )
    {
		std::vector<cost_type> tmp( begin, end );
		const size_t oldSize = tmp.size();
		tmp.reserve( stride * ((element_count / stride)+1) );
		for( size_t i = tmp.size(); i < tmp.capacity(); i++ )
			tmp.push_back( COST_TYPE_MAX );
		auto it = reduction_cascade<_tile_size>( tmp.begin(), tmp.end(), _tile_count );
		return begin + (it - tmp.begin());
    }

	return reduction_cascade<_tile_size>( begin, end, _tile_count );
}
//----------------------------------------------------------------------------
// Finds the minimum in a row using AMP. 
//----------------------------------------------------------------------------

int fmn::findMinimum_gpu( 
			const std::vector<cost_type>& w,
			int start,
			int n,
			std::vector<int>& next,
			const std::vector<bool>& visited)
{
	typedef std::vector<cost_type>::const_iterator ConstRandomAccessIterator;

	// using std::min_element, because we want to return the column.
	std::vector<cost_type>::const_iterator begin = w.begin() + start;
	std::vector<cost_type>::const_iterator end = begin + n;
	std::vector<cost_type>::const_iterator found;

	std::vector<cost_type> row(n);
	std::copy(begin, end, row.begin() );
	array_view<cost_type,1>av( n,  row );

	// bparadie, 2012-08-22:
	// I experimented with two parallel versions
	// from the Parallel Reduction using C++ AMP article:
	// reduction_simple_1() and reduction_cascade().
	//
	// But it turned out, it is very hard to beat the serial std::min_element.
	// I couldn't get reduction_cascade() to work and reduction_simple_1() was
	// about 10x slower than std::min_element().

	/*
		Using amp_stl_algorithms didn't improve performance either:

		template<typename ConstRandomAccessIterator, typename T, typename BinaryOperation>
		T reduce( ConstRandomAccessIterator first, ConstRandomAccessIterator last, T init, BinaryOperation op )

		Timer t;
		t.Start();
		cost_type x = amp_stl_algorithms::reduce(
					amp_stl_algorithms::begin(av),
					amp_stl_algorithms::end(av), 
					COST_TYPE_MAX, amp_algorithms::min<cost_type>() );
		t.Stop();
		std::cout << "Elapsed: " << t.Elapsed() << std::endl;

		t.Start();
		x = *(std::min_element( begin, end ));
		t.Stop();
		std::cout << "Elapsed: " << t.Elapsed() << std::endl;

	*/

	const bool use_reduction_simple_1 = true;
	if( use_reduction_simple_1 )
	{
		found = reduction_simple_1(begin,end);
	}
	else
	{
		const int tileCount = n / (TILE_SIZE*2);
		const int serial = n % (TILE_SIZE * tileCount * 2);
		const int parallel = n - serial;
		std::vector<cost_type>::const_iterator found = findMinimumCascade<TILE_SIZE>(begin, end, tileCount);
	}

	assert( found == std::min_element(begin,end) );

	// If the minimum is COST_TYPE_MAX that means that 
	// the whole row has been visited.
	if( *found == COST_TYPE_MAX )
		return -1;

	// return the column with the smallest weight.
	return static_cast<int>(found - begin);
}
