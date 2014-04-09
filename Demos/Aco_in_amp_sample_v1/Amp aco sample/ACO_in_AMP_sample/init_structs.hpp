// source: init_structs.hpp
// use:	   Defines structs for initializing components
// author: Alex Voicu
// date:   29/06/2012

#pragma once
#ifndef _INIT_STRUCTS_HPP_BUMPTZI
#define _INIT_STRUCTS_HPP_BUMPTZI

#include <amp.h>  // For concurrency::accelerator
#include <string> // For std::string
#include <vector> // For std::vector

namespace ini
{
	struct App_menu_init {
		std::string top_menu_txt;
		std::string slv_menu_txt;
		int top_menu_sz;
		int slv_menu_sz;
	};

	struct Ldr_menu_init {
		std::string top_menu_txt;
		int top_menu_sz;
	};
}// Namespace ini.
#endif // _INIT_STRUCTS_HPP_BUMPTZI