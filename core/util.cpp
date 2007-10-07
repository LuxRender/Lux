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

// util.cpp*
#include "lux.h"

#ifdef LUX_USE_SSE
typedef int     int32; //!<<@warning Is the 'int' type always 32 bit ?
#ifdef __GNUC__
const float __attribute__ ((aligned(16))) _F1001[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
const int32 __attribute__ ((aligned(16))) _Sign_PNNP[4] = { 0x00000000, 0x80000000, 0x80000000, 0x00000000 };
#else //intel compiler
const _MM_ALIGN16 float _F1001[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
const _MM_ALIGN16 __int32 _Sign_PNNP[4] = { 0x00000000, 0x80000000, 0x80000000, 0x00000000 };
#endif
#define Sign_PNNP   (*(__m128*)&_Sign_PNNP)        // + - - +
#endif //LUX_USE_SSE

#include "timer.h"
#include <map>
using std::map;
// Error Reporting Includes
#include <stdarg.h>
// Error Reporting Definitions
#define LUX_ERROR_IGNORE 0
#define LUX_ERROR_CONTINUE 1
#define LUX_ERROR_ABORT 2
// Error Reporting Functions
static void processError(const char *format, va_list args,
		const char *message, int disposition) {
#ifndef WIN32
	char *errorBuf;
	vasprintf(&errorBuf, format, args);
#else
	char errorBuf[2048];
	_vsnprintf(errorBuf, sizeof(errorBuf), format, args);
#endif
	// Report error
	switch (disposition) {
	case LUX_ERROR_IGNORE:
		return;
	case LUX_ERROR_CONTINUE:
		fprintf(stderr, "%s: %s\n", message, errorBuf);
		// Print scene file and line number, if appropriate
		extern int line_num;
		if (line_num != 0) {
			extern string current_file;
			fprintf(stderr, "\tLine %d, file %s\n", line_num,
				current_file.c_str());
		}
		break;
	case LUX_ERROR_ABORT:
		fprintf(stderr, "%s: %s\n", message, errorBuf);
		// Print scene file and line number, if appropriate
		extern int line_num;
		if (line_num != 0) {
			extern string current_file;
			fprintf(stderr, "\tLine %d, file %s\n", line_num,
				current_file.c_str());
		}
		exit(1);
	}
#ifndef WIN32
	free(errorBuf);
#endif
}
COREDLL void Info(const char *format, ...) {
	va_list args;
	va_start(args, format);
	processError(format, args, "Notice", LUX_ERROR_CONTINUE);
	va_end(args);
}
COREDLL void Warning(const char *format, ...) {
	va_list args;
	va_start(args, format);
	processError(format, args, "Warning", LUX_ERROR_CONTINUE);
	va_end(args);
}
COREDLL void Error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	processError(format, args, "Error", LUX_ERROR_CONTINUE);
	va_end(args);
}
COREDLL void Severe(const char *format, ...) {
	va_list args;
	va_start(args, format);
	processError(format, args, "Fatal Error", LUX_ERROR_ABORT);
	va_end(args);
}
// Matrix Method Definitions
COREDLL bool SolveLinearSystem2x2(const float A[2][2],
		const float B[2], float x[2]) {
	float det = A[0][0]*A[1][1] - A[0][1]*A[1][0];
	if (fabsf(det) < 1e-5)
		return false;
	float invDet = 1.0f/det;
	x[0] = (A[1][1]*B[0] - A[0][1]*B[1]) * invDet;
	x[1] = (A[0][0]*B[1] - A[1][0]*B[0]) * invDet;
	return true;
}
#ifndef LUX_USE_SSE
Matrix4x4::Matrix4x4(float mat[4][4]) {
	memcpy(m, mat, 16*sizeof(float));
}
#else
Matrix4x4::Matrix4x4(float mat[4][4]) {
	  _L1 = _mm_set_ps(mat[3][0], mat[2][0], mat[1][0], mat[0][0]);
      _L2 = _mm_set_ps(mat[3][1], mat[2][1], mat[1][1], mat[0][1]);
      _L3 = _mm_set_ps(mat[3][2], mat[2][2], mat[1][2], mat[0][2]);
      _L4 = _mm_set_ps(mat[3][3], mat[2][3], mat[1][3], mat[0][3]);
}
#endif

#ifndef LUX_USE_SSE
Matrix4x4::Matrix4x4(float t00, float t01, float t02, float t03,
                     float t10, float t11, float t12, float t13,
                     float t20, float t21, float t22, float t23,
                     float t30, float t31, float t32, float t33) {
	m[0][0] = t00; m[0][1] = t01; m[0][2] = t02; m[0][3] = t03;
	m[1][0] = t10; m[1][1] = t11; m[1][2] = t12; m[1][3] = t13;
	m[2][0] = t20; m[2][1] = t21; m[2][2] = t22; m[2][3] = t23;
	m[3][0] = t30; m[3][1] = t31; m[3][2] = t32; m[3][3] = t33;
}
#else
Matrix4x4::Matrix4x4(float f11, float f12, float f13, float f14,
               float f21, float f22, float f23, float f24,
               float f31, float f32, float f33, float f34,
               float f41, float f42, float f43, float f44)
{
      _L1 = _mm_set_ps(f41, f31, f21, f11);
      _L2 = _mm_set_ps(f42, f32, f22, f12);
      _L3 = _mm_set_ps(f43, f33, f23, f13);
      _L4 = _mm_set_ps(f44, f34, f24, f14);
}
#endif


#ifndef LUX_USE_SSE
Matrix4x4Ptr Matrix4x4::Transpose() const {
	Matrix4x4Ptr o(new Matrix4x4(m[0][0], m[1][0], m[2][0], m[3][0],
	                    m[0][1], m[1][1], m[2][1], m[3][1],
	                    m[0][2], m[1][2], m[2][2], m[3][2],
	                    m[0][3], m[1][3], m[2][3], m[3][3]));
	return o;
}
#else
Matrix4x4Ptr Matrix4x4::Transpose() const {
      __m128  xmm0 = _mm_unpacklo_ps(_L1,_L2),
                     xmm1 = _mm_unpacklo_ps(_L3,_L4),
                            xmm2 = _mm_unpackhi_ps(_L1,_L2),
                                   xmm3 = _mm_unpackhi_ps(_L3,_L4);

	__m128 L1, L2, L3, L4;
      L1 = _mm_movelh_ps(xmm0,xmm1);
      L2 = _mm_movehl_ps(xmm1,xmm0);
      L3 = _mm_movelh_ps(xmm2,xmm3);
      L4 = _mm_movehl_ps(xmm3,xmm2);

	Matrix4x4Ptr o (new Matrix4x4(L1, L2, L3, L4));
      return o;
}
#endif



#ifndef LUX_USE_SSE
Matrix4x4Ptr Matrix4x4::Inverse() const {
	int indxc[4], indxr[4];
	int ipiv[4] = { 0, 0, 0, 0 };
	float minv[4][4];
	memcpy(minv, m, 4*4*sizeof(float));
	for (int i = 0; i < 4; i++) {
		int irow = -1, icol = -1;
		float big = 0.;
		// Choose pivot
		for (int j = 0; j < 4; j++) {
			if (ipiv[j] != 1) {
				for (int k = 0; k < 4; k++) {
					if (ipiv[k] == 0) {
						if (fabsf(minv[j][k]) >= big) {
							big = float(fabsf(minv[j][k]));
							irow = j;
							icol = k;
						}
					}
					else if (ipiv[k] > 1)
						Error("Singular matrix in MatrixInvert");
				}
			}
		}
		++ipiv[icol];
		// Swap rows _irow_ and _icol_ for pivot
		if (irow != icol) {
			for (int k = 0; k < 4; ++k)
				swap(minv[irow][k], minv[icol][k]);
		}
		indxr[i] = irow;
		indxc[i] = icol;
		if (minv[icol][icol] == 0.)
			Error("Singular matrix in MatrixInvert");
		// Set $m[icol][icol]$ to one by scaling row _icol_ appropriately
		float pivinv = 1.f / minv[icol][icol];
		minv[icol][icol] = 1.f;
		for (int j = 0; j < 4; j++)
			minv[icol][j] *= pivinv;
		// Subtract this row from others to zero out their columns
		for (int j = 0; j < 4; j++) {
			if (j != icol) {
				float save = minv[j][icol];
				minv[j][icol] = 0;
				for (int k = 0; k < 4; k++)
					minv[j][k] -= minv[icol][k]*save;
			}
		}
	}
	// Swap columns to reflect permutation
	for (int j = 3; j >= 0; j--) {
		if (indxr[j] != indxc[j]) {
			for (int k = 0; k < 4; k++)
				swap(minv[k][indxr[j]], minv[k][indxc[j]]);
		}
	}
	//return new Matrix4x4(minv);
	Matrix4x4Ptr o (new Matrix4x4(minv));
	return o;
}
#else
Matrix4x4Ptr Matrix4x4::Inverse() const {

    __m128 A = _mm_movelh_ps(_L1, _L2),    // the four sub-matrices
               B = _mm_movehl_ps(_L2, _L1),
                   C = _mm_movelh_ps(_L3, _L4),
                       D = _mm_movehl_ps(_L4, _L3);
    __m128 iA, iB, iC, iD,					// partial inverse of the sub-matrices
    DC, AB;
    __m128 dA, dB, dC, dD;                 // determinant of the sub-matrices
    __m128 det, d, d1, d2;
    __m128 rd;

    //  AB = A# * B
    AB = _mm_mul_ps(_mm_shuffle_ps(A,A,0x0F), B);
    AB = _mm_sub_ps(AB,_mm_mul_ps(_mm_shuffle_ps(A,A,0xA5), _mm_shuffle_ps(B,B,0x4E)));
    //  DC = D# * C
    DC = _mm_mul_ps(_mm_shuffle_ps(D,D,0x0F), C);
    DC = _mm_sub_ps(DC,_mm_mul_ps(_mm_shuffle_ps(D,D,0xA5), _mm_shuffle_ps(C,C,0x4E)));

    //  dA = |A|
    dA = _mm_mul_ps(_mm_shuffle_ps(A, A, 0x5F),A);
    dA = _mm_sub_ss(dA, _mm_movehl_ps(dA,dA));
    //  dB = |B|
    dB = _mm_mul_ps(_mm_shuffle_ps(B, B, 0x5F),B);
    dB = _mm_sub_ss(dB, _mm_movehl_ps(dB,dB));

    //  dC = |C|
    dC = _mm_mul_ps(_mm_shuffle_ps(C, C, 0x5F),C);
    dC = _mm_sub_ss(dC, _mm_movehl_ps(dC,dC));
    //  dD = |D|
    dD = _mm_mul_ps(_mm_shuffle_ps(D, D, 0x5F),D);
    dD = _mm_sub_ss(dD, _mm_movehl_ps(dD,dD));

    //  d = trace(AB*DC) = trace(A#*B*D#*C)
    d = _mm_mul_ps(_mm_shuffle_ps(DC,DC,0xD8),AB);

    //  iD = C*A#*B
    iD = _mm_mul_ps(_mm_shuffle_ps(C,C,0xA0), _mm_movelh_ps(AB,AB));
    iD = _mm_add_ps(iD,_mm_mul_ps(_mm_shuffle_ps(C,C,0xF5), _mm_movehl_ps(AB,AB)));
    //  iA = B*D#*C
    iA = _mm_mul_ps(_mm_shuffle_ps(B,B,0xA0), _mm_movelh_ps(DC,DC));
    iA = _mm_add_ps(iA,_mm_mul_ps(_mm_shuffle_ps(B,B,0xF5), _mm_movehl_ps(DC,DC)));

    //  d = trace(AB*DC) = trace(A#*B*D#*C) [continue]
    d = _mm_add_ps(d, _mm_movehl_ps(d, d));
    d = _mm_add_ss(d, _mm_shuffle_ps(d, d, 1));
    d1 = _mm_mul_ps(dA,dD);
    d2 = _mm_mul_ps(dB,dC);

    //  iD = D*|A| - C*A#*B
    iD = _mm_sub_ps(_mm_mul_ps(D,_mm_shuffle_ps(dA,dA,0)) , iD);

    //  iA = A*|D| - B*D#*C;
    iA = _mm_sub_ps(_mm_mul_ps(A,_mm_shuffle_ps(dD,dD,0)) , iA);

    //  det = |A|*|D| + |B|*|C| - trace(A#*B*D#*C)
    det = _mm_sub_ps(_mm_add_ps(d1,d2),d);
    rd = (__m128)(_mm_div_ps(_mm_set_ss(1.0f),det));
#ifdef ZERO_SINGULAR
    rd = _mm_and_ps(_mm_cmpneq_ss(det,_mm_setzero_ps()), rd);
#endif

    //  iB = D * (A#B)# = D*B#*A
    iB = _mm_mul_ps(D, _mm_shuffle_ps(AB,AB,0x33));
    iB = _mm_sub_ps(iB,_mm_mul_ps(_mm_shuffle_ps(D,D,0xB1), _mm_shuffle_ps(AB,AB,0x66)));
    //  iC = A * (D#C)# = A*C#*D
    iC = _mm_mul_ps(A, _mm_shuffle_ps(DC,DC,0x33));
    iC = _mm_sub_ps(iC,_mm_mul_ps(_mm_shuffle_ps(A,A,0xB1), _mm_shuffle_ps(DC,DC,0x66)));

    rd = _mm_shuffle_ps(rd,rd,0);
    rd = _mm_xor_ps(rd, Sign_PNNP);

    //  iB = C*|B| - D*B#*A
    iB = _mm_sub_ps(_mm_mul_ps(C,_mm_shuffle_ps(dB,dB,0)) , iB);

    //  iC = B*|C| - A*C#*D;
    iC = _mm_sub_ps(_mm_mul_ps(B,_mm_shuffle_ps(dC,dC,0)) , iC);

    //  iX = iX / det
    iA =_mm_mul_ps(iA, rd);
    iB =_mm_mul_ps(iB, rd);
    iC =_mm_mul_ps(iC, rd);
    iD =_mm_mul_ps(iD, rd);

    //return new Matrix4x4(_mm_shuffle_ps(iA,iB,0x77),_mm_shuffle_ps(iA,iB,0x22),_mm_shuffle_ps(iC,iD,0x77),_mm_shuffle_ps(iC,iD,0x22));
    
    Matrix4x4Ptr o (new Matrix4x4(_mm_shuffle_ps(iA,iB,0x77),_mm_shuffle_ps(iA,iB,0x22),_mm_shuffle_ps(iC,iD,0x77),_mm_shuffle_ps(iC,iD,0x22)));
	return o;
}
#endif


// Statistics Definitions
struct COREDLL StatTracker {
	StatTracker(const string &cat, const string &n,
	            StatsCounterType *pa, StatsCounterType *pb = NULL,
		    bool percentage = true);
	string category, name;
	StatsCounterType *ptra, *ptrb;
	bool percentage;
};
typedef map<std::pair<string, string>, StatTracker *> TrackerMap;
static TrackerMap trackers;
static void addTracker(StatTracker *newTracker) {
	std::pair<string, string> s = std::make_pair(newTracker->category, newTracker->name);
	if (trackers.find(s) != trackers.end()) {
		newTracker->ptra = trackers[s]->ptra;
		newTracker->ptrb = trackers[s]->ptrb;
		return;
	}
	trackers[s] = newTracker;
}
static void StatsPrintVal(FILE *f, StatsCounterType v);
static void StatsPrintVal(FILE *f, StatsCounterType v1, StatsCounterType v2);
// Statistics Functions
StatTracker::StatTracker(const string &cat, const string &n,
                         StatsCounterType *pa, StatsCounterType *pb, bool p) {
	category = cat;
	name = n;
	ptra = pa;
	ptrb = pb;
	percentage = p;
}
StatsCounter::StatsCounter(const string &category, const string &name) {
	num = 0;
	addTracker(new StatTracker(category, name, &num));
}
StatsRatio::StatsRatio(const string &category, const string &name) {
	na = nb = 0;
	addTracker(new StatTracker(category, name, &na, &nb, false));
}
StatsPercentage::StatsPercentage(const string &category, const string &name) {
	na = nb = 0;
	addTracker(new StatTracker(category, name, &na, &nb, true));
}
void StatsPrint(FILE *dest) {
	fprintf(dest, "Statistics:\n");
	TrackerMap::iterator iter = trackers.begin();
	string lastCategory;
	while (iter != trackers.end()) {
		// Print statistic
		StatTracker *tr = iter->second;
		if (tr->category != lastCategory) {
			fprintf(dest, "%s\n", tr->category.c_str());
			lastCategory = tr->category;
		}
		fprintf(dest, "    %s", tr->name.c_str());
		// Pad out to results column
		int resultsColumn = 56;
		int paddingSpaces = resultsColumn - (int) tr->name.size();
		while (paddingSpaces-- > 0)
			putc(' ', dest);
		if (tr->ptrb == NULL)
			StatsPrintVal(dest, *tr->ptra);
		else {
			if (*tr->ptrb > 0) {
				float ratio = (float)*tr->ptra / (float)*tr->ptrb;
				StatsPrintVal(dest, *tr->ptra, *tr->ptrb);
				if (tr->percentage)
					fprintf(dest, " (%3.2f%%)", 100. * ratio);
				else
					fprintf(dest, " (%.2fx)", ratio);
			}
			else
				StatsPrintVal(dest, *tr->ptra, *tr->ptrb);
		}
		fprintf(dest, "\n");
		++iter;
	}
}
static void StatsPrintVal(FILE *f, StatsCounterType v) {
	if (v > 1e9) fprintf(f, "%.3fB", v / 1e9f);
	else if (v > 1e6) fprintf(f, "%.3fM", v / 1e6f);
	else if (v > 1e4) fprintf(f, "%.1fk", v / 1e3f);
	else fprintf(f, "%.0f", (float)v);
}
static void StatsPrintVal(FILE *f, StatsCounterType v1,
		StatsCounterType v2) {
	StatsCounterType m = min(v1, v2);
	if (m > 1e9) fprintf(f, "%.3fB:%.3fB", v1 / 1e9f, v2 / 1e9f);
	else if (m > 1e6) fprintf(f, "%.3fM:%.3fM", v1 / 1e6f, v2 / 1e6f);
	else if (m > 1e4) fprintf(f, "%.1fk:%.1fk", v1 / 1e3f, v2 / 1e3f);
	else fprintf(f, "%.0f:%.0f", v1, v2);
}
void StatsCleanup() {
	TrackerMap::iterator iter = trackers.begin();
	string lastCategory;
	while (iter != trackers.end()) {
		delete iter->second;
		++iter;
	}
	trackers.erase(trackers.begin(), trackers.end());
}
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
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */
// Random Number Functions
static void init_genrand(u_long seed) {
	mt[0]= seed & 0xffffffffUL;
	for (mti=1; mti<N; mti++) {
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
COREDLL unsigned long genrand_int32(void)
{
	unsigned long y;
	static unsigned long mag01[2]={0x0UL, MATRIX_A};
	/* mag01[x] = x * MATRIX_A  for x=0,1 */

	if (mti >= N) { /* generate N words at one time */
		int kk;

		if (mti == N+1)   /* if init_genrand() has not been called, */
			init_genrand(5489UL); /* default initial seed */

		for (kk=0;kk<N-M;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
		}
		for (;kk<N-1;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
		}
		y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
		mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

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
COREDLL float genrand_real1(void)
{
	return genrand_int32()*((float)1.0/(float)4294967295.0);
	/* divided by 2^32-1 */
}
/* generates a random number on [0,1)-real-interval */
COREDLL float genrand_real2(void)
{
	return genrand_int32()*((float)1.0/(float)4294967296.0);
	/* divided by 2^32 */
}
// Memory Allocation Functions
COREDLL void *AllocAligned(size_t size) {
#ifndef L1_CACHE_LINE_SIZE
#define L1_CACHE_LINE_SIZE 64
#endif
	return memalign(L1_CACHE_LINE_SIZE, size);
}
COREDLL void FreeAligned(void *ptr) {
#ifdef WIN32 // NOBOOK
	_aligned_free(ptr);
#else // NOBOOK
	free(ptr);
#endif // NOBOOK
}
// ProgressReporter Method Definitions
ProgressReporter::ProgressReporter(int totalWork, const string &title, int bar_length)
	: totalPlusses(bar_length - title.size()) {
	plussesPrinted = 0;
	frequency = (float)totalWork / (float)totalPlusses;
	count = frequency;
	timer = new Timer();
	timer->Start();
	outFile = stdout;
	// Initialize progress string
	const int bufLen = title.size() + totalPlusses + 64;
	buf = new char[bufLen];
	snprintf(buf, bufLen, "\r%s: [", title.c_str());
	curSpace = buf + strlen(buf);
	char *s = curSpace;
	for (int i = 0; i < totalPlusses; ++i)
		*s++ = ' ';
	*s++ = ']';
	*s++ = ' ';
	*s++ = '\0';
	fprintf(outFile, buf);
	fflush(outFile);
}
ProgressReporter::~ProgressReporter() { delete[] buf; delete timer; }
void ProgressReporter::Update(int num) const {
	count -= num;
	bool updatedAny = false;
	while (count <= 0) {
		count += frequency;
		if (plussesPrinted++ < totalPlusses)
			*curSpace++ = '+';
		updatedAny = true;
	}
	if (updatedAny) {
		fputs(buf, outFile);
		// Update elapsed time and estimated time to completion
		float percentDone = (float)plussesPrinted / (float)totalPlusses;
		float seconds = (float) timer->Time();
		float estRemaining = seconds / percentDone - seconds;
		if (percentDone == 1.f)
			fprintf(outFile, " (%.1fs)       ", seconds);
		else
			fprintf(outFile, " (%.1fs|%.1fs)  ", seconds, max(0.f, estRemaining));
		fflush(outFile);
	}
}
void ProgressReporter::Done() const {
	while (plussesPrinted++ < totalPlusses)
		*curSpace++ = '+';
	fputs(buf, outFile);
	float seconds = (float) timer->Time();
	fprintf(outFile, " (%.1fs)       \n", seconds);
	fflush(outFile);
}
