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

#include "hash.h"
#include "spinlock.h"
#include "utilities.h"

#include <vector>
#include <unordered_set>


namespace
{
	class path_pruner
	{
		public:
			path_pruner(): paths_rejected_(0) {};
			~path_pruner() {};

			template<typename T>
			bool try_add(T const& path)
			{
				auto hash = hash_path(path);
				bool has_path_been_added(false);
				lock_.enter();
				if(has_path_been_added = path_container_.find(hash) == path_container_.end())
				{					
					path_container_.insert(hash);
				}
				else
				{
					++paths_rejected_;
				}
				lock_.exit();
								
				
				return has_path_been_added;
			}


			template<typename T>
			size_t is_pruned(T const& path) const
			{
				auto hash = hash_path(path);
				lock_.enter();
				bool has_path_been_added = path_container_.find(hash) != path_container_.end();
				lock_.exit();

				return has_path_been_added;
			}
			

			size_t rejected_paths() const
			{
				lock_.enter();
				size_t paths_rejected = paths_rejected_;
				lock_.exit();

				return paths_rejected;
			}


			size_t approved_paths() const
			{				
				lock_.enter();
				size_t paths_approved = path_container_.size();
				lock_.exit();

				return paths_approved;
			}

		private:
			template<typename T>
			static size_t hash_path(T const& path)
			{
				//This hasher is here and not in multiset definition as this arrangement allows
				//for using the same hash for find and insert operations.
				static const std::hash<T> hasher;
				
				return hasher(path);
			}

			size_t paths_rejected_;
			path_pruner(path_pruner const&);
			path_pruner& operator=(path_pruner const&);

			mutable utilities::spinlock lock_;
			std::unordered_multiset<size_t> path_container_;
	};
}