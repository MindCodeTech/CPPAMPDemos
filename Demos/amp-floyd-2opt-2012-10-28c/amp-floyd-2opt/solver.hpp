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

} // Namespace slv
#endif // _SOLVER_HPP_BUMPTZI
