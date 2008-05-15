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

#define LCG(n) ((69069UL * n) & 0xffffffffUL)
#define MASK 0xffffffffUL

#define RAN_BUFFER_AMOUNT 2048

namespace lux
{

namespace random
{

// Dade - old code: it is not thread safe
/*#define MAX_SEEDS 64
static int newseed = 0;
static float seeds[64] = { 
0.8147236705f, 0.1354770064f, 0.0000925521f, 0.4019474089f,
0.9057919383f, 0.8350085616f, 0.0003553750f, 0.0861424059f,
0.5407393575f, 0.2690643370f, 0.0006090801f, 0.6007867455f,
0.2601301968f, 0.6379706860f, 0.0009086037f, 0.7664164901f,
0.3190678060f, 0.9074092507f, 0.0011911630f, 0.9212453961f,
0.7451665998f, 0.5743905902f, 0.0013162681f, 0.9683164358f,
0.4001524150f, 0.5284244418f, 0.0016778698f, 0.6921809912f,
0.1100450456f, 0.1057574451f, 0.0018501711f, 0.0146250632f,
0.8948429227f, 0.2533296347f, 0.0020738356f, 0.4565072358f,
0.2674683630f, 0.5533290505f, 0.0023895311f, 0.1681169569f,
0.3942572773f, 0.2532950938f, 0.0026231781f, 0.2203403264f,
0.4852786958f, 0.3545391560f, 0.0028830934f, 0.3188484907f,
0.6586979032f, 0.8418058157f, 0.0030069787f, 0.7844486237f,
0.9411987066f, 0.5897576213f, 0.0033880144f, 0.8060938716f,
0.5399997830f, 0.9201279879f, 0.0034516300f, 0.7110665441f,
0.7044532299f, 0.3615645170f, 0.0038146735f, 0.3312333524f
};*/

class RandomGenerator
{
public:
	RandomGenerator() {
		buf = (unsigned long int *)memalign(64,
			RAN_BUFFER_AMOUNT * sizeof(unsigned long int));
		bufid = RAN_BUFFER_AMOUNT;
	}

	~RandomGenerator() { delete[] buf; }

	void taus113_set(unsigned long int s) {
	  if (!s) s = 1UL; // default seed is 1

	  z1 = LCG (s); if (z1 < 2UL) z1 += 2UL;
	  z2 = LCG (z1); if (z2 < 8UL) z2 += 8UL;
	  z3 = LCG (z2); if (z3 < 16UL) z3 += 16UL;
	  z4 = LCG (z3); if (z4 < 128UL) z4 += 128UL;

	  // Calling RNG ten times to satify recurrence condition
	  for(int i=0; i<10; i++) nobuf_generateUInt();
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

	inline unsigned long generateUInt() {
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
	
private:
	unsigned long int z1, z2, z3, z4;
	unsigned long int *buf;
	int bufid;
};

// thread local pointer to boost random generator
extern boost::thread_specific_ptr<RandomGenerator> myGen;

static const float invUI = ((float)1.0/(float)4294967296.0);

// Dade - old code: it is not thread safe
/*inline void init(int tn) {
	if(!myGen.get())
		myGen.reset(new RandomGenerator);

	if(newseed >= MAX_SEEDS) newseed = 1; // 0 always used by 1st thr
	unsigned long seed = (unsigned long) (seeds[newseed] * 4294967296.0);
	newseed++;

	myGen->taus113_set(seed);
}*/

inline void init(int tn) {
	if(!myGen.get())
		myGen.reset(new RandomGenerator);

	myGen->taus113_set(tn);
}

// request RN's during render threads (uses per thread rangen/seed)
inline float floatValue() { 
	return myGen->generateUInt() * invUI;
}
inline unsigned long uintValue() { 
	return myGen->generateUInt();
}

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
