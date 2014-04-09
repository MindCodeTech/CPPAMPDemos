/***********************************************************************************************************
 * Copyright (c) 2012 Veikko Eeva <veikko@... see LinkedIn etc.>										   *
 *																										   *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software		   *
 * and associated documentation files (the "Software"), to deal in the Software without restriction,	   *
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,			   *
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is		   *
 * furnished to do so, subject to the following conditions:												   *
 * 																										   *
 * The above copyright notice and this permission notice shall be included in all copies or				   *
 * substantial portions of the Software.																   *
 *																										   *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT	   *
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, *
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE	   *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.												   *
 *																										   *
 ***********************************************************************************************************/

/***********************************************************************************************************
 * This code was written to participate to a competition organised by Beyond3D forums and supported by     *
 * AMD and Microsoft. See more at http://www.beyond3d.com/content/articles/121/. My sincere thanks         *
 * for the inducement to learn many new things.                                                            *
 *                                                                                                         * 
 ***********************************************************************************************************/

/***********************************************************************************************************
 * The (a)symmetric GPU 2-opt solver is based on Kamil Rocki's and Reiji Suda's CUDA TSP solver available  *
 * at http://olab.is.s.u-tokyo.ac.jp/~kamil.rocki/projects.html. This version is (semi-)ported with an	   *
 * explicit permission to produce a C++ AMP version under MIT license.                                     *
 *																										   *
 * Note, however, this version is strictly weaker, e.g. no tile_static for the time being, and uses a      *
 * different indexing scheme. Please, see the original paper, code and poster for more information         *
 * regarding performance characteristics and theory framework. As an added note:                           *
 * http://gpuscience.com/articles/logo-gpu-accelerated-travelling-salesman-problem-tsp-solver/             *
 * 																										   *
 * There's also a C++ AMP port available on 2D coordinates as described in the poster, that is included in *
 * \headers\raw_port\raw_port.h, that shows the main parts. Do note that code requires refactoring and     * 
 * clean-up and doesn't work as provided. It is included to show how the original might look			   *
 * like in C++ AMP.                   											                           *
 **********************************************************************************************************/

#pragma once

#include "iterator_overloads.h"
#include "opt2_move.h"
#include "utilities.h"

#include <amp.h>
#include <amp_math.h>

#include <array>
#include <limits>
#include <vector>


namespace kernel
{
	//TODO: This applies to all of the *-opt functions, the indices could be generated lazily with a special
	//generator and not in loops. In effect, somwhat like std::next_permutation and std::prev_permutation.

	template<typename T>
	opt2_move opt2kernel_asymmetric(std::vector<T> const& cost_matrix, concurrency::array<int, 1> const& path)
	{
		//A likely fast looking approach (on CPU) would likely be http://eprints.pascal-network.org/archive/00001369/01/025_brest.pdf.
		//Then there are other alternatives like starting a Dijikstra from every node on GPU and such.

		//Note that unlike in symmetric case, it might be beneficial to reverse a route even if
		//the change in length isn't negative in both directions. Hence using a "big value" instead
		//of zero. Note that this solution uses the approach that is commented out in the GPU version.
		const T current_tour_cost = solution_cost<T>(cost_matrix, path);
		auto temp_path = path;
		T best_reverse_cost = current_tour_cost;
		const decltype(path.extent.size()) dimension = path.extent.size();
		opt2_move best_move = opt2_move(0, 0, (std::numeric_limits<T>::max)());
		
		for(decltype(path.extent.size()) i = 1; i < dimension - 2; ++i)
		{			
			for(decltype(path.extent.size()) j = i + 1; j < dimension - 1; ++j)
			{
				const int a = path[i];
				const int b = path[j + 1];
				const int c = path[i - 1];
				const int d = path[j];
								
				const auto index1 = utilities::to_row_major_index(b, a, dimension);
				const auto index2 = utilities::to_row_major_index(d, c, dimension);
				const auto index3 = utilities::to_row_major_index(c, a, dimension);
				const auto index4 = utilities::to_row_major_index(d, b, dimension);

				const T change_cost = cost_matrix[index1] + cost_matrix[index2] - cost_matrix[index3] - cost_matrix[index4];			
				if(change_cost < best_move.minchange_)
				{
					int x1 = i;
					int x2 = j + 1;
					if(x1 > x2)
					{	
						std::swap(x1, x2);
					}
										
					//TODO: Note that reversing a path segment isn't probably necessary just to calculate the cost effect.
					//An example: using solution_cost_reversed has the same effect as first calling 
					//std::reverse(temp_path.begin(), temp_path.end()); and then calling solution_cost, see solution_cost_reversed
					//for more notes. Regarding this "flip", swapping indices in index calculation, e.g. changing
					//utilities::to_row_major_index(b, a, dimension) to utilities::to_row_major_index(a, b, dimension); or
					//changing it to utilities::to_column_major_index(b, a, dimension) has the same effect.
					//
					//These observations with the fact that having the knowledge of current path length it would be enough
					//to calculate just the cost of the reversed segment. This would remove the need to calculate through
					//the whole path every time a reversed cost is evaluated. This in turn, would allow for parallelizing
					//the route calculation when there are multiple paths to be checked (e.g. in case what the GPU produces) without
					//severe cache thrashing which results from random access to the cost matrix. On the other hand, it isn't
					//the handiest data structure for the job in the first place.
					std::reverse(std::begin(temp_path) + x1, std::begin(temp_path) + x2);
					auto reversed_cost = solution_cost<T>(cost_matrix, temp_path);
					if(best_reverse_cost > reversed_cost)
					{
						best_reverse_cost = reversed_cost;
						best_move = opt2_move(i, j + 1, reversed_cost - current_tour_cost);
					}

					//This temporary path will be reused, so it should be reversed back.
					std::reverse(std::begin(temp_path) + x1, std::begin(temp_path) + x2);
				}
			}
		}

		return best_move;
	}


