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

#include "iterator_overloads.h"
#include "matrix_utilities.h"
#include "testing.h"

#include <amp.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <random>
#include <vector>


namespace kernel
{
	namespace
	{
		template<typename T>
		void erase(std::vector<T>& vector, size_t index)
		{
			std::swap(vector[index], vector.back());
			vector.pop_back();
		}
	}

	//TODO: There are plenty of different methods. In that case, maybe this could be a
	//facade file (easy to spot amongst headers) or removed entirely.

	template<typename cost_element_type, typename path_element_type>
	std::vector<path_element_type> nearest_neighbor(std::vector<cost_element_type> const& cost_matrix, concurrency::array<path_element_type, 1> const& path)
	{
		//static std::mt19937 random_number_generator;
		const decltype(path.extent.size()) dimension = path.extent.size();
		std::vector<path_element_type> construction_path(std::begin(path) + 1, std::end(path));
		std::vector<path_element_type> neighbor_path;
        
		path_element_type index(0);
		path_element_type best_city(0);
		cost_element_type best_cost(0);
		neighbor_path.push_back(path[0]);
    
		while(!construction_path.empty())
		{
			best_city = 0;
			best_cost = cost_matrix[utilities::to_row_major_index(neighbor_path[index], construction_path[best_city], dimension)];
			for(decltype(construction_path.size()) i = 0; i < construction_path.size(); ++i)
			{
				//This is biased towards "first low cost". Should be taken to a vector instead and a random selection made amongst the draw ones?
				auto temp_cost = cost_matrix[utilities::to_row_major_index(neighbor_path[index], construction_path[i], dimension)]; 
				if(temp_cost < best_cost)
				{
					best_cost = temp_cost;
					best_city = i;
				}
			}
		
			neighbor_path.push_back(construction_path[best_city]);
			erase(construction_path, best_city);
			++index;
		}

		assert(test::no_duplicates(neighbor_path));
		return neighbor_path;
	}
}