// source: solver.cpp
// use:	   Implements Amp_aco_solver class.
// author: Alex Voicu
// date:   29/06/2012

#include <algorithm>			// For std::min_element.
#include <amp_graphics.h>		// For concurrency::graphics::texture.
#include <amp_math.h>			// For concurrency::fast_math::pow.
#include <cstdlib>				// For std::rand.
#include <ctime>				// For std::clock.
#include <iostream>				// For std::cin and std::cout.
#include <iterator>				// For std::begin and std::end.
#include <limits>				// For std::numeric_limits and std::stream_size.
#include <random>				// For std::mt19937.
#include <type_traits>			// For std::remove_pointer.

#include "amp_ant_colony_allocator.hpp"
#include "amp_queen_ant.hpp"
#include "amp_worker_ant.hpp"
#include "helper_functions.hpp"
#include "problem.hpp"
#include "solver.hpp"

using namespace concurrency;
using namespace aba;
using namespace ant;
using namespace hlp;
using namespace pbl;
using namespace slv;
using namespace std;

void Amp_aco_solver::set_acc(void)
{
	const auto is_cpu_acc = [ ](const accelerator& acc) { return acc.device_path == accelerator::cpu_accelerator; };
	
	auto acc_vec = accelerator::get_all();
	acc_vec.erase(remove_if(begin(acc_vec), end(acc_vec),is_cpu_acc), end(acc_vec)); // Filter out cpu_accelerator and count valid accelerators.

	for (auto i = 0u; i != acc_vec.size(); ++i) {
		wcout << i << ". " << acc_vec.at(i).description << '\n';
	}
	
	cout << "Select choice: ";
	unsigned int choice = acc_vec.size() + 1;
	while (cin >> choice) {
		if (choice < acc_vec.size()) {
			acc = acc_vec.at(choice);
			break;
		}
		else {
			continue;
		}
	}
	
	acc = hlp::disable_tdr(acc);
	
	wcout << "Currently chosen accelerator: " << acc.description << endl;
}

void Amp_aco_solver::set_params(void)
{
	const auto max_stream_sz = numeric_limits<streamsize>::max();

	cout << "Input pheromone trail weight (alpha > 0),\n" <<
			"pheromone weight (beta > 0),\n" <<
			"and pheromone evaporation rate (rho > 0):";

	do {
		cin >> alpha >> rho;
		if (cin == false) {
			cin.clear();
		}
		cin.ignore(max_stream_sz, '\n'); // Clean leftovers, if any.
	} while ((alpha <= 0.0) || (beta <= 0.0) || (rho <= 0.0));
}

void Amp_aco_solver::solve(const Tsp_container& tsp)
{	
	const concurrency::extent<2> pbl_sz(tsp.problem_dim, tsp.problem_dim);
	
	create_buffers(tsp);
	bind_array_views();

	fill_pher_mtx();
	fill_heur_mtx();	

	const unsigned int max_non_min_cycles = 8; // How many cycles can be spent trapped in a local minimum / running down some tangent towards infinity.
	unsigned int non_min_cycles = 0;
	unsigned int min_cost = numeric_limits<unsigned int>::max();
	
	// Start the solving loop.
	auto start = clock();
	do {
		update_choice_info();

		construct_tour_lds(tsp.problem_dim);

		update_pheromones();

		const auto curr_min = calc_min_cost();
		if (curr_min >= min_cost) { // If we're no longer minimising, or are stuck at a minimum, increment counter. 
			++non_min_cycles;
		}
		else {
			non_min_cycles = 0;
			min_cost = curr_min;
		}
		cout << min_cost << "; "; 
	} while (non_min_cycles != max_non_min_cycles);
	auto stop = clock();

	cout << "For problem " << tsp.problem_nam << " minimum tour cost is: " << min_cost << '\n';
	cout << "Solve took " << static_cast<double>(stop - start) / CLOCKS_PER_SEC << " seconds.\n";
	cin.get();
}

void Amp_aco_solver::solve(const vector<Tsp_container>& tsps)
{
	for (const auto& problem : tsps) {
		solve(problem);
	}
}

