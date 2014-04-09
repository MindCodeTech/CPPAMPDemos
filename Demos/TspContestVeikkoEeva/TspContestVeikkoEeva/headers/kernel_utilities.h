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

#pragma once

#include <amp.h>

#include <vector>


namespace kernel
{
	//TODO: When operating with floating points, should Kahan or pair-wise summation be used?
	//See more at http://en.wikipedia.org/wiki/Kahan_summation_algorithm and http://en.wikipedia.org/wiki/Pairwise_summation.

	template<typename out_type, typename cost_element_type, typename path_element_type>
	out_type parallel_solution_cost(concurrency::array_view<const cost_element_type, 1> const& cost_matrix_view, std::vector<path_element_type> const& path, concurrency::accelerator const& accelerator)
	{	
		//TODO: Is it possible to acquire the actual cost_element type name also?
		static_assert(std::is_convertible<out_type, cost_element_type>::value, "The \"out_type\" is not implicitly convertible from \"cost_element_type\"");
				
		const int dimension = static_cast<unsigned int>(path.size());
		const concurrency::array<path_element_type, 1> path_staging_array(dimension, path.begin(), path.end(), concurrency::accelerator(concurrency::accelerator::cpu_accelerator).default_view, accelerator.default_view);
		const concurrency::array_view<const path_element_type, 1> path_view(path_staging_array);
		
		static const int TILE_SIZE = 16;
		std::vector<out_type> partial_sums(256);
		concurrency::array_view< out_type, 1> partial_sums_view(partial_sums.size(), partial_sums);
		 
		auto sum_domain = concurrency::extent<1>(path.size()).tile<TILE_SIZE>();
		concurrency::parallel_for_each(accelerator.default_view, sum_domain.pad(), [=](concurrency::tiled_index<TILE_SIZE> t_idx) restrict(amp)
		{
			tile_static out_type partial_sum[TILE_SIZE];
			partial_sum[t_idx.local[0]] = 0;
			if(t_idx.global[0] < dimension - 1)
			{
				const auto index = utilities::to_row_major_index(path_view[t_idx.global[0] + 1], path_view[t_idx.global[0]], dimension);
				partial_sum[t_idx.local[0]] = cost_matrix_view[index];
			}
						
			t_idx.barrier.wait();

			for(int offset = t_idx.tile_dim0 / 2; offset > 0; offset /= 2)
			{
				if(t_idx.local[0] < offset)
				{
					partial_sum[t_idx.local[0]] += partial_sum[t_idx.local[0] + offset];
				}
				
				t_idx.barrier.wait();
			}
			
			if(t_idx.local[0] == 0)
			{
				partial_sums_view[t_idx.tile[0]] = partial_sum[0];
			}
		});
		partial_sums_view.synchronize();

		out_type cost(0);
		cost = std::accumulate(partial_sums.begin(), partial_sums.end(), 0, std::plus<out_type>());

		const decltype(cost_matrix_view.extent.size()) index =  utilities::to_row_major_index(path[0], path_view[dimension - 1], dimension);
		cost += cost_matrix_view[index];
				
		return cost;
	}


	//TODO: in asymmetric cases quite many of the cost calculations come after reversing the route. It could be beneficial to
	//introduce indices or iterators to path and calculate only the sub-tour cost and use it with the known current tour.
	template<typename out_type, typename cost_element_type, typename path_element_type>
	out_type solution_cost_reversed(std::vector<cost_element_type> const& cost_matrix, std::vector<path_element_type> const& path, size_t begin, size_t end)
	{	
		//TODO: Is it possible to acquire the actual cost_element type name also?
		static_assert(std::is_convertible<out_type, cost_element_type>::value, "The \"out_type\" is not implicitly convertible from \"cost_element_type\"");
		
		//Note that reversing the indexing done to the cost matrix like this (see solution_cost<T> below) has the same effect as
		//using utilities::to_column_major_index.
		out_type cost(0);
		const decltype(path.size()) dimension = path.size();
		for(decltype(path.size()) i = 0; i < dimension - 1; ++i) 
		{
			const decltype(cost_matrix.size()) index = utilities::to_row_major_index(path[i], path[i + 1], dimension);
			cost += cost_matrix[index];
		}

		const decltype(cost_matrix.size()) index =  utilities::to_row_major_index(path[dimension - 1], path[0], dimension);
		cost += cost_matrix[index];
		
		return cost;
	}

	
	//TODO: in asymmetric cases quite many of the cost calculations come after reversing the route. It could be beneficial to
	//introduce indices or iterators to path and calculate only the sub-tour cost and use it with the known current tour.
	template<typename out_type, typename cost_element_type, typename path_element_type>
	out_type solution_cost(std::vector<cost_element_type> const& cost_matrix, std::vector<path_element_type> const& path)
	{	
		//TODO: Is it possible to acquire the actual cost_element type name also?
		static_assert(std::is_convertible<out_type, cost_element_type>::value, "The \"out_type\" is not implicitly convertible from \"cost_element_type\"");
		
		out_type cost(0);
		const decltype(path.size()) dimension = path.size();
		for(decltype(path.size()) i = 0; i < dimension - 1; ++i) 
		{
			const decltype(cost_matrix.size()) index = utilities::to_row_major_index(path[i + 1], path[i], dimension);
			cost += cost_matrix[index];
		}

		const decltype(cost_matrix.size()) index =  utilities::to_row_major_index(path[0], path[dimension - 1], dimension);
		cost += cost_matrix[index];
						
		return cost;
	}


	//TODO: in asymmetric cases quite many of the cost calculations come after reversing the route. It could be beneficial to
	//introduce indices or iterators to path and calculate only the sub-tour cost and use it with the known current tour.
	template<typename out_type, typename cost_element_type, typename path_element_type>
	out_type solution_cost(std::vector<cost_element_type> const& cost_matrix, concurrency::array<path_element_type> const& path)
	{	
		//TODO: Is it possible to acquire the actual cost_element type name also?
		static_assert(std::is_convertible<out_type, cost_element_type>::value, "The \"out_type\" is not implicitly convertible from \"cost_element_type\"");
		
		out_type cost(0);
		const decltype(path.extent.size()) dimension = path.extent.size();
		for(decltype(path.extent.size()) i = 0; i < dimension - 1; ++i) 
		{
			const decltype(cost_matrix.size()) index = utilities::to_row_major_index(path[i + 1], path[i], dimension);
			cost += cost_matrix[index];
		}

		const decltype(cost_matrix.size()) index =  utilities::to_row_major_index(path[0], path[dimension - 1], dimension);
		cost += cost_matrix[index];
						
		return cost;
	}
}
