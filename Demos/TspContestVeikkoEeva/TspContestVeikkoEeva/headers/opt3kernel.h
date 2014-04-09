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

#include "kernel_utilities.h"
#include "opt2_move.h"

#include <amp.h>

#include <vector>


namespace kernel
{
	template<typename T>
	T reduced_opt3kernel_path_conditioner(std::vector<T> const& cost_matrix, concurrency::array<int, 1>& path)
	{
		const decltype(path.extent.size()) dimension = path.extent.size();
				
		//There are eight ways to check a path, one which doesn't reverse the route, also known as
		//"reduced 3-opt". 
		//Deciphering from here http://www.slidefinder.net/c/comp8620_lecture_neighbourhood_methods_local/lecture5-6/22189829
		//the following feels like being the combination.
		//TODO: Actually check besides having feelings...
		T new_distance(0);
		T current_distance = kernel::solution_cost<T>(cost_matrix, path);
		for(decltype(path.extent.size()) i = 0; i < dimension - 5; ++i)
		{
			for(decltype(path.extent.size()) j = i + 3; j < dimension - 3; ++j)
			{
				for(decltype(path.extent.size()) k = j + 3; k < dimension - 1; ++k)
				{
					const int b = i + 1;
					const int d = j + 1;
					const int f = k + 1;
					
					const int b_old = path[b];
					const int d_old = path[d];
					const int f_old = path[f];
										
					int temp = path[b];	
					path[b] = path[d];	
					path[d] = path[f];
					path[f] = temp;

					new_distance = kernel::solution_cost<T>(cost_matrix, path);
					if(current_distance < new_distance)
					{
						path[b] = b_old;
						path[d] = d_old;
						path[f] = f_old;
					}
					else
					{
						current_distance = new_distance;
					}
				}
			}	
		}

		return new_distance;
	}
}