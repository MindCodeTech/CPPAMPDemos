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

#include <bitset>
#include <vector>


namespace test
{
	template<typename path_container>
	bool no_duplicates(path_container const& path)
	{
		//TODO: This should be in a testing project, along with other tests. Maybe some nice sould will
		//add an example of a proper unit test and performance test project. Maybe something like
		//http://www.cambridge.org/gb/knowledge/isbn/item6618711/ would be of help.
						
		//Gleansed (checked for more advanced method, which does indeed exist)
		//from http://stackoverflow.com/questions/2872959/how-to-tell-if-an-array-is-a-permutation-in-on
		//and http://stackoverflow.com/questions/1851716/algorithm-to-find-the-duplicate-numbers-in-an-array-fastest-way.
		//Note that this method isn't the fastest possible and the math overflow question implicates ideas for
		//the path_pruner hashing (also a GPU version) if the applicability of it survives.
		
		//And yeah and so on, std::vector<bool> is broken and there's std::bitset and all...
		const decltype(path.size()) dimension = path.size();
		std::vector<bool> path_check(path.size());
						
		for(auto const& a: path)
		{
			path_check[a] = true;
		}

		for(decltype(path_check.size()) i = 0; i < dimension; ++i)
		{
			if(!path_check[i])
			{
				return false;
			}
		}
			
		return true;
	}
}
