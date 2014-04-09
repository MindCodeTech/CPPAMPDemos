// source: amp_worker_ant.hpp
// use:	   Defines a Worker_ant class template in accordance with ACO.
//		   It is supposed to live exclusively in restrict(amp) space.
// author: Alex Voicu
// date:   29/06/2012

#pragma once
#ifndef _AMP_WORKER_ANT_HPP_BUMPTZI
#define _AMP_WORKER_ANT_HPP_BUMPTZI

#include <amp.h>				// For concurrency::array_view and concurrency::index.
#include "amp_bit_array.hpp"
#include "amp_tea_prng.hpp"
#include "helper_functions.hpp"

namespace ant
{
	// Basic worker ant class.
	template<typename T, unsigned int colony_sz> 
	class Worker_ant {
	public:
		typedef typename rng::Amp_tea_prng<T>::T_2 Trng_2;

		template<typename Idx_t>
		Worker_ant(const Idx_t& idx, const concurrency::array_view<const T, 2>& ch_inf, unsigned int rand_seed) restrict(amp) 
			: tabu_list(), prng(idx.global + rand_seed), choice_info(ch_inf), gidx(idx.global), lidx(idx.local), 
			cached_rndn(false), rnd_num(0), cached_prob(false), probability(-1)  { };

		operator unsigned int(void) const restrict(amp) { return lidx[1]; }; // Conversion to uint for convenience in being called by Queen-ant.

		T calc_probability(unsigned int curr_city, unsigned int offset) const restrict(amp);
		T read_probability(void) const restrict(amp) { return (cached_prob == true) ? probability : T(-1); }

		void new_city(void) const restrict(amp) { cached_prob = false; };
		void mark_visited(unsigned int city_idx) restrict(amp);
	private:
		T read_heuristic(unsigned int curr_city, unsigned int offset) const restrict(amp); 

		aba::Amp_bit_array tabu_list;
		mutable rng::Amp_tea_prng<T> prng;

		const concurrency::array_view<const T, 2> choice_info;
		const concurrency::index<2> gidx;
		const concurrency::index<2> lidx;

		mutable bool cached_rndn;
		mutable T rnd_num;        // Cached random number

		mutable bool cached_prob;
		mutable T probability;	  // Cached probability.
	};

	template<typename T, unsigned int colony_sz> 
	T Worker_ant<T, colony_sz>::calc_probability(unsigned int curr_city, unsigned int offset) const restrict(amp)
	{
		if (cached_rndn == false) {
			const auto rns = prng(); // Temp storage for generated pseudo-random numbers.
			rnd_num = rns.y; // Cache the second rn.
			cached_rndn = true;
				
			probability = read_heuristic(curr_city, offset) * rns.x;  // And use the first.
			cached_prob = true;
		}
		else {
			cached_rndn = false;

			probability = read_heuristic(curr_city, offset) * rnd_num; // Use cached pseudorandom-number.
			cached_prob = true;
		}
		return probability;
	}

	template<typename T, unsigned int colony_sz>
	void Worker_ant<T, colony_sz>::mark_visited(unsigned int city_idx) restrict(amp)
	{
		if (city_idx % colony_sz != lidx[1]) {   // Worker stride == colony_sz, so % by that will give us its address in the grid, making it easy to check if we have the right worker.
			return;
		}
		else {
			tabu_list.set(city_idx / colony_sz); // Dividing by the stride gives us the count of steps the ant has taken to reach the city, and therefore its idx in the local tabu_list.
		}
	}

	template<typename T, unsigned int colony_sz>
	T Worker_ant<T, colony_sz>::read_heuristic(unsigned int curr_city, unsigned int offset) const restrict(amp)
	{ 
		const auto candidate_city = gidx[1] + offset;
		const auto cand_idx = concurrency::index<2>(curr_city, candidate_city); // Current city sets the row.

		if (choice_info.extent.contains(cand_idx) == false) {
			return T(0); 
		}
		else {
			const auto tabu_idx = offset / colony_sz; // We walk down each row, / colony_sz returns the idx for the visited list.

			return (tabu_list[tabu_idx] == true) ? T(0) : choice_info(cand_idx); // Return zero for visited, heuristic val otherwise.
		}
	};
} // Namespace ant.
#endif //_AMP_WORKER_ANT_HPP_BUMPTZI

	