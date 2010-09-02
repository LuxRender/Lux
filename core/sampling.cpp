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

// sampling.cpp*
#include "lux.h"
#include "memory.h"
#include "sampling.h"
#include "scene.h"
#include "transport.h"
#include "volume.h"
#include "film.h"

using namespace lux;

// Sampler Method Definitions
Sampler::Sampler(int xstart, int xend, int ystart, int yend,
		u_int spp) {
	xPixelStart = min(xstart, xend);
	xPixelEnd = max(xstart, xend);
	yPixelStart = min(ystart, yend);
	yPixelEnd = max(ystart, yend);
	samplesPerPixel = spp;
}

float *Sampler::GetLazyValues(const Sample &sample, u_int num, u_int pos)
{
	return sample.xD[num] + pos * sample.dxD[num];
}

void Sampler::AddSample(const Sample &sample)
{
	sample.contribBuffer->AddSampleCount(1.f);
	for (u_int i = 0; i < sample.contributions.size(); ++i)
		sample.contribBuffer->Add(sample.contributions[i], 1.f);
	sample.contributions.clear();
}

// Sample Method Definitions
Sample::Sample(SurfaceIntegrator *surf, VolumeIntegrator *vol,
	const Scene &scene)
{
	stamp = 0;
	samplerData = NULL;
	surf->RequestSamples(this, scene);
	vol->RequestSamples(this, scene);
	// Allocate storage for sample pointers
	u_int nPtrs = n1D.size() + n2D.size() + nxD.size();
	if (!nPtrs) {
		oneD = twoD = xD = NULL;
		return;
	}
	oneD = AllocAligned<float *>(nPtrs);
	timexD = AllocAligned<int *>(nxD.size());
	twoD = oneD + n1D.size();
	xD = twoD + n2D.size();
	// Compute total number of sample values needed
	u_int totSamples = 0;
	u_int totTime = 0;
	for (u_int i = 0; i < n1D.size(); ++i)
		totSamples += n1D[i];
	for (u_int i = 0; i < n2D.size(); ++i)
		totSamples += 2 * n2D[i];
	for (u_int i = 0; i < nxD.size(); ++i) {
		totSamples += dxD[i] * nxD[i];
		totTime += nxD[i];
	}
	// Allocate storage for sample values
	float *mem = AllocAligned<float>(totSamples);
	int *tmem = AllocAligned<int>(totTime);
	// make sure to assign onexD[0] even if n1D.size() == 0
	oneD[0] = mem;
	for (u_int i = 0; i < n1D.size(); ++i) {
		oneD[i] = mem;
		mem += n1D[i];
	}
	for (u_int i = 0; i < n2D.size(); ++i) {
		twoD[i] = mem;
		mem += 2 * n2D[i];
	}
	// make sure to assign timexD[0] even if nxD.size() == 0
	timexD[0] = tmem;
	for (u_int i = 0; i < nxD.size(); ++i) {
		xD[i] = mem;
		mem += dxD[i] * nxD[i];
		timexD[i] = tmem;
		tmem += nxD[i];
	}
}

Sample::~Sample()
{
	if (oneD != NULL) {
		FreeAligned(oneD[0]);
		FreeAligned(oneD);
	}
	if (timexD != NULL) {
		FreeAligned(timexD[0]);
		FreeAligned(timexD);
	}
}

namespace lux
{

// Sampling Function Definitions
void StratifiedSample1D(const RandomGenerator &rng, float *samp,
	u_int nSamples, bool jitter)
{
	float invTot = 1.f / nSamples;
	for (u_int i = 0;  i < nSamples; ++i) {
		float delta = jitter ? rng.floatValue() : 0.5f;
		*samp++ = (i + delta) * invTot;
	}
}
void StratifiedSample2D(const RandomGenerator &rng, float *samp,
	u_int nx, u_int ny, bool jitter)
{
	float dx = 1.f / nx, dy = 1.f / ny;
	for (u_int y = 0; y < ny; ++y)
		for (u_int x = 0; x < nx; ++x) {
			float jx = jitter ? rng.floatValue() : 0.5f;
			float jy = jitter ? rng.floatValue() : 0.5f;
			*samp++ = (x + jx) * dx;
			*samp++ = (y + jy) * dy;
		}
}
void Shuffle(const RandomGenerator &rng, float *samp, u_int count, u_int dims)
{
	for (u_int i = 0; i < count; ++i) {
		u_int other = rng.uintValue() % count;
		for (u_int j = 0; j < dims; ++j)
			swap(samp[dims*i + j], samp[dims*other + j]);
	}
}
void LatinHypercube(const RandomGenerator &rng, float *samples,
	u_int nSamples, u_int nDim)
{
	// Generate LHS samples along diagonal
	float delta = 1.f / nSamples;
	for (u_int i = 0; i < nSamples; ++i)
		for (u_int j = 0; j < nDim; ++j)
			samples[nDim * i + j] = (i + rng.floatValue()) * delta;
	// Permute LHS samples in each dimension
	for (u_int i = 0; i < nDim; ++i) {
		for (u_int j = 0; j < nSamples; ++j) {
			u_int other = rng.uintValue() % nSamples;
			swap(samples[nDim * j + i],
			     samples[nDim * other + i]);
		}
	}
}
 
}//namespace lux

