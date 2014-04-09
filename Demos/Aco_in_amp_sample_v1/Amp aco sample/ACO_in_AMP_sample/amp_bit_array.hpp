// source: amp_bit_array.hpp
// use:	   Declares a bit array class, useable in a restrict(amp) context; based on Chapter 11.4 from
//   	   D. M. Capper "Introducing C++ for Scientists, Engineers and Mathematicians 2nd edition"
// author: Alex Voicu
// date:   29/06/2012

#pragma once
#ifndef _AMP_BIT_ARRAY_HPP_BUMPTZI
#define _AMP_BIT_ARRAY_HPP_BUMPTZI

#include <amp_short_vectors.h> // For concurrency::graphics::uint_4.
#include <climits>			   // For UINT_MAX and CHAR_BIT.

namespace aba
{
	class Amp_bit_array {
	public:
		Amp_bit_array(bool set_all = false) restrict(amp) : data(set_all ? UINT_MAX : 0) { };

		void set(unsigned int pos) restrict(amp);
		void clear(unsigned int pos) restrict(amp);
		void set(void) restrict(amp) { data = UINT_MAX; };
		void clear(void) restrict(amp) { data = 0; };
		bool operator[](unsigned int pos) const restrict(amp);
		unsigned int length(void) restrict(amp) { return max_bits; };

	private:
		concurrency::graphics::uint_4 data; // This gives us up to 128 bits per bit_array, enough for our ACO use case.
		
		static const unsigned int max_bits = CHAR_BIT * sizeof(concurrency::graphics::uint_4);
		static const unsigned int bits_per_word = CHAR_BIT * sizeof(concurrency::graphics::uint_4::value_type);
		static const unsigned int bit_mask = 1;
	};
#ifdef _DEBUG
using namespace concurrency;
#include "amp_bit_array.cpp"
#endif
} // Namespace aba.
#endif // _AMP_BIT_ARRAY_HPP_BUMPTZI
