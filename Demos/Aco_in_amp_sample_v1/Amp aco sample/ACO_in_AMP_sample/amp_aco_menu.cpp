// source: amp_aco_menu.cpp
// use:    Implements Amp_aco_menu class.
// author: Alex Voicu
// date:   29/06/2012

#include "amp_aco_menu.hpp"
#include "helper_functions.hpp"
#include "loader.hpp"
#include "problem.hpp"
#include "solver.hpp"

using namespace clm;
using namespace std;

Amp_aco_menu::Amp_aco_menu(const ini::App_menu_init& init, ldr::Loader& ldr, slv::Solver& slv)
	: top_menu(init.top_menu_txt, init.top_menu_sz), solve_menu(init.slv_menu_txt, init.slv_menu_sz), ldr_ref(ldr), slv_ref(slv)
{
	ldr_ref.init_menu(top_menu);
}

void Amp_aco_menu::init_menu(void)
{
	// Define lambda wrappers for members.
	const auto show_ldr_menu = [this]( ) { ldr_ref.show_menu(); };
	const auto slv_set_acc = [this]( ) { slv_ref.set_acc(); };
	const auto slv_set_prm = [this]( ) { slv_ref.set_params(); };
	const auto slv_solv_sg = [this]( ) { slv_ref.solve(ldr_ref.get_single_tsp()); };
	const auto slv_solv_mult = [this]( ) { slv_ref.solve(ldr_ref.get_multi_tsp()); };
		
	auto sysret_lmb = [ ]( ) { hlp::ret_to_syst(); };
	// Set options for top_menu:
	top_menu.set_option(unique_ptr<Option>(new Action("Load TSP(s) from file(s)", show_ldr_menu, &top_menu)));
	top_menu.set_option(unique_ptr<Option>(new Action("Solve TSP(s) and output solution(s)", hlp::clear_scr, &solve_menu)));
	top_menu.set_option(unique_ptr<Option>(new Action("Return to system", sysret_lmb)));
	// Set options for solve_menu:
	solve_menu.set_option(unique_ptr<Option>(new Action("Set accelerator for ACO solver", slv_set_acc, &solve_menu)));
	solve_menu.set_option(unique_ptr<Option>(new Action("Set parameters for ACO solver", slv_set_prm, &solve_menu)));
	solve_menu.set_option(unique_ptr<Option>(new Action("Solve single TSP", slv_solv_sg, &solve_menu)));
	solve_menu.set_option(unique_ptr<Option>(new Action("Solve multiple TSPs", slv_solv_mult, &solve_menu)));
	solve_menu.set_option(unique_ptr<Option>(new Action("Return to previous menu.", hlp::clear_scr, &top_menu)));
	// Show the menu.
	top_menu.activate();
}







