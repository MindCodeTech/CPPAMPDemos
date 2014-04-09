// source: problem.hpp
// use:	   Defines a simple class inheritance hierarchy
//		   for representing a TSP
// author: Alex Voicu
// date:   29/06/2012
#pragma once
#ifndef _PROBLEM_HPP_BUMPTZI
#define _PROBLEM_HPP_BUMPTZI

#include <regex>			   // For std::smatch, std::regex and std::regex_search.
#include <string>			   // For std::string.
#include <vector>			   // For std::vector.
#include "init_structs.hpp"

namespace pbl
{
	struct Tsp_container {
		std::vector<float> pbl_adj_mtx;
		std::string problem_nam;
		std::string problem_typ;
		std::string edg_wgt_typ;
		std::string edg_wgt_fmt;
		unsigned int problem_dim;

		// bparadie, 2012-09-10: holds the coordinates
		// see read_func_data()
		std::vector<float> pbl_coords;
	};

	// Parse a valid .tsp file, return a TSP container
	Tsp_container parse_tsp(const std::string& in_work_str);
	// Extract tsp params from loaded problem in string form.
	std::string extract_param(const std::string& where, const std::regex& what);
}

#endif // _PROBLEM_HPP_BUMPTZI