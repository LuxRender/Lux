/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

#include "lux.h"

#include <boost/thread/tss.hpp>

namespace lux
{
  void *AllocAligned(size_t size);
  void FreeAligned(void *);
}

#define LCG(n) ((69069UL * n) & 0xffffffffUL)
#define MASK 0xffffffffUL

#define RAN_BUFFER_AMOUNT 2048

static const float invUI = ((float)1.0/(float)4294967296.0);

namespace lux
{

class RandomGenerator
{
public:
	RandomGenerator() {
		buf = (unsigned long int *)lux::AllocAligned(
			RAN_BUFFER_AMOUNT * sizeof(unsigned long int));
		bufid = RAN_BUFFER_AMOUNT;
	}

	~RandomGenerator() { lux::FreeAligned(buf); }

	void taus113_set(unsigned long int s) {
	  if (!s) s = 1UL; // default seed is 1

	  z1 = LCG (s); if (z1 < 2UL) z1 += 2UL;
	  z2 = LCG (z1); if (z2 < 8UL) z2 += 8UL;
	  z3 = LCG (z2); if (z3 < 16UL) z3 += 16UL;
	  z4 = LCG (z3); if (z4 < 128UL) z4 += 128UL;

	  // Calling RNG ten times to satify recurrence condition
	  for(int i=0; i<10; i++) nobuf_generateUInt();
	}

	inline void init(int tn) {
		taus113_set(tn);
	}

	inline unsigned long nobuf_generateUInt() {
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

	inline unsigned long uintValue() {
	  // Repopulate buffer if necessary
	  if(bufid == RAN_BUFFER_AMOUNT) {
		  for(int i=0; i<RAN_BUFFER_AMOUNT; i++)
			  buf[i] = nobuf_generateUInt();
		  bufid = 0;
	  }

	  unsigned long int ii = buf[bufid];
	  bufid++;
	  return ii; 
	}

	inline float floatValue() {
	  // Repopulate buffer if necessary
	  if(bufid == RAN_BUFFER_AMOUNT) {
		  for(int i=0; i<RAN_BUFFER_AMOUNT; i++)
			  buf[i] = nobuf_generateUInt();
		  bufid = 0;
	  }

	  unsigned long int ii = buf[bufid];
	  bufid++;
	  return ii * invUI; 
	}

private:
	unsigned long int z1, z2, z3, z4;
	unsigned long int *buf;
	int bufid;
};

namespace random {

static RandomGenerator* PGen;
// request RN's during engine initialization (pre threads)
inline float floatValueP() { 
	if(!PGen) {
		PGen = new RandomGenerator();
		PGen->taus113_set(1);
	}
	return PGen->nobuf_generateUInt() * invUI;
}
inline unsigned long uintValueP() { 
	if(!PGen) {
		PGen = new RandomGenerator();
		PGen->taus113_set(1);
	}
	return PGen->nobuf_generateUInt();
}

} // random

} // lux

#endif //LUX_RANDOM_H
