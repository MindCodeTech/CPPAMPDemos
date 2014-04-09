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
#include "matrix_utilities.h"
#include "utilities.h"

#include <amp.h>

#include <cmath>
#include <random>
#include <vector>


namespace kernel
{
	//Note that clamping random values with modulo isn't always the smartest thing to do.
	//See more at http://www.azillionmonkeys.com/qed/random.html or 
    //at http://stackoverflow.com/questions/2509679/how-to-generate-a-random-number-from-within-a-range-c.

	//TODO: These functions, or some of them, could benefit from having state information.
	//There are possibilities for parametrizing other things besides kicks too, like random factors
	//and if the cost_matrix should be used in accepting only moves that improve tour cost.
	//A functor with policy-based design, perhaps? See more at http://en.wikipedia.org/wiki/Policy-based_design.
	template<typename T>
	void double_bridge(concurrency::array<T, 1>& path, int kicks)
	{		
		//There's a good account on double bridge move at 
		//http://scim.brad.ac.uk/staff/pdf/picowlin/CowlingKeuthen2004.pdf
		//on page 7 onwards. This method on choosing at random should work equally
		//well on both symmetric and asymmetric instances. Equally well can mean also
		//equally poorly in that fully random move isn't necessarily the best one to adpot.
		//
		//In any event, this principle is also used by Kamil Rocki's and Reiji Suda's
		//CUDA implementation, though only for symmetric cases.
		//See more at http://olab.is.s.u-tokyo.ac.jp/~kamil.rocki/projects.html.
		//
		//There's some more notes at http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.48.5310,
		//see page 3.
		//
		//Then there are other methods of choosing an optimal 4-opt move, see at
		//http://www.researchgate.net/publication/225878428_Finding_a_best_traveling_salesman_4-opt_move_in_the_same_time_as_a_best_2-opt_move.

		for(int kick = 0; kick < kicks; ++kick)
		{
			//A random device probably isn't needed nor wanted, likewise not distributions.
			//At least not if this isn't put into production...
			//So just a fast random number generator will be used.
			//static std::random_device random_device;
			static std::mt19937 random_number_generator;
			const decltype(path.extent.size()) dimension = path.extent.size(); 
			decltype(path.extent.size()) a = random_number_generator() % dimension / 2;
			decltype(path.extent.size()) b = random_number_generator() % dimension;
						
			if(b < a)
			{
				std::swap(a, b);
			}

			std::reverse(std::begin(path) + a, std::begin(path) + b);
		}
	}


	template<typename T>
	T double_bridge(T& path, int kicks)
	{		
		//There's a good account on double bridge move at 
		//http://scim.brad.ac.uk/staff/pdf/picowlin/CowlingKeuthen2004.pdf
		//on page 7 onwards. This method on choosing at random should work equally
		//well on both symmetric and asymmetric instances. Equally well can mean also
		//equally poorly in that fully random move isn't necessarily the best one to adpot.
		//
		//In any event, this principle is also used by Kamil Rocki's and Reiji Suda's
		//CUDA implementation, though only for symmetric cases.
		//See more at http://olab.is.s.u-tokyo.ac.jp/~kamil.rocki/projects.html.
		//
		//There's some more notes at http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.48.5310,
		//see page 3.
		//
		//Then there are other methods of choosing an optimal 4-opt move, see at
		//http://www.researchgate.net/publication/225878428_Finding_a_best_traveling_salesman_4-opt_move_in_the_same_time_as_a_best_2-opt_move.

		for(int kick = 0; kick < kicks; ++kick)
		{
			//A random device probably isn't needed nor wanted, likewise not distributions.
			//At least not if this isn't put into production...
			//So just a fast random number generator will be used.
			//static std::random_device random_device;
			static std::mt19937 random_number_generator;
			const decltype(path.size()) dimension = path.size(); 
			decltype(path.size()) a = random_number_generator() % dimension;
			decltype(path.size()) b = random_number_generator() % dimension;
						
			if(b < a)
			{
				std::swap(a, b);
			}

			std::reverse(std::begin(path) + a, std::begin(path) + b);
		}
			
		return path;
	}


