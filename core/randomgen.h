/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_RANDOM_H
#define LUX_RANDOM_H

namespace lux
{

// Random Number State
/*
   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote
        products derived from this software without specific prior written
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define LUX_N 624
#define LUX_M 397
#define LUX_MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define LUX_UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LUX_LOWER_MASK 0x7fffffffUL /* least significant r bits */

namespace random
{
//public:
	extern unsigned long mt[];//[LUX_N]; /* the array for the state vector  */
	extern int mti;//=LUX_N+1; /* mti==N+1 means mt[N] is not initialized */
	
	void init_genrand(u_long seed);
	
//private:
	inline unsigned long genrand_int32(void)
		{
			unsigned long y;
			static unsigned long mag01[2]={0x0UL, LUX_MATRIX_A};
			/* mag01[x] = x * MATRIX_A  for x=0,1 */

			if (mti >= LUX_N) { /* generate N words at one time */
				int kk;

				if (mti == LUX_N+1)   /* if init_genrand() has not been called, */
					init_genrand(5489UL); /* default initial seed */

				for (kk=0;kk<LUX_N-LUX_M;kk++) {
					y = (mt[kk]&LUX_UPPER_MASK)|(mt[kk+1]&LUX_LOWER_MASK);
					mt[kk] = mt[kk+LUX_M] ^ (y >> 1) ^ mag01[y & 0x1UL];
				}
				for (;kk<LUX_N-1;kk++) {
					y = (mt[kk]&LUX_UPPER_MASK)|(mt[kk+1]&LUX_LOWER_MASK);
					mt[kk] = mt[kk+(LUX_M-LUX_N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
				}
				y = (mt[LUX_N-1]&LUX_UPPER_MASK)|(mt[0]&LUX_LOWER_MASK);
				mt[LUX_N-1] = mt[LUX_M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

				mti = 0;
			}

			y = mt[mti++];

			/* Tempering */
			y ^= (y >> 11);
			y ^= (y << 7) & 0x9d2c5680UL;
			y ^= (y << 15) & 0xefc60000UL;
			y ^= (y >> 18);

			return y;
		}
	
	
	/* generates a random number on [0,1]-real-interval */
	inline float genrand_real1(void)
	{
		return genrand_int32()*((float)1.0/(float)4294967295.0);
		/* divided by 2^32-1 */
	}
	/* generates a random number on [0,1)-real-interval */
	inline float genrand_real2(void)
	{
		return genrand_int32()*((float)1.0/(float)4294967296.0);
		/* divided by 2^32 */
	}
	
	
	
	inline float floatValue()
	{
		return genrand_real2();
	}
		
	inline unsigned long uintValue()
	{
		return genrand_int32();
	}
}

}

#endif //LUX_RANDOM_H

