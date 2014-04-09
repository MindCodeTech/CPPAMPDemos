// source: amp_queen_ant.hpp
// use:	   Defines and declares a Queen_ant class template in accordance with ACO.
//		   It is supposed to live exclusively in restrict(amp) space.
// author: Alex Voicu
// date:   29/06/2012

#pragma once
#ifndef _AMP_QUEEN_ANT_HPP_BUMPTZI
#define _AMP_QUEEN_ANT_HPP_BUMPTZI

#include <amp.h>			  // For concurrency::tile_barrier.
#include "amp_worker_ant.hpp"

namespace ant
{
	// Queen_ant should live in LDS, therefore she won't get constructors 
	// (constructing the temporary for initialization through assignment would be register-crippling).
	template<typename T, unsigned int colony_sz, unsigned int tour_lds_sz> 
	class Queen_ant {
	public: 
		template<typename Idx_t>
		void init(const Idx_t& idx, unsigned int cities) restrict(amp);
	
		void find_best_cand(const Worker_ant<T, colony_sz>& worker) restrict(amp);
		void go_to_next(Worker_ant<T, colony_sz>& worker) restrict(amp);

		template<typename Tex, typename Av>
		void export_costs(const Worker_ant<T, colony_sz>& worker, const Tex& costs, Av& tour_costs) restrict(amp); // Makes sense to have the queen handle it, she has the tours in LDS.
		template<typename Av>
		void export_tour(const Worker_ant<T, colony_sz>& worker, Av& tours) const restrict(amp);

		void wait_local_fence(void) const restrict(amp) { barrier.wait_with_tile_static_memory_fence(); };
		void wait_global_fence(void) const restrict(amp) { barrier.wait_with_global_memory_fence(); };
		void wait_all_fence(void) const restrict(amp) { barrier.wait_with_all_memory_fence(); };
	private:
		void choose_best(const Worker_ant<T, colony_sz>& worker, unsigned int offset) restrict(amp);

		unsigned int queen_tour[tour_lds_sz]; // LDS chunk that houses the queen's tour as it's being built.
		T shared_store[colony_sz];		      // LDS chunk that provides scratch-space for intermediate reductions.
		
		unsigned int tour_end;			  // Index of one past the last node added to the list.
		unsigned int city_cnt;	          // Total count of cities / nodes.
		
		concurrency::tile_barrier barrier; // Queen's "rod" in the form of the group's barrier; queen handles the syncs!

		T max_prob;    // This caches the highest recorded prob.
		
		unsigned int best_candidate; // This caches the idx attached to the highest recorded prob.
		unsigned int curr_city;      // The current residence of the queen.
	};

	template<typename T, unsigned int colony_sz, unsigned int tour_lds_sz>
	template<typename Idx_t>
	void Queen_ant<T, colony_sz, tour_lds_sz>::init(const Idx_t& idx, unsigned int cities) restrict(amp) // Use this to init the Queen in her LDS lair.
	{
		if (idx.local[1] == 0) {
			queen_tour[0] = idx.tile[0];       // Put starting city in the tour.
			shared_store[idx.local[1]] = T(0); // Zero out store.
			tour_end = 1;                      // We already filled out an elem.
			city_cnt = cities;                 // Queen is aware of how many stops she'll need to make.
			barrier = idx.barrier;			   // Queen gets her rod of power.
			max_prob = T(-1);				   // Same as above.
			best_candidate = 0;				   // Same as above.
			curr_city = idx.tile[0];		   // This is the city where we woke up.
		}
		else {
			shared_store[idx.local[1]] = T(0);
		}
		wait_local_fence(); // We now have a barrier, let's use it to order the ants to wait until we wake up properly.
	}

	template<typename T, unsigned int colony_sz, unsigned int tour_lds_sz>
	void Queen_ant<T, colony_sz, tour_lds_sz>::find_best_cand(const Worker_ant<T, colony_sz>& worker) restrict(amp)
	{
		register const auto loc_city_cnt = city_cnt;
		wait_local_fence(); // Make sure everybody sees the same loop counter and hits the same barriers.

		for (auto i = 0u; i < loc_city_cnt; i += colony_sz) {
			shared_store[worker] = worker.calc_probability(curr_city, i);
			
			wait_local_fence();
			
			choose_best(worker, i);
		}
	}

