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

#include <type_traits>


namespace utilities
{
	//TODO: Should these return always ptrdiff_t, size_t or somesuch? It's probably something the caller
	//should static_assert if needed.
	//See at
	//http://www.viva64.com/en/a/0050/
	//http://www.viva64.com/en/a/0018/print/
	//and so forth for the reasoning.
	template<typename x_type, typename y_type, typename dimension_type>
	typename std::common_type<x_type, y_type, dimension_type>::type to_column_major_index(x_type const& x, y_type const& y, dimension_type const& square_dimensions)
	{
		return square_dimensions * y + x;
	}
	
	
	template<typename x_type, typename y_type, typename dimension_type>
	typename std::common_type<x_type, y_type, dimension_type>::type to_row_major_index(x_type const& x, y_type const& y, dimension_type const& square_dimensions) restrict(cpu, amp)
	{
		return square_dimensions * x + y;
	}


	template<typename column_major_type, typename width_type, typename height_type>
	typename std::common_type<column_major_type, width_type, height_type>::type to_row_major_index_from_column_major(column_major_type const& column_major_index, width_type const& width, height_type const& height)
	{
		const typename std::common_type<column_major_type, width_type>::type row = column_major_index % height;
		const typename std::common_type<column_major_type, height_type>::type column = column_major_index / height;
		return row * width + column;

	}

		
	template<typename row_major_type, typename width_type, typename height_type>
	typename std::common_type<row_major_type, width_type, height_type>::type to_column_major_index_from_row_major(row_major_type const& row_major_index, width_type const& width, height_type const& height)
	{
		return to_row_major_index_from_column_major(row_major_index, height, width);
	}

		
	template<typename x_type, typename y_type, typename dimension_type>
	typename std::common_type<x_type, y_type, dimension_type>::type to_row_major_index_from_column_major_indices(x_type const& x, y_type const& y, dimension_type const& square_dimensions)
	{
		return to_row_major_index(y, x, square_dimensions);
	}
		

	template<typename x_type, typename y_type, typename dimension_type>
	typename std::common_type<x_type, y_type, dimension_type>::type to_column_major_index_from_row_major_indices(x_type const& x, y_type const& y, dimension_type const& square_dimensions)
	{
		return to_row_major_index_from_column_major_indices(x, y, square_dimensions);
	}
}