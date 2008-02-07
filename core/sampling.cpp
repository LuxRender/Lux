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

// sampling.cpp*
#include "lux.h"
#include "sampling.h"
#include "transport.h"
#include "volume.h"
#include "film.h"

using namespace lux;

// Sampler Method Definitions
Sampler::~Sampler() {
}
Sampler::Sampler(int xstart, int xend, int ystart, int yend,
		int spp) {
	xPixelStart = xstart;
	xPixelEnd = xend;
	yPixelStart = ystart;
	yPixelEnd = yend;
	samplesPerPixel = spp;
}
float *Sampler::GetLazyValues(Sample *sample, u_int num, u_int pos)
{
	return sample->xD[num] + pos * sample->dxD[num];
}
void Sampler::AddSample(const Sample &sample, const Ray &ray,
	const SWCSpectrum &L, float alpha, Film *film)
{
	if (!L.Black())
		film->AddSample(sample.imageX, sample.imageY, L.ToXYZ(), alpha);
}
// Sample Method Definitions
Sample::Sample(SurfaceIntegrator *surf,
		VolumeIntegrator *vol, const Scene *scene) {
	stamp = 0;
	sampler = NULL;
	surf->RequestSamples(this, scene);
	vol->RequestSamples(this, scene);
	// Allocate storage for sample pointers
	int nPtrs = n1D.size() + n2D.size() + nxD.size();
	if (!nPtrs) {
		oneD = twoD = xD = NULL;
		return;
	}
	oneD = (float **)AllocAligned(nPtrs * sizeof(float *));
	timexD = (int **)AllocAligned(nxD.size() * sizeof(int *));
	twoD = oneD + n1D.size();
	xD = twoD + n2D.size();
	// Compute total number of sample values needed
	int totSamples = 0;
	int totTime = 0;
	for (u_int i = 0; i < n1D.size(); ++i)
		totSamples += n1D[i];
	for (u_int i = 0; i < n2D.size(); ++i)
		totSamples += 2 * n2D[i];
	for (u_int i = 0; i < nxD.size(); ++i) {
		totSamples += dxD[i] * nxD[i];
		totTime += nxD[i];
	}
	// Allocate storage for sample values
	float *mem = (float *)AllocAligned(totSamples *
		sizeof(float));
	int *tmem = (int *)AllocAligned(totTime * sizeof(int));
	for (u_int i = 0; i < n1D.size(); ++i) {
		oneD[i] = mem;
		mem += n1D[i];
	}
	for (u_int i = 0; i < n2D.size(); ++i) {
		twoD[i] = mem;
		mem += 2 * n2D[i];
	}
	for (u_int i = 0; i < nxD.size(); ++i) {
		xD[i] = mem;
		mem += dxD[i] * nxD[i];
		timexD[i] = tmem;
		tmem += nxD[i];
	}
}

namespace lux
{

// Sampling Function Definitions
 void StratifiedSample1D(float *samp, int nSamples,
		bool jitter) {
	float invTot = 1.f / nSamples;
	for (int i = 0;  i < nSamples; ++i) {
		float delta = jitter ? lux::random::floatValue() : 0.5f;
		*samp++ = (i + delta) * invTot;
	}
}
 void StratifiedSample2D(float *samp, int nx, int ny,
		bool jitter) {
	float dx = 1.f / nx, dy = 1.f / ny;
	for (int y = 0; y < ny; ++y)
		for (int x = 0; x < nx; ++x) {
			float jx = jitter ? lux::random::floatValue() : 0.5f;
			float jy = jitter ? lux::random::floatValue() : 0.5f;
			*samp++ = (x + jx) * dx;
			*samp++ = (y + jy) * dy;
		}
}
 void Shuffle(float *samp, int count, int dims) {
	for (int i = 0; i < count; ++i) {
		u_int other = lux::random::uintValue() % count;
		for (int j = 0; j < dims; ++j)
			swap(samp[dims*i + j], samp[dims*other + j]);
	}
}
 void LatinHypercube(float *samples,
                             int nSamples, int nDim) {
	// Generate LHS samples along diagonal
	float delta = 1.f / nSamples;
	for (int i = 0; i < nSamples; ++i)
		for (int j = 0; j < nDim; ++j)
			samples[nDim * i + j] = (i + lux::random::floatValue()) * delta;
	// Permute LHS samples in each dimension
	for (int i = 0; i < nDim; ++i) {
		for (int j = 0; j < nSamples; ++j) {
			u_int other = lux::random::uintValue() % nSamples;
			swap(samples[nDim * j + i],
			     samples[nDim * other + i]);
		}
	}
}
 
}//namespace lux

