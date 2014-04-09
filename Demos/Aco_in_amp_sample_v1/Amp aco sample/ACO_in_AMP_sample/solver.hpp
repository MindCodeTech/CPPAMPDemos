// source: solver.hpp
// use:	   Defines an inheritance hierarchy for a C++ AMP TSP solver.
//		   Defines a simple solver based on CECILIA et al. (2012) "Enhancing Data
//		   Parallelism for Ant Colony Optimisation on GPUs"
// author: Alex Voicu
// date:   29/06/2012

#pragma once
#ifndef _SOLVER_HPP_BUMPTZI
#define _SOLVER_HPP_BUMPTZI

#include <amp.h>			 // For concurrency::accelerator, concurrency::array and concurrency::array_view.
#include <amp_graphics.h>	 // For concurrency::graphics::texture.
#include "cmd_line_menu.hpp"
#include "init_structs.hpp"

namespace pbl { struct Tsp_container; } // Forward declaration.

namespace slv
{
	class Solver {
	public:
		virtual void set_acc(void) = 0;
		virtual void set_params(void) = 0;
		virtual void solve(const pbl::Tsp_container& tsp) = 0;
		virtual void solve(const std::vector<pbl::Tsp_container>& tsps) = 0;

		virtual ~Solver(void) { };
	};

	// Struct for amalgamating all the data reqs of the solver.
	template<typename T>
	struct Amp_aco_solver_data  {
		// Containers.
		std::unique_ptr<concurrency::graphics::texture<T, 2>> cost_mtx;
		std::unique_ptr<concurrency::array<T, 2>> pheromones;
		std::unique_ptr<concurrency::array<T, 2>> heuristics;
		std::unique_ptr<concurrency::array<T, 2>> choice_inf;
		std::unique_ptr<concurrency::array<unsigned int, 2>> tours;
		std::unique_ptr<concurrency::array<T, 1>> tour_costs;
		// Views to const.
		std::unique_ptr<concurrency::array_view<const T, 2>> pheromones_avc;
		std::unique_ptr<concurrency::array_view<const T, 2>> heuristics_avc;
		std::unique_ptr<concurrency::array_view<const T, 2>> choice_inf_avc;
		std::unique_ptr<concurrency::array_view<unsigned int, 2>> tours_avc;
		std::unique_ptr<concurrency::array_view<T, 1>> tour_costs_avc;
		// Views.
		std::unique_ptr<concurrency::array_view<T, 2>> pheromones_av;
		std::unique_ptr<concurrency::array_view<T, 2>> heuristics_av;
		std::unique_ptr<concurrency::array_view<T, 2>> choice_inf_av;
		std::unique_ptr<concurrency::array_view<unsigned int, 2>> tours_av;
		std::unique_ptr<concurrency::array_view<T, 1>> tour_costs_av;
	};

	// Actual Aco solver.
	class Amp_aco_solver : public Solver {
	public:
		Amp_aco_solver(void) : acc(), alpha(1), beta(2), rho(0.5) { }; // Reasonable defaults for ACO params as suggested in the paper.

		virtual void set_acc(void) override;
		virtual void set_params(void) override;
		virtual void solve(const pbl::Tsp_container& tsp) override;
		virtual void solve(const std::vector<pbl::Tsp_container>& tsps) override;
	private:
		void create_buffers(const pbl::Tsp_container& tsp); 
		void bind_array_views(void);

		void fill_pher_mtx(void);
		void fill_heur_mtx(void);

		void update_choice_info(void);
		
		void construct_tour_lds(unsigned int pbl_sz); // Used to hide the ugly sizing of the LDS.

		template<unsigned int tour_lds_sz, typename Dom_t> 
		void construct_tour(const Dom_t& domain);
		
		void update_pheromones(void);

		unsigned int calc_min_cost(void) const;

		Amp_aco_solver_data<float> data;
		concurrency::accelerator acc;

		double alpha;
		double beta;
		double rho;
	};
} // Namespace slv
#endif // _SOLVER_HPP_BUMPTZI
