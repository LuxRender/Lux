/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 *   Lux Renderer is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Lux Renderer is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

// Tausworthe (taus113) random numbergenerator by radiance
// Based on code from GSL (GNU Scientific Library)
// MASK assures 64bit safety

// Usage in luxrender:
// lux::random::floatValue() returns a random float
// lux::random::uintValue() returns a random uint
//
// NOTE: calling random values outside of a renderthread will result in a crash
// thread safety/uniqueness using thread specific ptr (boost)
// Before renderthreads execute, use floatValueP() and uintValueP() instead.

#ifndef LUX_RANDOM_H
#define LUX_RANDOM_H

#include "memory.h"
#include <boost/noncopyable.hpp>

#define MASK 0xffffffffUL
#define FLOATMASK 0x00ffffffUL

#define RAN_BUFFER_AMOUNT 2048

static const float invUI = (1.f / (FLOATMASK + 1UL));

namespace lux
{

class RandomGenerator : boost::noncopyable
{
public:
	RandomGenerator() {
		buf = lux::AllocAligned<unsigned long>(RAN_BUFFER_AMOUNT);
		bufid = RAN_BUFFER_AMOUNT;
	}
	RandomGenerator(unsigned long tn) {
		buf = lux::AllocAligned<unsigned long>(RAN_BUFFER_AMOUNT);
		bufid = RAN_BUFFER_AMOUNT;
		taus113_set(tn);
	}

	~RandomGenerator() { lux::FreeAligned(buf); }

	inline void init(unsigned long tn) {
		taus113_set(tn);
	}

	inline unsigned long uintValue() const {
		// Repopulate buffer if necessary
		const unsigned int offset = bufid; // for thread safety
		if (offset >= RAN_BUFFER_AMOUNT) {
			for(int i = 0; i < RAN_BUFFER_AMOUNT; ++i)
				buf[i] = nobuf_generateUInt();
			bufid = 1;
			return buf[0];
		}
		bufid = offset + 1;
		return buf[offset];
	}

	inline float floatValue() const {
		return (uintValue() & FLOATMASK) * invUI; 
	}

private:
	inline unsigned long LCG(const unsigned long n) {
		return 69069UL * n; // The result is clamped to 32 bits (long)
	}
	void taus113_set(unsigned long s) {
		if (!s)
			s = 1UL; // default seed is 1

		z1 = LCG(s);
		if (z1 < 2UL)
			z1 += 2UL;
		z2 = LCG (z1);
		if (z2 < 8UL)
			z2 += 8UL;
		z3 = LCG (z2);
		if (z3 < 16UL)
			z3 += 16UL;
		z4 = LCG (z3);
		if (z4 < 128UL)
			z4 += 128UL;

		// Calling RNG ten times to satify recurrence condition
		for(int i = 0; i < 10; ++i)
			nobuf_generateUInt();
	}

	inline unsigned long nobuf_generateUInt() const {
		const unsigned long b1 = ((((z1 << 6UL) & MASK) ^ z1) >> 13UL);
		z1 = ((((z1 & 4294967294UL) << 18UL) & MASK) ^ b1);

		const unsigned long b2 = ((((z2 << 2UL) & MASK) ^ z2) >> 27UL);
		z2 = ((((z2 & 4294967288UL) << 2UL) & MASK) ^ b2);

		const unsigned long b3 = ((((z3 << 13UL) & MASK) ^ z3) >> 21UL);
		z3 = ((((z3 & 4294967280UL) << 7UL) & MASK) ^ b3);

		const unsigned long b4 = ((((z4 << 3UL) & MASK) ^ z4) >> 12UL);
		z4 = ((((z4 & 4294967168UL) << 13UL) & MASK) ^ b4);

		return (z1 ^ z2 ^ z3 ^ z4);
	}

	mutable unsigned long z1, z2, z3, z4;
	unsigned long *buf;
	mutable int bufid;
};

namespace random {

static RandomGenerator PGen(1);
// request RN's during engine initialization (pre threads)
inline unsigned long uintValueP() { 
	return PGen.uintValue();
}
inline float floatValueP() { 
	return PGen.floatValue();
}

} // random

} // lux

#endif //LUX_RANDOM_H
