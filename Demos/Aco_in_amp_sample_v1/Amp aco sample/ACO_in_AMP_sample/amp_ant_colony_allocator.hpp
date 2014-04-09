// source: amp_ant_colony_allocator.hpp
// use:	   Defines and declares a Ant_colony_allocator class template, which takes care of sizing LDS chunks.
//		   It tries to approximately hide the lack of run-time allocation of LDS. It is a bit kludgy.
// author: Alex Voicu
// date:   29/06/2012

#pragma once
#ifndef _AMP_ANT_COLONY_ALLOCATOR_HPP
#define _AMP_ANT_COLONY_ALLOCATOR_HPP

namespace ant
{
	template<typename Tour_t, typename Queen_t>
	struct Ant_colony_allocator {
		static const unsigned int lds_per_cu = 32768; // 32KB of LDS per compute unit exposed by DX.
		// Fixed LDS increments, applying a 2 exponential growth strategy.
		// Using 1 as the third argument to the ternary meanst that all template instatiations will compile fine, 0 would kill that.
		// Reasonably ugly and non granular. Also inexact by at least 1 sizeof(Tour_t) in the present implementation.
		static const unsigned int lds_32    = (32    > sizeof(Queen_t)) ? (32    - sizeof(Queen_t)) / sizeof(Tour_t) : 1; // 32B.
		static const unsigned int lds_64    = (64    > sizeof(Queen_t)) ? (64    - sizeof(Queen_t)) / sizeof(Tour_t) : 1; // 64B.
		static const unsigned int lds_128   = (128   > sizeof(Queen_t)) ? (128   - sizeof(Queen_t)) / sizeof(Tour_t) : 1; // 128B.
		static const unsigned int lds_256   = (256   > sizeof(Queen_t)) ? (256   - sizeof(Queen_t)) / sizeof(Tour_t) : 1; // 256B.
		static const unsigned int lds_512   = (512   > sizeof(Queen_t)) ? (512   - sizeof(Queen_t)) / sizeof(Tour_t) : 1; // 512B.
		static const unsigned int lds_1024  = (1024  > sizeof(Queen_t)) ? (1024  - sizeof(Queen_t)) / sizeof(Tour_t) : 1; // 1024B.
		static const unsigned int lds_2048  = (2048  > sizeof(Queen_t)) ? (2048  - sizeof(Queen_t)) / sizeof(Tour_t) : 1; // 2048B.
		static const unsigned int lds_4096  = (4096  > sizeof(Queen_t)) ? (4096  - sizeof(Queen_t)) / sizeof(Tour_t) : 1; // 4096B.
		static const unsigned int lds_8192  = (8192  > sizeof(Queen_t)) ? (8192  - sizeof(Queen_t)) / sizeof(Tour_t) : 1; // 8192B.
		static const unsigned int lds_16384 = (16384 > sizeof(Queen_t)) ? (16384 - sizeof(Queen_t)) / sizeof(Tour_t) : 1; // 16384B.
		static const unsigned int lds_32768 = (32768 > sizeof(Queen_t)) ? (32768 - sizeof(Queen_t)) / sizeof(Tour_t) : 1; // 32768B.
	};
} // Namespace ant.
#endif // _AMP_ANT_COLONY_ALLOCATOR_HPP