	template<typename T, unsigned int colony_sz, unsigned int tour_lds_sz>
	void Queen_ant<T, colony_sz, tour_lds_sz>::choose_best(const Worker_ant<T, colony_sz>& worker, unsigned int offset) restrict(amp)
	{
		register const auto cur_max = hlp::amp_scan(shared_store, shared_store + colony_sz, worker, barrier, hlp::Amp_max<T>());	
	
		if ((worker == 0u) && (max_prob < cur_max)) { // We've found a better candidate, so we store him.
			 max_prob = cur_max;
		}
		wait_local_fence(); // Make sure everybdy sees the same max_prob.
	
		if (worker.read_probability() == max_prob && offset + worker < city_cnt) { // Using == between floats is quite unwise.
			concurrency::atomic_exchange(&best_candidate, offset + worker);        // Offset puts us in the tile of considered cities, the particular worker indexes into it.
		}
		wait_local_fence(); // Make sure we've properly updated the best_candidate.
	}

	template<typename T, unsigned int colony_sz, unsigned int tour_lds_sz>
	void Queen_ant<T, colony_sz, tour_lds_sz>::go_to_next(Worker_ant<T, colony_sz>& worker) restrict(amp)
	{ 
		worker.mark_visited(best_candidate); // Inform brood of movement.

		if (worker == 0u) {
			curr_city = best_candidate;              // Move to next.
			max_prob = T(-1);			             // Clear stored prob.
			queen_tour[tour_end++] = best_candidate; // Append it to the end of the tour.
		}
		wait_local_fence();

		worker.new_city(); // Inform workers we're moving.
	}

	template<typename T, unsigned int colony_sz, unsigned int tour_lds_sz>
	template<typename Tex, typename Av>
	void Queen_ant<T, colony_sz, tour_lds_sz>::export_costs(const Worker_ant<T, colony_sz>& worker, const Tex& costs, Av& tour_costs) restrict(amp)
	{
		register const auto adj_tour_end = sizeof(queen_tour) / sizeof(queen_tour[0]) - 1; // Trick it so that we don't get tile_barrier whines, off by one so that we can build pairs of nodes for indexing into costs mtx.
		shared_store[worker] = T(0);
		wait_local_fence();

		for (auto i = 0u; i < adj_tour_end; i += colony_sz) {
			if (i + worker + 1 < tour_end) { // Check to ensure that we're still in a valid interval.
				const concurrency::index<2> cost_idx(queen_tour[i + worker], queen_tour[i + worker + 1]);

				shared_store[worker] += costs(cost_idx);
			}
			wait_local_fence();
		}
		
		const auto temp_sum = hlp::amp_scan(shared_store, shared_store + colony_sz, worker, barrier, hlp::Amp_plus<T>());
		if (worker == 0u) {
			const auto final_idx = concurrency::index<2>(queen_tour[adj_tour_end], queen_tour[0]); // Close the loop by going back to the city of birth.
			tour_costs(queen_tour[0]) = temp_sum + costs(final_idx);
		}
		wait_global_fence();
	}

	template<typename T, unsigned int colony_sz, unsigned int tour_lds_sz>
	template<typename Av>
	void Queen_ant<T, colony_sz, tour_lds_sz>::export_tour(const Worker_ant<T, colony_sz>& worker, Av& tours) const restrict(amp)
	{
		for (auto i = 0u; i < tour_end; i += colony_sz) {
			const concurrency::index<2> out_idx(queen_tour[0], i + worker); // On the starting city's row.
			if (tours.get_extent().contains(out_idx) == true) {
				tours(out_idx) = queen_tour[out_idx[1]];
			}
		}
	}
} // Namespace ant.
#endif // _AMP_QUEEN_ANT_HPP_BUMPTZI