void Amp_aco_solver::create_buffers(const Tsp_container& tsp)
{
	typedef remove_pointer<decltype(data.choice_inf.get())>::type Choice_inf_t;
	typedef remove_pointer<decltype(data.cost_mtx.get())>::type Cost_mtx_t;
	typedef remove_pointer<decltype(data.heuristics.get())>::type Heuristics_t;
	typedef remove_pointer<decltype(data.pheromones.get())>::type Pheromones_t;
	typedef remove_pointer<decltype(data.tours.get())>::type Tours_t;
	typedef remove_pointer<decltype(data.tour_costs.get())>::type Tour_costs_t;

	const concurrency::extent<2> pbl_dim(tsp.problem_dim, tsp.problem_dim);

	data.choice_inf.reset(new Choice_inf_t(pbl_dim, acc.default_view));
	data.cost_mtx.reset(new Cost_mtx_t(pbl_dim, begin(tsp.pbl_adj_mtx), end(tsp.pbl_adj_mtx), acc.default_view));
	data.heuristics.reset(new Heuristics_t(pbl_dim, acc.default_view));
	data.pheromones.reset(new Pheromones_t(pbl_dim, acc.default_view));
	data.tours.reset(new Tours_t(pbl_dim, acc.default_view));
	data.tour_costs.reset(new Tour_costs_t(pbl_dim[0], accelerator(accelerator::cpu_accelerator).default_view, acc.default_view)); // Tour costs should be a staging buffer since we check convergence using them on CPU side.
}

void Amp_aco_solver::bind_array_views(void)
{
	typedef remove_pointer<decltype(data.choice_inf_avc.get())>::type Choice_inf_avc_t;
	typedef remove_pointer<decltype(data.heuristics_avc.get())>::type Heuristics_avc_t;
	typedef remove_pointer<decltype(data.pheromones_avc.get())>::type Pheromones_avc_t;
	typedef remove_pointer<decltype(data.tours_avc.get())>::type Tours_avc_t;
	typedef remove_pointer<decltype(data.tour_costs_avc.get())>::type Tour_costs_avc_t;

	typedef remove_pointer<decltype(data.choice_inf_av.get())>::type Choice_inf_av_t;
	typedef remove_pointer<decltype(data.heuristics_av.get())>::type Heuristics_av_t;
	typedef remove_pointer<decltype(data.pheromones_av.get())>::type Pheromones_av_t;
	typedef remove_pointer<decltype(data.tours_av.get())>::type Tours_av_t;
	typedef remove_pointer<decltype(data.tour_costs_av.get())>::type Tour_costs_av_t;

	// Views to const.
	data.choice_inf_avc.reset(new Choice_inf_avc_t(*data.choice_inf));
	data.heuristics_avc.reset(new Heuristics_avc_t(*data.heuristics));
	data.pheromones_avc.reset(new Pheromones_avc_t(*data.pheromones));
	data.tours_avc.reset(new Tours_avc_t(*data.tours));
	data.tour_costs_avc.reset(new Tour_costs_avc_t(*data.tour_costs));
	// Views.
	data.choice_inf_av.reset(new Choice_inf_av_t(*data.choice_inf));
	data.heuristics_av.reset(new Heuristics_av_t(*data.heuristics));
	data.pheromones_av.reset(new Pheromones_av_t(*data.pheromones));
	data.tours_av.reset(new Tours_av_t(*data.tours));
	data.tour_costs_av.reset(new Tour_costs_av_t(*data.tour_costs));
}


void Amp_aco_solver::fill_pher_mtx(void)
{
	typedef remove_pointer<decltype(data.pheromones.get())>::type::value_type Value_t;
	auto pheromones_av = *data.pheromones_av;

	parallel_for_each(acc.default_view, pheromones_av.extent, [=](index<2> idx) restrict(amp) { // All paths are equal at init.
		pheromones_av(idx) = Value_t(1); 
	});
}

void Amp_aco_solver::fill_heur_mtx(void)
{
	typedef remove_pointer<decltype(data.heuristics.get())>::type::value_type Value_t;
	
	const Value_t beta(this->beta); // Unclean magic to use it in the lambda.
	const Value_t eps = numeric_limits<Value_t>::epsilon();
	
	auto& heuristics = *data.heuristics;
	auto& cost_mtx = *data.cost_mtx;

	parallel_for_each(acc.default_view, heuristics.extent, [=, &heuristics, &cost_mtx](index<2> idx) restrict(amp) {
		if (idx[0] == idx[1]) { // Make diagonal (self - self dist) elements 0.
			heuristics(idx) = Value_t(0);
		}
		else {
			heuristics(idx) = (cost_mtx(idx) <= eps) ? Value_t(1) : fast_math::pow(Value_t(1) / cost_mtx(idx), beta); // If cost is hyper-small, set prob to 1.
		}
	});
}

