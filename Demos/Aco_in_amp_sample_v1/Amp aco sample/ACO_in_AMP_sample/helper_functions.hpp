// source: helper_functions.hpp
// use:	   Declares useful helpers for UI processing. Defines inline functions.
//		   Tailored for AMP_ant_colony_optimization app, not very general purpose.
// author: Alex Voicu
// date:   29/06/2012

#pragma once
#ifndef _HELPER_FUNCTIONS_HPP_BUMPTZI
#define _HELPER_FUNCTIONS_HPP_BUMPTZI

#include <amp.h>				 // For concurrency::tile_barrier.
#include <amp_math.h>			 // For concurrency::fast_math::sqrt.
#include <cstdlib>				 // For system and exit.
#include <fstream>				 // For std::ifstream.
#include <functional>			 // For std::binary_function.
#include <filesystem>			 // For std::tr2::sys::directory_entry, std::tr2::sys::path, std::tr2::sys::is_directory and std::tr2::sys::directory_iterator.
#include <iostream>				 // For std::cout and std::cin.
#include <iterator>				 // For std::istreambuf_iterator.
#include <regex>				 // For std::regex.
#include <vector>				 // For std::vector.
#include "exception_classes.hpp"

namespace pbl
{
	struct Tsp_container; // Forward declaration.
}

namespace hlp
{
	// An enum for getting keys.
	const enum class In_key : char { Up = 'W', Down = 'S', Enter = '\r' };
	// Exit to system.
	inline void ret_to_syst(void) { std::cout << "\nExit to system\n"; exit(EXIT_SUCCESS); };
	// Clear screen.
	inline void clear_scr(void) { system("cls"); }; // This is a bit evil, but useful
	// Get keyboard input.
	In_key get_kbd_in(void);
	// Move cursor based on input.
	unsigned int move_cursor(In_key cmd, unsigned int max, unsigned int cur_pos);
	// Parse a directory's contents.
	std::vector<std::tr2::sys::directory_entry> parse_dir(const std::tr2::sys::path& dir_path);
	// Print valid contents out to std::cout.
	void print_dir(const std::vector<std::tr2::sys::directory_entry>& dir_contents, unsigned int choice);
	// Windows 8 only helper for disabling TDR.
	concurrency::accelerator disable_tdr(const concurrency::accelerator& in_acc);
	// Open a file, load it into a string for manipulation.
	template<typename T> 
	std::string file_to_str(const T& file_pth)
	{
		std::ifstream in_file(file_pth);
		
		if (!in_file) {
			throw exc::File_err_ex("hlp::file_to_str<T>(const T&)", file_pth);
		}

		return std::string(std::istreambuf_iterator<char>(in_file), std::istreambuf_iterator<char>());
	}
	
	// Simple functors to use in an AMP context.
	// Parallel_scan, which assumes data is already in LDS and is POT. Just a primitive, really.
	template<typename T, typename Bin_op>
	T amp_scan(T* const beg_ptr, T* const end_ptr, unsigned int idx, concurrency::tile_barrier& barrier, Bin_op op) restrict(amp)
	{
		if (end_ptr <= beg_ptr) {
			//direct3d_errorf("Empty / invalid interval passed to hlp::amp_scan<T, Bin_op>(T*, T*, unsigned int, Bin_op).\n"); Maybe this will work some day.
			return concurrency::fast_math::sqrt(T(-1));
		}
		else {
			unsigned int half_width = (end_ptr - beg_ptr) / 2;
			for (auto i = half_width; i != 0u; i /= 2) { // Could also be done in reverse, and the div replaced by rightshift.
				if (idx < i) {
					beg_ptr[idx] = op(beg_ptr[idx], beg_ptr[idx + i]);
				}
				barrier.wait_with_tile_static_memory_fence();
			}
			return beg_ptr[idx];
		}
	}
	// Plus<T>
	template<typename T>
	struct Amp_plus : public std::binary_function<T, T, T> {
		T operator()(const T& lhs, const T& rhs) restrict(amp) { return  lhs + rhs; };
	};
	// Max<T>
	template<typename T>
	struct Amp_max : public std::binary_function<T, T, T> {
		const T& operator()(const T& lhs, const T& rhs) restrict(amp) { return (lhs < rhs) ? rhs : lhs; };
	};
} // Namespace hlp
#endif // _HELPER_FUNCTIONS_HPP_BUMPTZI