	template<typename T>
	opt2_move opt2kernel_symmetric(std::vector<T> const& cost_matrix, concurrency::array<int, 1> const& path)
	{
		const decltype(path.extent.size()) dimension = path.extent.size();
		opt2_move best_move = opt2_move(0, 0, 0);
		
		//Due to symmetry not all combinations need to be checked. For a detailed illustration, see
		//Kamil Rocki's excellent presentation at http://olab.is.s.u-tokyo.ac.jp/~kamil.rocki/projects.html.
		for(decltype(path.extent.size()) i = 1; i < dimension - 2; ++i)
		{				
			for(decltype(path.extent.size()) j = i + 1; j < dimension - 1; ++j)
			{
				const int a = path[i];
				const int b = path[j + 1];
				const int c = path[i - 1];
				const int d = path[j];
								
				const auto index1 = utilities::to_row_major_index(b, a, dimension);
				const auto index2 = utilities::to_row_major_index(d, c, dimension);
				const auto index3 = utilities::to_row_major_index(c, a, dimension);
				const auto index4 = utilities::to_row_major_index(d, b, dimension);
								
				const T change_cost = cost_matrix[index1] + cost_matrix[index2] - cost_matrix[index3] - cost_matrix[index4];	
				if(change_cost < best_move.minchange_)
				{
					best_move = opt2_move(i, j + 1, change_cost);
				}
			}
		}
				
		return best_move;
	}

		
	inline void min_change(opt2_move& a, opt2_move const& b) restrict(amp)
	{
		a = (a.minchange_ < b.minchange_) ? a : b;
	}
	
	
	template<typename T, size_t tile_size, size_t tiles>
	void opt2_simple_kernel_symmetric(concurrency::array_view<const T, 1> const& cost_matrix_view, concurrency::array<int, 1> const& gpu_path, concurrency::array_view<opt2_move, 1> candinate_2opt_moves, concurrency::accelerator const& accelerator)
	{
		const unsigned int THREADS = tile_size * tiles;
		const unsigned int dimension = static_cast<unsigned int>(gpu_path.extent.size());
		const unsigned int number_of_2opt_exchanges = static_cast<unsigned int>(dimension - 3) * static_cast<unsigned int>(dimension - 2) / 2;
		const unsigned int iterations = (number_of_2opt_exchanges / (THREADS * tiles)) + 1;
		
		auto tsp_domain = concurrency::extent<1>(THREADS).tile<tiles>();
		parallel_for_each(accelerator.default_view, tsp_domain, [=, &gpu_path](concurrency::tiled_index<tiles> t_idx) restrict(amp)
		{				
			register opt2_move best = opt2_move(0, 0, 0);
			best.minchange_ = 0;
			best.i_ = 0;
			best.j_ = 0;
			tile_static opt2_move best_move_candinates[tile_size];
																
			for(register int k = t_idx.local[0]; k < tile_size; k += t_idx.tile_dim0)
			{					
				best_move_candinates[k] = best;
			}

			t_idx.barrier.wait();			

			for(register unsigned int k = 0; k < iterations; ++k)
			{				
				register unsigned int id = t_idx.global[0] + k * THREADS;
				if(id < number_of_2opt_exchanges)
				{
					register unsigned int j = static_cast<unsigned int>(3 + concurrency::fast_math::sqrtf(8.0f * static_cast<float>(id) + 1.0f)) / 2;		
					register unsigned int i = id - (j - 2) * (j - 1) / 2 + 1; 
						
					register const int a = gpu_path[i];
					register const int b = gpu_path[j + 1];
					register const int c = gpu_path[i - 1];
					register const int d = gpu_path[j];
													
					register const auto index1 = utilities::to_row_major_index(b, a, dimension);
					register const auto index2 = utilities::to_row_major_index(d, c, dimension);
					register const auto index3 = utilities::to_row_major_index(c, a, dimension);
					register const auto index4 = utilities::to_row_major_index(d, b, dimension);
					
					//Calculate the cost effect of swap and save for further reduction if better than some previous one.
					register const T change = cost_matrix_view[index1] + cost_matrix_view[index2] - cost_matrix_view[index3] - cost_matrix_view[index4];
					if(change < best.minchange_)
					{
						best.minchange_ = change;
						best.i_ = i;
						best.j_ = j + 1;

						best_move_candinates[t_idx.local[0]] = best;
					}
				}
			}
			t_idx.barrier.wait();
			
			
			//This reduction number 3 at http://blogs.msdn.com/b/nativeconcurrency/archive/2012/03/08/parallel-reduction-using-c-amp.aspx.
			for(register int s = tile_size / 2; s > 0; s /= 2)
			{
				if(t_idx.local[0] < s)
				{					
					min_change(best_move_candinates[t_idx.local[0]], best_move_candinates[t_idx.local[0] + s]);
				}
								
				t_idx.barrier.wait();
			}
			
			//Here the reduction is done, result is saved to global memory and written to the bus.
			if(t_idx.local[0] == 0)
			{					
				candinate_2opt_moves[t_idx.tile[0]] = best_move_candinates[0];
			}
		});
				
		candinate_2opt_moves.synchronize();
	}
		