void Amp_aco_solver::update_choice_info(void) 
{
	typedef remove_pointer<decltype(data.heuristics.get())>::type::value_type Value_t;
	
	const Value_t alpha(this->alpha);

	const auto heuristics_avc = *data.heuristics_avc;
	const auto pheromones_avc = *data.pheromones_avc;
	auto choice_info_av = *data.choice_inf_av;

	choice_info_av.discard_data();
	parallel_for_each(acc.default_view, choice_info_av.extent, [=](index<2> idx) restrict(amp) {
		choice_info_av(idx) = fast_math::pow(pheromones_avc(idx), alpha) * heuristics_avc(idx); 
	});
}

inline void Amp_aco_solver::construct_tour_lds(unsigned int pbl_sz)
{
	typedef remove_pointer<decltype(data.choice_inf.get())>::type::value_type Value_t;
	// We set tile size along the Y axis to equal 1 so that we get one group per city (row).
	// Tile size along the X axis must be a power of two, otherwise the parallel_scan will fumble. TODO fix.
	static const int tile_sz_x = 64; // Quite ugly. 
	static const int tile_sz_y = 1;  // Quite ugly.
	
	typedef Ant_colony_allocator<unsigned int, Queen_ant<Value_t, tile_sz_x, 1>> Col_alloc; // Use this to size LDS properly.

	// The following is utter ugliness.
	concurrency::extent<2> patched_ext(data.tour_costs->extent[0], tile_sz_x); // We do it like this so that we get city_count tiles, each associated with a queen-ant.
	const auto tiled_ext = patched_ext.tile<tile_sz_y, tile_sz_x>().pad();

	if (pbl_sz <= Col_alloc::lds_32) {
		construct_tour<Col_alloc::lds_32>(tiled_ext);
	}
	else if (pbl_sz <= Col_alloc::lds_64) {
		construct_tour<Col_alloc::lds_64>(tiled_ext);
	}
	else if (pbl_sz <= Col_alloc::lds_128) {
		construct_tour<Col_alloc::lds_128>(tiled_ext);
	}
	else if (pbl_sz <= Col_alloc::lds_256) {
		construct_tour<Col_alloc::lds_256>(tiled_ext);
	}
	else if (pbl_sz <= Col_alloc::lds_512) {
		construct_tour<Col_alloc::lds_512>(tiled_ext);
	}
	else if (pbl_sz <= Col_alloc::lds_1024) {
		construct_tour<Col_alloc::lds_1024>(tiled_ext);
	}
	else if (pbl_sz <= Col_alloc::lds_2048) {
		construct_tour<Col_alloc::lds_2048>(tiled_ext);
	}
	else if (pbl_sz <= Col_alloc::lds_4096) {
		construct_tour<Col_alloc::lds_4096>(tiled_ext);
	}
	else if (pbl_sz <= Col_alloc::lds_8192) {
		construct_tour<Col_alloc::lds_8192>(tiled_ext);
	}
	else if (pbl_sz <= Col_alloc::lds_16384) {
		construct_tour<Col_alloc::lds_16384>(tiled_ext);
	}
	else if (pbl_sz <= Col_alloc::lds_32768) {
		construct_tour<Col_alloc::lds_32768>(tiled_ext);
	}
	else {
		throw std::runtime_error("Problem size is too large for this solver!");
	}
}

template<unsigned int tour_lds_sz, typename Dom_t> 
void Amp_aco_solver::construct_tour(const Dom_t& domain)
{
	typedef remove_pointer<decltype(data.cost_mtx.get())>::type Cost_mtx_t;
	typedef tiled_index<domain.tile_dim0, domain.tile_dim1> Idx_t;
	typedef remove_pointer<decltype(data.choice_inf.get())>::type::value_type Value_t;

	const auto choice_info_avc = *data.choice_inf_avc;
	const auto& costs_tex = *data.cost_mtx;
	auto pheromones_av = *data.pheromones_av;
	auto tours_av = *data.tours_av;
	auto tour_costs_av = *data.tour_costs_av;
	
	static mt19937 cpu_prng;
	const unsigned int rand_seed = cpu_prng(); // Generate seeds for the on-GPU pRNG.
	
	tours_av.discard_data(); // Mostly for documentation, underline the idea that we're thrashing these in the processing loop
	tour_costs_av.discard_data();
	parallel_for_each(acc.default_view, domain, [=, &costs_tex](Idx_t tidx) restrict(amp) {
		tile_static Queen_ant<Value_t, tidx.tile_dim1, tour_lds_sz> queen;
		register Worker_ant<Value_t, tidx.tile_dim1> workers(tidx, choice_info_avc, rand_seed);

		queen.init(tidx, choice_info_avc.extent[0]);       // Wake up the queen, tell her how many cities she has to visit (== count of rows in choice info / tours).
		workers.mark_visited(tidx.tile[0]);                // Mark the place of birth as visited.

		const unsigned int cities_left = domain[0] - 1;
		for (auto i = 0u; i != cities_left; ++i) {  
			queen.find_best_cand(workers);

			queen.go_to_next(workers);
		}
		queen.export_costs(workers, costs_tex, tour_costs_av); // Write out the cost of the tour.

		queen.export_tour(workers, tours_av);		           // Write out the tour itself.
	});
}

