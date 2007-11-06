
#include "random.h"


namespace lux
{
namespace random
{

unsigned long mt[LUX_N]; /* the array for the state vector  */
int mti=LUX_N+1; /* mti==N+1 means mt[N] is not initialized */

void init_genrand(u_long seed) {
		mt[0]= seed & 0xffffffffUL;
		for (mti=1; mti<LUX_N; mti++) {
			mt[mti] =
			(1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti);
			/* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
			/* In the previous versions, MSBs of the seed affect   */
			/* only MSBs of the array mt[].                        */
			/* 2002/01/09 modified by Makoto Matsumoto             */
			mt[mti] &= 0xffffffffUL;
			/* for >32 bit machines */
		}
	}



}

}
