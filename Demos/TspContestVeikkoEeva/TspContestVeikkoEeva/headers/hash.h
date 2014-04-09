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

#include <vector>


namespace std
{
	//This is a simple FNV-1a hasher, constants acquired from http://isthe.com/chongo/tech/comp/fnv/.
	//TODO: This isn't fool-proof...
	//TODO: Would implementation based on Mersenne twister be equally as good and more std?
	//TODO: There's most likely a more refined approach than these two, separate classes.
	//Probably something with traits and enable_if.
	template<typename T>
	struct hash<std::vector<T>>
	{
		typedef std::vector<T> argument_type;
		typedef size_t result_type;

		result_type operator()(argument_type const& argument) const
		{
			static const unsigned int fnv_prime_32 = 16777619;
			static const unsigned int offset_basis_32 = 2166136261U;
			
			uint32_t hash = offset_basis_32;
			result_type result_hash(0);
			for(T const& element: argument)
			{
				result_hash = result_hash ^ element;
				result_hash = result_hash * fnv_prime_32;
			}
			
			return result_hash;
		}
	};

	
	template<typename T>
	struct hash<concurrency::array<T, 1>>
	{
		typedef concurrency::array<T, 1> argument_type;
		typedef size_t result_type;

		result_type operator()(argument_type const& argument) const
		{
			static const unsigned int fnv_prime_32 = 16777619;
			static const unsigned int offset_basis_32 = 2166136261U;
			
			uint32_t hash = offset_basis_32;
			result_type result_hash(0);
			for(T const& element: argument)
			{
				result_hash = result_hash ^ element;
				result_hash = result_hash * fnv_prime_32;
			}
			
			return result_hash;
		}
	};
}