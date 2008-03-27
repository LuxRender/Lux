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

#ifndef LUX_SAMPLING_H
#define LUX_SAMPLING_H
// sampling.h*
#include "lux.h"
#include "film.h"
#include "color.h"
#include "memory.h"

namespace lux
{

// Sampling Declarations
enum SamplingType {
	SAMPLING_DIRECT = 1 << 0,
	SAMPLING_INDIRECT = 1 << 1,
	SAMPLING_EYETOLIGHT = 1 << 2,
	SAMPLING_LIGHTTOEYE = 1 << 3,
	SAMPLING_ALL = (1 << 4) - 1
};

class Sample {
public:
	// Sample Public Methods
	Sample(SurfaceIntegrator *surf, VolumeIntegrator *vol,
		const Scene *scene);

	u_int Add1D(u_int num) {
		n1D.push_back(num);
		return n1D.size()-1;
	}
	u_int Add2D(u_int num) {
		n2D.push_back(num);
		return n2D.size()-1;
	}
	u_int AddxD(vector<u_int> &structure, u_int num) {
		nxD.push_back(num);
		sxD.push_back(structure);
		u_int d = 0;
		for (u_int i = 0; i < structure.size(); ++i)
			d += structure[i];
		dxD.push_back(d);
		return nxD.size()-1;
	}
	void AddContribution(float x, float y, const XYZColor &c, float a,
		int b = 0, int g = 0) const {
		contributions.push_back(Contribution(x, y, c, a, b, g));
	}
	~Sample() {
		if (oneD != NULL) {
			FreeAligned(oneD[0]);
			FreeAligned(oneD);
		}
		if (timexD != NULL) {
			FreeAligned(timexD[0]);
			FreeAligned(timexD);
		}
	}

	// Sample internal public type
	class Contribution {
	public:
		Contribution(float x, float y, const XYZColor &c, float a, int b, int g) :
			imageX(x), imageY(y), color(c), alpha(a), buffer(b), 
			bufferGroup(g) { }

		float imageX, imageY;
		XYZColor color;
		float alpha;
		int buffer, bufferGroup;
	};

