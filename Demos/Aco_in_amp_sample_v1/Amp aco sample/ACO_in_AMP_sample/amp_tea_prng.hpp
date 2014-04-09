// source: amp_tea_prng.hpp
// use:	   Defines and declares a simple class template implementing the crypto pRNG proposed
//   	   by Olano et. al in http://www.csee.umbc.edu/~olano/papers/GPUTEA.pdf.
// author: Alex Voicu
// date:   29/06/2012

#pragma once
#ifndef _AMP_TEA_PRNG_BUMPTZI
#define _AMP_TEA_PRNG_BUMPTZI

#include <amp_short_vectors.h> // For concurrency::graphics::uint_2 and concurrency::graphics::uint_4.
#include <climits>			   // For UINT_MAX.

namespace rng
{
	typedef concurrency::graphics::uint_4 Uint_4;
	typedef concurrency::graphics::uint_2 Uint_2;
	typedef concurrency::index<2> Idx_2;

	// Magic numbers from paper/research listed for reference, as AMP doesn't like statics / globals.
	static const Uint_4 tea_keys = Uint_4(0xA341316C, 0xC8013EA4, 0xAD90777D, 0x7E95761E);

	template<typename T> 
	class Amp_tea_prng {
	public:
		typedef typename concurrency::graphics::short_vector<T, 2>::type T_2;

		explicit Amp_tea_prng(const Uint_2 seed) restrict(amp) 
			: keys(0xA341316C, 0xC8013EA4, 0xAD90777D, 0x7E95761E), seeds(seed) { };
		explicit Amp_tea_prng(const Idx_2 seed) restrict(amp) 
			: keys(0xA341316C, 0xC8013EA4, 0xAD90777D, 0x7E95761E), seeds(seed[0], seed[1]) { };

		T_2 operator()(const unsigned int rounds = 16) restrict(amp);
	private:
		const Uint_4 keys;
	    Uint_2 seeds;
		static const unsigned int delta = 0x9E3779B9;
	};

	template<typename T> 
	typename Amp_tea_prng<T>::T_2 Amp_tea_prng<T>::operator()(const unsigned int rounds) restrict(amp)
	{
		register Uint_2 sum(0, 0);
		register const Uint_2 lshift(4, 4); // Shift left by 4.
		register const Uint_2 rshift(5, 5); // Shift right by 5;

		for (auto i = 0u; i != rounds; ++i) {
			sum += delta;
			seeds += ((seeds.yx << lshift) + keys.xz) ^ (seeds.yx + sum) ^ ((seeds.yx >> rshift) + keys.yw);
		}

		return (T_2(seeds) / T(UINT_MAX));
	}
} // Namespace rng.
#endif // _AMP_TEA_PRNG_BUMPTZI