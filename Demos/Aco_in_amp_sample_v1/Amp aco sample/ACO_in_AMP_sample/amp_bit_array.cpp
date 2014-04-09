// source: amp_bit_array.cpp
// use:	   Implements Amp_bit_array class.
// author: Alex Voicu
// date:   29/06/2012

#ifndef _DEBUG
#include "amp_bit_array.hpp"

using namespace aba;
using namespace concurrency;
#endif

void Amp_bit_array::set(unsigned int pos) restrict(amp)
{
	if (pos >= max_bits) {
		//direct3d_errorf("Amp_bit_array::set(unsigned int) passed index is out of range\n"); Maybe this will work some day.
		return;
	}
	else {
		const auto word = pos / bits_per_word;
		const auto bit = pos % bits_per_word;
		const auto mask = bit_mask << bit; // Put one in position to be set.

		switch (word) { 
		case 0:
			data.x |= mask;
			break;
		case 1:
			data.y |= mask;
			break;
		case 2:
			data.z |= mask;
			break;
		case 3:
			data.w |= mask;
			break;
		default:
			return; // Should not get here.
		}
	}
}

void Amp_bit_array::clear(unsigned int pos) restrict(amp)
{
	if (pos >= max_bits) {
		//direct3d_errorf("Amp_bit_array::clear(unsigned int) passed index is out of range\n"); Maybe this will work some day.
		return;
	}
	else {
		const auto word = pos / bits_per_word;
		const auto bit = pos % bits_per_word;
		const auto mask = ~(bit_mask << bit); // Zero out position to be cleared.

		switch (word) {
		case 0:
			data.x &= mask;
			break;
		case 1:
			data.y &= mask;
			break;
		case 2:
			data.z &= mask;
			break;
		case 3:
			data.w &= mask;
			break;
		default:
			return; // Should not get here.
		}
	}
}

bool Amp_bit_array::operator[](unsigned int pos) const restrict(amp)
{
	if (pos >= max_bits) {
		//direct3d_errorf("Amp_bit_array::operator[](unsigned int) passed index is out of range\n"); Maybe this will work some day.
		return false;
	}
	else {
		bool is_set = false;

		const auto word = pos / bits_per_word;
		const auto bit = pos % bits_per_word;
		const auto mask = bit_mask << bit;

		switch (word) {
		case 0:
			if (data.x & mask) {
				is_set = true;
			}
			break;
		case 1:
			if (data.y & mask) {
				is_set = true;
			}
			break;
		case 2:
			if (data.z & mask) {
				is_set = true;
			}
			break;
		case 3:
			if (data.w & mask) {
				is_set = true;
			}
			break;
		default:
			break;
		}

		return is_set;
	}
}