	//Sample public data
	// Reference to the sampler for lazy evaluation
	Sampler *sampler;
	SamplingType sampling;
	// Camera _Sample_ Data
	float imageX, imageY;
	float lensU, lensV;
	float time;
	float wavelengths, singleWavelength;
	// Integrator _Sample_ Data
	mutable int stamp;
	vector<u_int> n1D, n2D, nxD, dxD;
	vector<vector<u_int> > sxD;
	float **oneD, **twoD, **xD;
	int **timexD;
	mutable vector<Contribution> contributions;
};

class  Sampler {
public:
	// Sampler Interface
	virtual ~Sampler() {}
	Sampler(int xstart, int xend, int ystart, int yend, int spp);
	virtual bool GetNextSample(Sample *sample, u_int *use_pos) = 0;
	virtual float *GetLazyValues(Sample *sample, u_int num, u_int pos);
	virtual u_int GetTotalSamplePos() = 0;
	int TotalSamples() const {
		return samplesPerPixel * (xPixelEnd - xPixelStart) * (yPixelEnd - yPixelStart);
	}
	virtual int RoundSize(int size) const = 0;
	virtual void SampleBegin(const Sample *sample)
	{
		isSampleEnd = false;
		sample->contributions.clear();
	}
	virtual void SampleEnd()
	{
		isSampleEnd = true;
	}
	void SetFilm(Film* f)
	{
		film = f;
	}
	virtual void GetBufferType(BufferType *t)
	{}
/*	virtual void AddSample(float imageX, float imageY, const Sample &sample,
		const Ray &ray, const XYZColor &L, float alpha, int id = 0);*/
	virtual void AddSample(const Sample &sample);
	virtual Sampler* clone() const = 0;   // Lux Virtual (Copy) Constructor
	// Sampler Public Data
	int xPixelStart, xPixelEnd, yPixelStart, yPixelEnd;
	int samplesPerPixel;
	Film *film;
	bool isSampleEnd;
};
class SampleGuard
{
public:
	SampleGuard(Sampler *s, const Sample *sample)
	{
		sampler = s;
		sampler->SampleBegin(sample);
	}
	~SampleGuard()
	{
		sampler->SampleEnd();
	}
private:
	Sampler *sampler;
};

// PxLoc X and Y pixel coordinate struct
struct PxLoc {
	short x;
	short y;
};
class PixelSampler {
public:
	PixelSampler() {}
	virtual ~PixelSampler() {}
	virtual u_int GetTotalPixels() = 0;
	virtual bool GetNextPixel(int &xPos, int &yPos, u_int *use_pos) = 0;
};

void StratifiedSample1D(float *samples, int nsamples, bool jitter = true);
void StratifiedSample2D(float *samples, int nx, int ny, bool jitter = true);
void Shuffle(float *samp, int count, int dims);
void LatinHypercube(float *samples, int nSamples, int nDim);

// Sampling Inline Functions
inline double RadicalInverse(int n, int base)
{
	double val = 0.;
	double invBase = 1. / base, invBi = invBase;
	while (n > 0) {
		// Compute next digit of radical inverse
		int d_i = (n % base);
		val += d_i * invBi;
		n /= base;
		invBi *= invBase;
	}
	return val;
}
inline double FoldedRadicalInverse(int n, int base)
{
	double val = 0.;
	double invBase = 1. / base, invBi = invBase;
	int modOffset = 0;
	while (val + base * invBi != val) {
		// Compute next digit of folded radical inverse
		int digit = ((n + modOffset) % base);
		val += digit * invBi;
		n /= base;
		invBi *= invBase;
		++modOffset;
	}
	return val;
}
inline float VanDerCorput(u_int n, u_int scramble = 0)
{
	n = (n << 16) | (n >> 16);
	n = ((n & 0x00ff00ff) << 8) | ((n & 0xff00ff00) >> 8);
	n = ((n & 0x0f0f0f0f) << 4) | ((n & 0xf0f0f0f0) >> 4);
	n = ((n & 0x33333333) << 2) | ((n & 0xcccccccc) >> 2);
	n = ((n & 0x55555555) << 1) | ((n & 0xaaaaaaaa) >> 1);
	n ^= scramble;
	return (float)n / (float)0x100000000LL;
}
inline float Sobol2(u_int n, u_int scramble = 0)
{
	for (u_int v = 1u << 31; n != 0; n >>= 1, v ^= v >> 1)
		if (n & 0x1) scramble ^= v;
	return (float)scramble / (float)0x100000000LL;
}
inline float LarcherPillichshammer2(u_int n, u_int scramble = 0)
{
	for (u_int v = 1u << 31; n != 0; n >>= 1, v |= v >> 1)
		if (n & 0x1) scramble ^= v;
	return (float)scramble / (float)0x100000000LL;
}
inline float Halton(u_int n, u_int scramble = 0)
{
	float s = FoldedRadicalInverse(n, 2);
	u_int s0 = (u_int) (s * (float)0x100000000LL);
	s0 ^= scramble;
	return (float)s0 / (float)0x100000000LL;
}
inline float Halton2(u_int n, u_int scramble = 0)
{
	float s = FoldedRadicalInverse(n, 3);
	u_int s0 = (u_int) (s * (float)0x100000000LL);
	s0 ^= scramble;
	return (float)s0 / (float)0x100000000LL;
}
inline void SampleHalton(u_int n, u_int scramble[2], float sample[2])
{
	sample[0] = FoldedRadicalInverse(n, 2);
	sample[1] = FoldedRadicalInverse(n, 3);
	u_int s0 = (u_int) (sample[0] * (float)0x100000000LL);
	u_int s1 = (u_int) (sample[1] * (float)0x100000000LL);
	s0 ^= scramble[0];
	s1 ^= scramble[1];
	sample[0] = (float)s0 / (float)0x100000000LL;
	sample[1] = (float)s1 / (float)0x100000000LL;
}
inline void Sample02(u_int n, u_int scramble[2], float sample[2])
{
	sample[0] = VanDerCorput(n, scramble[0]);
	sample[1] = Sobol2(n, scramble[1]);
}

inline void LDShuffleScrambled1D(int nSamples,
		int nPixel, float *samples) {
	u_int scramble = lux::random::uintValue();
	for (int i = 0; i < nSamples * nPixel; ++i)
		samples[i] = VanDerCorput(i, scramble);
	for (int i = 0; i < nPixel; ++i)
		Shuffle(samples + i * nSamples, nSamples, 1);
	Shuffle(samples, nPixel, nSamples);
}
inline void LDShuffleScrambled2D(int nSamples,
		int nPixel, float *samples) {
	u_int scramble[2] = { lux::random::uintValue(), lux::random::uintValue() };
	for (int i = 0; i < nSamples * nPixel; ++i)
		Sample02(i, scramble, &samples[2*i]);
	for (int i = 0; i < nPixel; ++i)
		Shuffle(samples + 2 * i * nSamples, nSamples, 2);
	Shuffle(samples, nPixel, 2 * nSamples);
}
inline void HaltonShuffleScrambled1D(int nSamples,
		int nPixel, float *samples) {
	u_int scramble = lux::random::uintValue();
	for (int i = 0; i < nSamples * nPixel; ++i)
		samples[i] = Halton(i, scramble);
	for (int i = 0; i < nPixel; ++i)
		Shuffle(samples + i * nSamples, nSamples, 1);
	Shuffle(samples, nPixel, nSamples);
}
inline void HaltonShuffleScrambled2D(int nSamples,
		int nPixel, float *samples) {
	u_int scramble[2] = { lux::random::uintValue(), lux::random::uintValue() };
	for (int i = 0; i < nSamples * nPixel; ++i)
		SampleHalton(i, scramble, &samples[2*i]);
	for (int i = 0; i < nPixel; ++i)
		Shuffle(samples + 2 * i * nSamples, nSamples, 2);
	Shuffle(samples, nPixel, 2 * nSamples);
}

}//namespace lux

#endif // LUX_SAMPLING_H