	template<typename T>
	std::vector<int> random_4opt(std::vector<T> const& cost_matrix, std::vector<int>& path)
	{	
		//TODO: Should this have a "kicks" parameter too instead of using cost_matrix? How about
		//introducing cost_matrix to double_bridge?
		//TODO: Check the proposed 4-opt on page 3 at:
        //http://www.fsa.ulaval.ca/personnel/renaudj/pdf/Recherche/Renaud,%20Boctor%20et%20Laporte%201996.pdf
		T best_change(1);
		while(best_change > 0)
		{
			//The idea here with regard to the double bridge move above is to introduce less
			//violent perturbation effects in asymmetric cases (how about symmetric ones?). But, but...
			//It looks like swapping edges isn't enough and actually using uniform distributions instead
			//of plain modulus will introduce a degrading effect. In the double_bridge case too, it's noticeable
			//how the results worsen if any of the distribution functions are used.

			const decltype(path.size()) dimension = path.size();
			//static std::random_device random_device;
			//static std::knuth_b random_number_generator(random_device());
			static std::mt19937 random_number_generator;
    
			std::uniform_int_distribution<decltype(path.size())> distribution1(0, dimension - 2);
			const decltype(path.size()) a = distribution1(random_number_generator);
    
			std::uniform_int_distribution<decltype(path.size())> distribution2(2, dimension - 2);
			const decltype(path.size()) b = (a + distribution2(random_number_generator)) % dimension;
			
			decltype(path.size()) c = (a + 1) % dimension;
			decltype(path.size()) d = (b + 1) % dimension;
			const decltype(path.size()) e = (c + 1) % dimension;
			const decltype(path.size()) f = (d + 1) % dimension;

			const auto index1 = utilities::to_row_major_index(a, c, dimension);
			const auto index2 = utilities::to_row_major_index(c, e, dimension);
			const auto index3 = utilities::to_row_major_index(b, d, dimension);
			const auto index4 = utilities::to_row_major_index(d, f, dimension);
			const auto index5 = utilities::to_row_major_index(a, d, dimension);
			const auto index6 = utilities::to_row_major_index(d, e, dimension);
			const auto index7 = utilities::to_row_major_index(b, c, dimension);
			const auto index8 = utilities::to_row_major_index(c, f, dimension);
		
			//Note: The idea/inspiration to cost change calculation is molested from the LKH code
			//at http://lkh.googlecode.com/svn/trunk/LHK/Best4OptMove.c
			//(I hope it's at least half-way there regarding correctness, something fishy in that the cost
			//has always the wrong sign for what it's supposed to have!).
			const T cost_change =  -(cost_matrix[index1] + cost_matrix[index2] + cost_matrix[index3] + cost_matrix[index4] -
					     		     cost_matrix[index5] - cost_matrix[index6] - cost_matrix[index7] - cost_matrix[index8]);
			
			if(cost_change < best_change)
			{
				//Hmm... Would this be possible to do without the reverse..? With a plain swap when
				//edges are chosen smart enough... Hmm also for the edges here now. Check!
				if(c > d)
				{
					std::swap(c, d);
				}
				std::reverse(path.begin() + c, path.begin() + d);
				
				/*
				/*if(e > f)
				{
					std::swap(e, f);
				}
				std::reverse(path.begin() + e, path.begin() + f);
				*/
				
				best_change = cost_change;
			}
		}

		return path;
	}


	std::vector<int> random_4opt2(std::vector<int>& path, int kicks)
	{
		//In search for research papers for the best perturbation function (ants, genes, swarms etc. not included currently)
		//this was found at http://www.comp.nus.edu.sg/~stevenha/database/viz/TSP_ILS.cpp.
		//Likely the author should be informed this is a copy of his perturbation function!
		std::vector<int> new_path(path.size());
		const decltype(path.size()) dimension = path.size();
		for(int kick = 0; kick < kicks; ++kick)
		{
			// 4-Opt double bridge move... pick 3 split points randomly...
			decltype(path.size()) position1 = 1 + rand() % (dimension / 4);
			decltype(path.size()) position2 = position1 + 1 + rand() % (dimension / 4);
			decltype(path.size()) position3 = position2 + 1 + rand() % (dimension / 4);

			// Perturb from current solution, in ILS, current solution is the LO...
			decltype(path.size()) j = 0;
			for(decltype(path.size()) i = 0; i < position1; ++i, ++j)
			{
				new_path[j] = path[i]; // Part A
			}

			for(decltype(path.size()) i = position3; i < dimension; ++i, ++j) 
			{
				new_path[j] = path[i]; // Part D
			}

			for(decltype(path.size()) i = position2; i < position3; ++i, ++j) 
			{
				new_path[j] = path[i]; // Part C
			}

			for(decltype(path.size()) i = position1; i < position2; ++i, ++j) 
			{
				new_path[j] = path[i]; // Part B
			}

			for(decltype(path.size()) i = 0; i < dimension; ++i)
			{
				path[i] = new_path[i]; // put back
			}
		}

		return path;
	}
}