	/* THIS IS HERE ONLY FOR DOCUMENTATION PURPOSES, SEE COMMENTS.
	template<typename T, size_t tile_size, size_t tiles>
	opt2_move opt2_simple_kernel_asymmetric(std::vector<T> const& cost_matrix, concurrency::array_view<const T, 1> const& cost_matrix_view, concurrency::array<int, 1> const& p, concurrency::accelerator const& accelerator)
	{
		const unsigned int THREADS = tile_size * tiles;
		const unsigned int dimension = static_cast<unsigned int>(p.extent.size());
		const unsigned int number_of_2opt_exchanges = static_cast<unsigned int>(dimension - 3) * static_cast<unsigned int>(dimension - 2) / 2;
		const unsigned int iterations = (number_of_2opt_exchanges / (THREADS * tiles)) + 1;
		
		std::array<opt2_move, tiles> candinate_2opt_moves;
		concurrency::array_view<opt2_move, 1> candinate_2opt_moves_view(candinate_2opt_moves.size(), candinate_2opt_moves);

		auto tsp_domain = concurrency::extent<1>(THREADS).tile<tiles>();
		parallel_for_each(accelerator.default_view, tsp_domain, [=, &p](concurrency::tiled_index<tiles> t_idx) restrict(amp)
		{				
			opt2_move best = opt2_move(0, 0, 0);
			best.minchange_ = 0;
			best.i_ = 0;
			best.j_ = 0;
			tile_static opt2_move best_move_candinates[tile_size];
										
			for(int k = t_idx.local[0]; k < tile_size; k += t_idx.tile_dim0)
			{					
				best_move_candinates[k] = best;
			}
			t_idx.barrier.wait();			

			for(unsigned int k = 0; k < iterations; ++k)
			{				
				unsigned int id = t_idx.global[0] + k * THREADS;
				if(id < number_of_2opt_exchanges)
				{
					unsigned int j = static_cast<unsigned int>(3 + concurrency::fast_math::sqrtf(8.0f * static_cast<float>(id) + 1.0f)) / 2;		
					unsigned int i = id - (j - 2) * (j - 1) / 2 + 1; 
										
					const int a = p[i];
					const int b = p[j + 1];
					const int c = p[i - 1];
					const int d = p[j];
								
					const auto index1 = utilities::to_row_major_index(b, a, dimension);
					const auto index2 = utilities::to_row_major_index(d, c, dimension);
					const auto index3 = utilities::to_row_major_index(c, a, dimension);
					const auto index4 = utilities::to_row_major_index(d, b, dimension);
					
					//Calculate the cost effect of swap and save for further reduction if better than some previous one.
					const T change = cost_matrix_view[index1] + cost_matrix_view[index2] - cost_matrix_view[index3] - cost_matrix_view[index4];
					if(change < best.minchange_)
					{
						best.minchange_ = change;
						best.i_ = i;
						best.j_ = j + 1;

						best_move_candinates[t_idx.local[0]] = best;
					}
				}
			}
			t_idx.barrier.wait();
			
			
			//This reduction number 3 at http://blogs.msdn.com/b/nativeconcurrency/archive/2012/03/08/parallel-reduction-using-c-amp.aspx.
			for(int s = tile_size / 2; s > 0; s /= 2)
			{
				if(t_idx.local[0] < s)
				{					
					min_change(best_move_candinates[t_idx.local[0]], best_move_candinates[t_idx.local[0] + s]);
				}
								
				t_idx.barrier.wait();
			}
			
			//Here the reduction is done, result is saved to global memory and written to the bus.
			if(t_idx.local[0] == 0)
			{					
				candinate_2opt_moves_view[t_idx.tile[0]] = best_move_candinates[0];
			}
		});
		
		candinate_2opt_moves_view.synchronize();
		return prune_asymmetric_candinates(candinate_2opt_moves, cost_matrix, cost_matrix_view, p, accelerator);
		*/
		/*
		 * It looks like most of the time one of the best or near-best candinates is amongst the calculations above.
		 * On the other hand, in some problem instances this will not be the case and the following would be a better
		 * method. The problem with it is that the results start to "spiral" to smaller and smaller from "big number"
		 * to one and it can take quite a while to get there. It's not performant. This behaviour is probably 
		 * related to the "asymmetry degree" (in lack of knowledge for a better term) of the cost matrix. The gist of
		 * the matter is that edges in the tour will be swapped, re-swapped, reversed and re-reversed until there's
		 * no gain in any direction, whilst it could be better to just jump to another route.
		 *
		 * One way to aim for the "best of both worlds", could be to make only a finite number of runs and then force a
		 * perturbation and start the "spiral" towards local/global optima again. One way would be to calculate with
		 * the method above and the one below and "sample" the results so that the results are compared and a best one
		 * will be chosen as long as the above function still returns a feasible route (as it will cut rather
		 * quickly with no spiraling behaviour).
		 *
		 * One can observe spiraling, for instance, just printing the return values (and perhaps a route over-all distance)
		 * to standard out.

		const unsigned int THREADS = tile_size * tiles;
		const unsigned int dimension = static_cast<unsigned int>(path.size());
		const unsigned int number_of_2opt_exchanges = static_cast<unsigned int>(dimension - 3) * static_cast<unsigned int>(dimension - 2) / 2;
		const unsigned int iterations = (number_of_2opt_exchanges / (THREADS * tiles)) + 1;
		
		std::vector<opt2_move> candinate_2opt_moves(4096 * 4096);
		concurrency::array_view<opt2_move, 1> candinate_2opt_moves_view3(candinate_2opt_moves.size(), candinate_2opt_moves3);

		concurrency::array<int, 1> path_staging_array(dimension, path.begin(), path.end(), concurrency::accelerator(concurrency::accelerator::cpu_accelerator).default_view, accelerator.default_view);
		concurrency::array_view<const int, 1> path_view(path_staging_array);
		
		auto tsp_domain = concurrency::extent<1>(THREADS).tile<tiles>();
		parallel_for_each(accelerator.default_view, tsp_domain, [=](concurrency::tiled_index<tiles> t_idx) restrict(amp)
		{	
			for(unsigned int k = 0; k < iterations; ++k)
			{				
				unsigned int id = t_idx.global[0] + k * THREADS;
				if(id < number_of_2opt_exchanges)
				{
					unsigned int j = static_cast<unsigned int>(3 + concurrency::fast_math::sqrtf(8.0f * static_cast<float>(id) + 1.0f)) / 2;		
					unsigned int i = id - (j - 2) * (j - 1) / 2 + 1; 
										
					const int a = path_view[i];
					const int b = path_view[j + 1];
					const int c = path_view[i - 1];
					const int d = path_view[j];
								
					const auto index1 = utilities::to_row_major_index(b, a, dimension);
					const auto index2 = utilities::to_row_major_index(d, c, dimension);
					const auto index3 = utilities::to_row_major_index(c, a, dimension);
					const auto index4 = utilities::to_row_major_index(d, b, dimension);
					
					//Calculate the cost effect of swap and save for further reduction if better than some previous one.
					const float change = cost_matrix_view[index1] + cost_matrix_view[index2] - cost_matrix_view[index3] - cost_matrix_view[index4];
					
					if(change < 0)
					{
						candinate_2opt_moves_view[id] = best_2opt_move(i, j + 1, change);
					}
				}
			}
		});
		
		candinate_2opt_moves_view3.synchronize();
		return prune_asymmetric_candinates(candinate_2opt_moves, cost_matrix, cost_matrix_view, path, accelerator);
				
		return ret;
	} */
}