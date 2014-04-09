// source: amp_aco_menu.hpp
// use:	   Defines a menu class for AMP_ant_colony_optimization application
// author: Alex Voicu
// date:   29/06/2012

#pragma once
#ifndef _AMP_ACO_MENU_HPP_BUMPTZI
#define _AMP_ACO_MENU_HPP_BUMPTZI

#include "cmd_line_menu.hpp"

namespace ini { struct App_menu_init; } // Forward declaration.

namespace ldr { class Loader; } // Forward declaration.

namespace slv { class Solver; } // Forward declaration.

class Amp_aco_menu {
public:
	Amp_aco_menu(const ini::App_menu_init& init, ldr::Loader& ldr, slv::Solver& slv);

	void init_menu(void);
private:
	Amp_aco_menu(void);                                 // No default construction
	Amp_aco_menu(const Amp_aco_menu& other);            // No copy construction
	Amp_aco_menu& operator=(const Amp_aco_menu& other); // No copy assignment

	clm::Menu top_menu;
	clm::Menu solve_menu;

	ldr::Loader& ldr_ref;
	slv::Solver& slv_ref;
};

#endif // _AMP_ACO_MENU_HPP_BUMPTZI