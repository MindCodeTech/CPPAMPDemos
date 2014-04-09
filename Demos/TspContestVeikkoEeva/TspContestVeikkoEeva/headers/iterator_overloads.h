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


namespace std
{
	//TODO: Note a complete enumeration. Probably alsot not the smartest way.
	template<typename T>
	T* begin(concurrency::array<T, 1>& a)
	{
		return a.data();
	}

	template<typename T>
	const T* begin(concurrency::array<T, 1> const& a)
	{
		return a.data();
	}


	template<typename T>
	T* end(concurrency::array<T, 1>& a)
	{
		return a.data() + a.extent.size();
	}
	

	template<typename T>
	const T* end(concurrency::array<T, 1> const& a)
	{
		return a.data() + a.extent.size();
	}

	template<typename T>
	T* begin(concurrency::array_view<T, 1>& a)
	{
		return a.data();
	}

	template<typename T>
	const T* begin(concurrency::array_view<T, 1> const& a)
	{
		return a.data();
	}


	template<typename T>
	T* end(concurrency::array_view<T, 1>& a)
	{
		return a.data() + a.extent.size();
	}
	

	template<typename T>
	const T* end(concurrency::array_view<T, 1> const& a)
	{
		return a.data() + a.extent.size();
	}
}