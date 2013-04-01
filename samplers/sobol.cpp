/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

#include <boost/foreach.hpp>

#include "slg/sampler/sampler.h"

#include "sobol.h"
#include "scene.h"
#include "dynload.h"
#include "error.h"

using namespace lux;

SobolSampler::SobolData::SobolData(const Sampler &sampler, const Sample &sample) :
		rng0(sample.rng->floatValue()), rng1(sample.rng->floatValue()), pass(SOBOL_STARTOFFSET),
		samplingMap(NULL), noiseAwareMapVersion(0), userSamplingMapVersion(0) {
	nxD = sampler.nxD.size();
	xD = new float *[nxD];
	for (u_int i = 0; i < nxD; ++i)
		xD[i] = new float[sampler.dxD[i]];
}

SobolSampler::SobolData::~SobolData() {
	for (u_int i = 0; i < nxD; ++i)
		delete[] xD[i];
	delete[] xD;
}

u_int SobolSampler::SobolData::SobolDimension(const SobolSampler &sampler,
		const u_int index, const u_int dimension) const {
	const u_int offset = dimension * SOBOL_BITS;
	u_int result = 0;
	u_int i = index;

	for (u_int j = 0; i; i >>= 1, j++) {
		if (i & 1)
			result ^= sampler.directions[offset + j];
	}

	return result;
}

float SobolSampler::SobolData::GetSample(const SobolSampler &sampler, const u_int index) const {
	const u_int result = SobolDimension(sampler, pass, index);
	const float r = result * (1.f / 0xffffffffu);

	// Cranley-Patterson rotation to reduce visible regular patterns
	const float shift = (index & 1) ? rng0 : rng1;

	return r + shift - floorf(r + shift);
}

SobolSampler::SobolSampler(int xstart, int xend, int ystart, int yend,
		bool useNoise) : Sampler(xstart, xend, ystart, yend, 1), directions(NULL),
		useNoiseAware(useNoise) {
	totalPixels = (xPixelEnd - xPixelStart) * (yPixelEnd - yPixelStart);

	AddStringConstant(*this, "name", "Name of current sampler", "sobol");
}

SobolSampler::~SobolSampler() {
	delete[] directions;
}

void SobolSampler::InitSample(Sample *sample) const {
	// Check if I have to initialize directions
	if (!directions) {
		fast_mutex::scoped_lock lock(initDirectionsMutex);

		// Check if I have still to initialize directions (or some other
		// threads has already done)
		if (!directions) {
			// Count the number of samples I have to generate and initialize offsets

			// First samples are for image X/Y, lens U/V, time and wavelength
			u_int sampleCount = 6;
			offset1D.push_back(6);
			BOOST_FOREACH(u_int size, n1D) {
					offset1D.push_back(size);
					sampleCount += size;
			}

			offset2D.push_back(sampleCount);
			BOOST_FOREACH(u_int size, n2D) {
					offset2D.push_back(2 * size);
					sampleCount += 2 * size;
			}

			offsetxD.push_back(sampleCount);
			for (u_int i = 0; i < nxD.size(); ++i) {
					offsetxD.push_back(dxD[i] * nxD[i]);
					sampleCount += dxD[i] * nxD[i];
			}
			LOG(LUX_DEBUG, LUX_NOERROR) << "Total sample count: " << sampleCount;

			// Initialize Sobol data
			directions = new u_int[sampleCount * SOBOL_BITS];
			slg::SobolGenerateDirectionVectors(directions, sampleCount);
		}
	}

	sample->sampler = const_cast<SobolSampler *>(this);
	sample->samplerData = new SobolData(*this, *sample);
}

// return TotalPixels so scene shared thread increment knows total sample positions
u_int SobolSampler::GetTotalSamplePos() {
	return totalPixels;
}

