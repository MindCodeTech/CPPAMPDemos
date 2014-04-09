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

#include "opt2_move.h"
#include "opt2kernel.h"

#include <amp.h>

#include <algorithm>
#include <vector>


namespace kernel
{
	class context
	{
		public:
			static const size_t problem_size_category_6 = 8192;
			static const size_t problem_size_category_5 = 4096;
			static const size_t problem_size_category_4 = 2045;
			static const size_t problem_size_category_3 = 1024;
			static const size_t problem_size_category_2 = 512;
			static const size_t problem_size_category_1 = 256;
			static const size_t problem_size_category_0	= 64; 
	};

	template<typename cost_element_type>
	class kernel_context
	{
		public:
			kernel_context(std::vector<cost_element_type> const& cost_matrix, size_t dimension, bool is_symmetric, concurrency::accelerator const& accelerator): 
				cost_matrix_(cost_matrix),
				dimension_(dimension),
				is_symmetric_(is_symmetric),
				accelerator_(accelerator),
				cost_matrix_view_(concurrency::array<cost_element_type, 1>(cost_matrix.size(), std::begin(cost_matrix), std::end(cost_matrix), concurrency::accelerator(concurrency::accelerator::cpu_accelerator).default_view, accelerator.default_view)),
				candinate_2opt_moves_staging_array_(1, concurrency::accelerator(concurrency::accelerator::cpu_accelerator).default_view, accelerator.default_view)
			{
				if(dimension_ <= context::problem_size_category_1)
				{
					candinate_2opt_moves_staging_array_ = concurrency::array<opt2_move, 1>(256, concurrency::accelerator(concurrency::accelerator::cpu_accelerator).default_view, accelerator.default_view);
				}
				else if(dimension_ <= context::problem_size_category_2)
				{
					candinate_2opt_moves_staging_array_ = concurrency::array<opt2_move, 1>(512, concurrency::accelerator(concurrency::accelerator::cpu_accelerator).default_view, accelerator.default_view);
				}
				else if(dimension_ <= context::problem_size_category_3)
				{
					candinate_2opt_moves_staging_array_ = concurrency::array<opt2_move, 1>(1024, concurrency::accelerator(concurrency::accelerator::cpu_accelerator).default_view, accelerator.default_view);
				}
				else if(dimension_ <= context::problem_size_category_4)
				{
					candinate_2opt_moves_staging_array_ = concurrency::array<opt2_move, 1>(1024, concurrency::accelerator(concurrency::accelerator::cpu_accelerator).default_view, accelerator.default_view);
				}
				else if(dimension_ <= context::problem_size_category_5)
				{
					candinate_2opt_moves_staging_array_ = concurrency::array<opt2_move, 1>(1024, concurrency::accelerator(concurrency::accelerator::cpu_accelerator).default_view, accelerator.default_view);
				}
				else if(dimension_ <= context::problem_size_category_6)
				{
					candinate_2opt_moves_staging_array_ = concurrency::array<opt2_move, 1>(1024, concurrency::accelerator(concurrency::accelerator::cpu_accelerator).default_view, accelerator.default_view);
				}
			};
			
			~kernel_context() {};

			template<typename path_element_type>
			opt2_move opt2kernel_calculation(concurrency::array<path_element_type, 1>& path_staging_array, concurrency::array<path_element_type, 1> const& gpu_path)
			{
				opt2_move result;
				if(dimension_ <= context::problem_size_category_0)
				{
					if(is_symmetric_)
					{
						result = opt2kernel_symmetric(cost_matrix_, path_staging_array);
					}
					else
					{
						result = result = opt2kernel_asymmetric(cost_matrix_, path_staging_array);
					}
				}
				else
				{
					concurrency::array_view<opt2_move, 1> candinate_2opt_moves(candinate_2opt_moves_staging_array_);
					candinate_2opt_moves.discard_data();
					
					if(dimension_ <= context::problem_size_category_1)
					{
						opt2_simple_kernel_symmetric<cost_element_type, 256, 256>(cost_matrix_view_, gpu_path, candinate_2opt_moves, accelerator_);
					}
					else if(dimension_ <= context::problem_size_category_2)
					{
						opt2_simple_kernel_symmetric<cost_element_type, 512, 512>(cost_matrix_view_, gpu_path, candinate_2opt_moves, accelerator_);
					}
					else if(dimension_ <= context::problem_size_category_3)
					{
						opt2_simple_kernel_symmetric<cost_element_type, 1024, 1024>(cost_matrix_view_, gpu_path, candinate_2opt_moves, accelerator_);
					}
					else if(dimension_ <= context::problem_size_category_4)
					{
						opt2_simple_kernel_symmetric<cost_element_type, 1024, 1024>(cost_matrix_view_, gpu_path, candinate_2opt_moves, accelerator_);
					}
					else if(dimension_ <= context::problem_size_category_5)
					{
						opt2_simple_kernel_symmetric<cost_element_type, 1024, 1024>(cost_matrix_view_, gpu_path, candinate_2opt_moves, accelerator_);
					}
					else if(dimension_ <= context::problem_size_category_6)
					{
						opt2_simple_kernel_symmetric<cost_element_type, 2048 , 1024>(cost_matrix_view_, gpu_path, candinate_2opt_moves, accelerator_);
					}

					if(is_symmetric_)
					{
						result = *std::min_element(std::begin(candinate_2opt_moves), std::end(candinate_2opt_moves), [](opt2_move const& a, opt2_move const& b) { return(a.minchange_ < b.minchange_); });
					}
					else
					{
						result = prune_asymmetric_candinates(candinate_2opt_moves, cost_matrix_, path_staging_array);
					}
				} 
				
				return result;
			}


		private:
			size_t dimension_;
			bool is_symmetric_;
			const concurrency::array_view<const cost_element_type, 1> cost_matrix_view_;
			concurrency::array<opt2_move, 1> candinate_2opt_moves_staging_array_;
			const std::vector<cost_element_type> cost_matrix_;
			const concurrency::accelerator accelerator_;

			template<typename opt2_container_type, typename T>
			opt2_move prune_asymmetric_candinates(opt2_container_type const& candinate_2opt_moves, std::vector<T> const& cost_matrix, concurrency::array<int, 1> const& path)
			{
				const T current_tour_cost = solution_cost<T>(cost_matrix, path);
				auto temp_path = path;
				T best_reverse_cost = current_tour_cost;
				opt2_move best_move = opt2_move(0, 0, 0);
								
				for(decltype(candinate_2opt_moves.extent.size()) i = 0; i < candinate_2opt_moves.extent.size(); ++i)
				{					
					if(candinate_2opt_moves[i].minchange_ < best_move.minchange_ && candinate_2opt_moves[i].i_ != candinate_2opt_moves[i].j_)
					{
						int x1 = candinate_2opt_moves[i].i_;
						int x2 = candinate_2opt_moves[i].j_;
						if(x1 > x2)
						{	
							std::swap(x1, x2);
						}

						std::reverse(std::begin(temp_path) + x1, std::begin(temp_path) + x2);
						T reversed_cost = solution_cost<T>(cost_matrix, temp_path);
						if(best_reverse_cost > reversed_cost)
						{
							best_reverse_cost = reversed_cost;
							best_move = opt2_move(candinate_2opt_moves[i].i_, candinate_2opt_moves[i].j_, reversed_cost - current_tour_cost);
						}

						//This temporary path will be reused, so it should be reversed back.
						std::reverse(std::begin(temp_path) + x1, std::begin(temp_path) + x2);
					}
				}
			  
				return best_move;
			}
	};
}