void Amp_aco_solver::update_pheromones(void)
{
	typedef remove_pointer<decltype(data.tour_costs.get())>::type::value_type Value_t;
	
	const auto pheromones_av = *data.pheromones_av;
	const auto tours_avc = *data.tours_avc;
	const auto tour_costs_avc = *data.tour_costs_avc;
	const Value_t rho(this->rho);
	
	static const int tile_sz_x = 16; // Quite ugly.
	static const int tile_sz_y = 16; // Quite ugly.
	const auto tiled_ext = pheromones_av.extent.tile<tile_sz_y, tile_sz_x>().pad();
	
	parallel_for_each(acc.default_view, tiled_ext, [=](tiled_index<tile_sz_y, tile_sz_x> tidx) restrict(amp)
	{
		// We put tours in a 1D array of edges (defined by 2 nodes) in the LDS, with each "row" corresponding to a tour.
		tile_static index<2> tours_lds[tidx.tile_dim0 * tidx.tile_dim1]; // This is where we'll store edges.
		tile_static Value_t tour_qual_lds[tidx.tile_dim0];				 // This is where we'll store their quality.

		register Value_t pherom_to_add(0); // Since each thread discretely owns a particular edge, we can determine the pheromone to add quantity in registers.
		
		tours_lds[tidx.local[0] * tidx.tile_dim1 + tidx.local[1]] = index<2>(tours_avc.extent[0], tours_avc.extent[1]);
		if (tidx.local[1] == 0) {
			tour_qual_lds[tidx.local[0]] = Value_t(0);
		}
		tidx.barrier.wait_with_tile_static_memory_fence();
		
		for (auto i = 0u; i < tours_avc.extent[0]; i += tidx.tile_dim0) {
			for (auto j = 0u; j < tours_avc.extent[1]; j += tidx.tile_dim1) {
				const index<2> first(tidx.global[0] + i, tidx.local[1] + j); // Row index == global index + offset, col index = local index + offset; this way each group starts on col 0 and slides along row.
				const auto second = ((first[1] + 1) == tours_avc.extent[1]) ? index<2>(tidx.global[0], 0) : index<2>(first[0], first[1] + 1); // If we're at the end of the tour wrap back to starting node, otherwise just take next elem.

				if (tours_avc.extent.contains(first) == true) { // If the first is not contained the second won't either so we needn't check.
					tours_lds[tidx.local[0] * tidx.tile_dim1 + tidx.local[1]] = index<2>(tours_avc(first), tours_avc(second)); // Load the edge.

					if (tidx.local[1] == 0) {
						tour_qual_lds[tidx.local[0]] = Value_t(1) / tour_costs_avc(tidx.global[0]); // Calculate the quality of the tour owning the row.
					}
				}
				tidx.barrier.wait_with_tile_static_memory_fence();

				for (auto k = 0u; k != tidx.tile_dim0; ++k) {
					for (auto l = 0u; l != tidx.tile_dim1; ++l) {
						if (tours_lds[k * tidx.tile_dim1 + l] == tidx.global) {
							pherom_to_add += tour_qual_lds[k];
						}
					}
				}
				tidx.barrier.wait_with_tile_static_memory_fence(); // Make sure nobody starts reading from LDS.
			}
		}
		if (pheromones_av.extent.contains(tidx.global) == true) {
			pheromones_av(tidx.global) = direct3d::mad((1 - rho), pheromones_av(tidx.global), pherom_to_add); // (1 - rho) * pherom_0 + delta_pherom.  
		}
	});
}

unsigned int Amp_aco_solver::calc_min_cost(void) const
{
	const auto first = data.tour_costs_avc->data();
	const auto last = data.tour_costs_avc->data() + data.tour_costs_avc->extent[0];

	return static_cast<unsigned int>(*min_element(first, last));
}
	


	