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

#include <cctype>
#include <functional>
#include <string>
#include <algorithm>

using std::isspace;
using std::not1;
using std::ptr_fun;
using std::string;


namespace utilities
{
	enum class string_split_options
	{
		none					= 0,
		remove_empty_entries	= 1
	};

	namespace
	{
		template<typename out_iterator>
		auto split(string const& string, char delimiter, out_iterator destination, string_split_options split_options = none) -> decltype(destination) 
		{
			decltype(string.size()) begin = 0;
			decltype(string.size()) end = 0;
			while((end = string.find(delimiter, begin)) != string::npos)
			{			
				auto sub_string = string.substr(begin, end - begin);
				begin = ++end;
				if(split_options == string_split_options::remove_empty_entries && sub_string == "")
				{
					continue;
				}
			
				*destination++ = sub_string;
			}
			
			auto sub_string = string.substr(begin);
			if(split_options == string_split_options::remove_empty_entries && sub_string == "")
			{
				return (*destination);
			}

			return (*destination)++ = sub_string;
		}


		inline string& trim_left(string& string_to_trim)
		{
			string_to_trim.erase(string_to_trim.begin(), find_if(string_to_trim.begin(), string_to_trim.end(), not1(ptr_fun<int, int>(isspace))));
	
			return string_to_trim;
		}


		inline string& trim_right(string& string_to_trim)
		{
				string_to_trim.erase(find_if(string_to_trim.rbegin(), string_to_trim.rend(), not1(ptr_fun<int, int>(isspace))).base(), string_to_trim.end());

				return string_to_trim;
		}


		inline string& trim(string& string_to_trim)
		{
			return trim_left(trim_right(string_to_trim));
		}
	}
}