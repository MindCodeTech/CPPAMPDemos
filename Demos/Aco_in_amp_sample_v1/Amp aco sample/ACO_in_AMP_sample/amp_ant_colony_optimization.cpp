// source: amp_ant_colony_optimization.cpp
// use:    Implements ACO on GPU using C++ AMP.
// author: Alex Voicu
// date:   29/06/2012

#include <cstdlib>			// For EXIT_FAILURE and EXIT_SUCCESS.
#include <iostream>			// For std::cerr and std::cin.
#include <exception>		// For std::exception.
#include "amp_aco_menu.hpp"
#include "loader.hpp"
#include "problem.hpp"
#include "solver.hpp"

using namespace concurrency;
using namespace ini;
using namespace ldr;
using namespace slv;
using namespace std;

int main(void) 
{
	// Setup init parameters
	// App menu
	App_menu_init app_init;
	app_init.top_menu_sz = 4;
	app_init.slv_menu_sz = 5;
	app_init.top_menu_txt = "***TSP solver using AntColonyOptimization implemented in C++ AMP***";
	app_init.slv_menu_txt = "***Select accelerator for solve and execute it***";
	// Loader menu
	Ldr_menu_init ldr_init;
	ldr_init.top_menu_txt = "***Choose single/multiple problems to load***";
	ldr_init.top_menu_sz = 5;
	
	try {
		Simple_loader loader(ldr_init);
		Amp_aco_solver solver;
		Amp_aco_menu app_menu(app_init, loader, solver);
		app_menu.init_menu();
	}
	catch (exception& ex) {
		cerr << ex.what() << endl;
		cin.get();

		return EXIT_FAILURE;
	}
	catch (...) {
		cerr << "Some incredibly dangerous and utterly unknown thing messed up the poor app!" << endl;
		cin.get();

		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}