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

#include <string>


namespace tsplib
{
	template<typename T>
	T best_known_tour_cost(std::string const& problem_instance_name)
	{
		//Sources for the costs:
		//http://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/STSP.html
		//http://eprints.pascal-network.org/archive/00001369/01/025_brest.pdf
		T best_known_tour_cost = static_cast<T>(-1.0);
		
		if(problem_instance_name == "att48")
		{
			best_known_tour_cost = static_cast<T>(10628);
		}
		if(problem_instance_name == "att532")
		{
			best_known_tour_cost = static_cast<T>(27686);
		}
		if(problem_instance_name == "bays29")
		{
			best_known_tour_cost = static_cast<T>(2020);
		}
		if(problem_instance_name == "berlin52")
		{
			best_known_tour_cost = static_cast<T>(7542);
		}
		else if(problem_instance_name == "d2103")
		{
			best_known_tour_cost = static_cast<T>(80450);
		}
		else if(problem_instance_name == "d657")
		{
			best_known_tour_cost = static_cast<T>(48912);
		}
		else if(problem_instance_name == "dsj1000")
		{
			//This is for CEIL_2D.
			best_known_tour_cost = static_cast<T>(18660188);
		}
		else if(problem_instance_name == "fl3795")
		{
			best_known_tour_cost = static_cast<T>(28772);
		} 
		else if(problem_instance_name == "fnl4461")
		{
			best_known_tour_cost = static_cast<T>(182566);
		}
		else if(problem_instance_name == "ftv33")
		{
			best_known_tour_cost = static_cast<T>(1286);
		}
		else if(problem_instance_name == "ft70")
		{
			best_known_tour_cost = static_cast<T>(38673);
		}
		else if(problem_instance_name == "ftv70")
		{
			best_known_tour_cost = static_cast<T>(1950);
		}
		else if(problem_instance_name == "ftv170")
		{
			best_known_tour_cost = static_cast<T>(2755);
		}
		else if(problem_instance_name == "kro124p")
		{
			best_known_tour_cost = static_cast<T>(36230);
		}
		else if(problem_instance_name == "kroA200")
		{
			best_known_tour_cost = static_cast<T>(29368);
		}
		else if(problem_instance_name == "pcb1173")
		{
			best_known_tour_cost = static_cast<T>(56892);
		}
		else if(problem_instance_name == "pla7397")
		{
			best_known_tour_cost = static_cast<T>(23260728);
		}
		else if(problem_instance_name == "rbg323")
		{
			best_known_tour_cost = static_cast<T>(1326);
		}
		else if(problem_instance_name == "rbg358")
		{
			best_known_tour_cost = static_cast<T>(1163);
		}
		else if(problem_instance_name == "rd100")
		{
			best_known_tour_cost = static_cast<T>(7910);
		}
		else if(problem_instance_name == "rbg403")
		{
			best_known_tour_cost = static_cast<T>(2465);
		}
		else if(problem_instance_name == "rbg443")
		{
			best_known_tour_cost = static_cast<T>(2720);
		}
				
		return best_known_tour_cost;
	}
}