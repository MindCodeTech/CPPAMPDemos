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

#include <amp.h>

#include <functional>
#include <numeric>
#include <type_traits>
#include <vector>


namespace utilities
{
	std::vector<concurrency::accelerator> get_accelerators()
	{
		std::vector<concurrency::accelerator> accelerators = concurrency::accelerator::get_all();
		//accelerators.push_back(concurrency::accelerator(concurrency::accelerator::direct3d_ref));
		
		accelerators.erase(std::remove_if(accelerators.begin(), accelerators.end(), [](concurrency::accelerator const& accelerator)
		{
			return accelerator.is_emulated;
		}), accelerators.end());
		//accelerators.push_back(concurrency::accelerator(concurrency::accelerator::direct3d_warp));
		
		return accelerators;
	}
		
	
	//TODO: Do this is in a more type-safe way. Note that the starting symbol needs to be 0 is zero-based
	//indexing is assumed in calculation. The route symbols are being used in indexing the cost matrix.
	template<typename path_element_type>
	std::vector<path_element_type> generate_path_symbols(size_t dimension, path_element_type path_start_symbol = 0)
	{
		//TODO: Here should be static_assert for this being an integral type. Also, the path/tour/route element should be
		//a class!
		std::vector<path_element_type> path_symbols(dimension);
		std::iota(path_symbols.begin(), path_symbols.end(), path_start_symbol);
		
		return path_symbols;
	}


	template<typename path_element_type>
	concurrency::array<path_element_type, 1> generate_staged_path_symbols(concurrency::accelerator accelerator, size_t dimension, path_element_type path_start_symbol = 0)
	{
		//TODO: Here should be static_assert for this being an integral type. Also, the path/tour/route element should be
		//a class!
		concurrency::array<path_element_type, 1> staged_path_array(dimension, concurrency::accelerator(concurrency::accelerator::cpu_accelerator).default_view, accelerator.default_view);
		std::iota(std::begin(staged_path_array), std::end(staged_path_array), path_start_symbol);
				
		return staged_path_array;
	}
}