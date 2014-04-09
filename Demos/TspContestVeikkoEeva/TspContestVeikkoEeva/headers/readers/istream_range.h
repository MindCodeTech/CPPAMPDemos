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

#include <functional>
#include <iterator>

class istream;


namespace tsp
{	
	template<typename T>
	class istream_range
	{
		public:
			typedef std::istream_iterator<T> iterator_type;
 
			istream_range(std::istream& stream): stream_(stream) {}
			 
			iterator_type begin() const
			{
				return iterator_type(stream_);
			}
 

			iterator_type end() const
			{
				return iterator_type();
			}


			iterator_type begin()
			{
				return iterator_type(stream_);
			}
 

			iterator_type end()
			{
				return iterator_type();
			}

		private:
			std::istream& stream_;
	};
}