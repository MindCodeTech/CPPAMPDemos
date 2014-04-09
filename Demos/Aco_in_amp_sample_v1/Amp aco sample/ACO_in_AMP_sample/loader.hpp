// source: loader.hpp
// use:	   Defines an inheritance hierarchy for TSP specification loaders.
// author: Alex Voicu
// date:   29/06/2012

#pragma once
#ifndef _LOADER_HPP_BUMPTZI
#define _LOADER_HPP_BUMPTZI

#include <exception>  // For std::logic_error.
#include <filesystem> // For std::tr2::sys::path, std::tr2::sys::is_directory and std::tr2::sys::directory_iterator
#include <map>		  // For std::map.
#include <string>	  // For std::string.
#include <vector>	  // For std::vector.
#include "cmd_line_menu.hpp"
#include "helper_functions.hpp"
#include "problem.hpp"

namespace ini { struct Ldr_menu_init; } // Forward declaration.

namespace ldr
{
	class Loader {
	public:
		virtual void init_menu(clm::Option& return_to) = 0;
		virtual void show_menu(void) const = 0;
		virtual pbl::Tsp_container get_single_tsp(void) const = 0;
		virtual std::vector<pbl::Tsp_container> get_multi_tsp(void) const = 0;

		virtual ~Loader(void) { };
	};

	class Simple_loader : public Loader {
	public:
		explicit Simple_loader(const ini::Ldr_menu_init& init);

		virtual void init_menu(clm::Option& return_to) override;
		virtual void show_menu(void) const override;
		pbl::Tsp_container get_single_tsp(void) const override;
		std::vector<pbl::Tsp_container> get_multi_tsp(void) const override;
	private:
		std::map<std::string, std::tr2::sys::path> pbl_to_load;
		clm::Menu top_menu;
		std::string sg_solve_tgt;
		
		void single_load(void);
		void multi_load(void);
		bool process_choice(std::tr2::sys::path chosen_path);
	};

	inline void Simple_loader::show_menu(void) const
	{
		hlp::clear_scr();
		top_menu.activate();
	}

	inline pbl::Tsp_container Simple_loader::get_single_tsp(void) const
	{
		// Call parser, return result.
		if (sg_solve_tgt == "") {
			throw std::logic_error("Tried calling Simple_loader::get_single_tsp() with no TSPs in queue");
		}
		return pbl::parse_tsp(hlp::file_to_str(pbl_to_load.at(sg_solve_tgt).relative_path().string()));
	}
} // namespace ldr
#endif // _LOADER_HPP_BUMPTZI