bool SobolSampler::GetNextSample(Sample *sample) {
	SobolData *data = (SobolData *)(sample->samplerData);
	bool haveMoreSamples = true;

	// Compute new set of samples if needed for next pixel
	const float ix = data->GetSample(*this, 0);
	const float iy = data->GetSample(*this, 1);
	if (useNoiseAware || film->HasUserSamplingMap()) {
		// Noise-aware and/or user driven sampler

		// Check if there is a new version of the noise map and/or user-sampling map
		if (useNoiseAware) {
			if (film->HasUserSamplingMap()) {
				film->GetSamplingMap(data->noiseAwareMapVersion, data->userSamplingMapVersion,
						data->samplingMap, data->samplingDistribution2D);
			} else
				film->GetNoiseAwareMap(data->noiseAwareMapVersion,
						data->samplingMap, data->samplingDistribution2D);
		} else {
			if (film->HasUserSamplingMap()) {
				film->GetUserSamplingMap(data->userSamplingMapVersion,
						data->samplingMap, data->samplingDistribution2D);
			} else {
				// This should never happen
				LOG(LUX_ERROR, LUX_SYSTEM) << "Internal error in SobolSampler::GetNextSample()";
			}
		}

		if ((data->noiseAwareMapVersion > 0) || (data->userSamplingMapVersion > 0)) {
			float uv[2], pdf;
			data->samplingDistribution2D->SampleContinuous(ix, iy, uv, &pdf);
			sample->imageX = uv[0] * (xPixelEnd - xPixelStart) + xPixelStart;
			sample->imageY = uv[1] * (yPixelEnd - yPixelStart) + yPixelStart;

			if (film->enoughSamplesPerPixel)
				haveMoreSamples = false;
		} else {
			sample->imageX = ix * (xPixelEnd - xPixelStart) + xPixelStart;
			sample->imageY = iy * (yPixelEnd - yPixelStart) + yPixelStart;
		}
	} else {
		sample->imageX = ix * (xPixelEnd - xPixelStart) + xPixelStart;
		sample->imageY = iy * (yPixelEnd - yPixelStart) + yPixelStart;

		if (film->enoughSamplesPerPixel)
			haveMoreSamples = false;
	}

	// Return next \mono{SobolSampler} sample point
	sample->lensU = data->GetSample(*this, 2);
	sample->lensV = data->GetSample(*this, 3);
	sample->time = data->GetSample(*this, 4);
	sample->wavelengths = data->GetSample(*this, 5);

	++(data->pass);

	return haveMoreSamples;
}

float SobolSampler::GetOneD(const Sample &sample, u_int num, u_int pos) {
	SobolData *data = (SobolData *)(sample.samplerData);

	return data->GetSample(*this, offset1D[num] + pos);
}

void SobolSampler::GetTwoD(const Sample &sample, u_int num, u_int pos, float u[2]) {
	SobolData *data = (SobolData *)(sample.samplerData);

	const u_int offset = offset2D[num] + 2 * pos;
	u[0] = data->GetSample(*this, offset);
	u[1] = data->GetSample(*this, offset + 1);
}

float *SobolSampler::GetLazyValues(const Sample &sample, u_int num, u_int pos) {
	SobolData *data = (SobolData *)(sample.samplerData);

	float *sd = data->xD[num];
	const u_int offset = offsetxD[num] + dxD[num] * pos;
	for (u_int i = 0; i < dxD[num]; ++i)
		sd[i] = data->GetSample(*this, offset + i);

	return sd;
}

Sampler *SobolSampler::CreateSampler(const ParamSet &params, Film *film) {
	bool useNoiseAware = params.FindOneBool("noiseaware", false);
	if (useNoiseAware) {
		// Enable Film noise-aware map generation
		film->EnableNoiseAwareMap();
	}

    int xstart, xend, ystart, yend;
    film->GetSampleExtent(&xstart, &xend, &ystart, &yend);

    return new SobolSampler(xstart, xend, ystart, yend, useNoiseAware);
}

static DynamicLoader::RegisterSampler<SobolSampler> r("sobol");
