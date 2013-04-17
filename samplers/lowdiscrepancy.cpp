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
 
// lowdiscrepancy.cpp*
#include "lowdiscrepancy.h"
#include "error.h"
#include "scene.h"
#include "dynload.h"

using namespace lux;

LDSampler::LDData::LDData(const Sampler &sampler, int xPixelStart, int yPixelStart, u_int pixelSamples) :
	noiseAwareMapVersion(0), userSamplingMapVersion(0) {
	xPos = xPixelStart - 1;
	yPos = yPixelStart;
	samplePos = pixelSamples - 1;

	// Allocate space for pixel's low-discrepancy sample tables
	imageSamples = new float[6 * pixelSamples];
	lensSamples = imageSamples + 2 * pixelSamples;
	timeSamples = imageSamples + 4 * pixelSamples;
	wavelengthsSamples = imageSamples + 5 * pixelSamples;
	oneDSamples = new float *[sampler.n1D.size()];
	n1D = sampler.n1D.size();
	for (u_int i = 0; i < sampler.n1D.size(); ++i)
		oneDSamples[i] = new float[sampler.n1D[i] * pixelSamples];
	twoDSamples = new float *[sampler.n2D.size()];
	n2D = sampler.n2D.size();
	for (u_int i = 0; i < sampler.n2D.size(); ++i)
		twoDSamples[i] = new float[2 * sampler.n2D[i] * pixelSamples];
	xDSamples = new float *[sampler.nxD.size()];
	xD = new float *[sampler.nxD.size()];
	nxD = sampler.nxD.size();
	for (u_int i = 0; i < sampler.nxD.size(); ++i) {
		xDSamples[i] = new float[sampler.dxD[i] * sampler.nxD[i] *
			pixelSamples];
		xD[i] = new float[sampler.dxD[i]];
	}
}

LDSampler::LDData::~LDData()
{
	delete[] imageSamples;
	for (u_int i = 0; i < n1D; ++i)
		delete[] oneDSamples[i];
	for (u_int i = 0; i < n2D; ++i)
		delete[] twoDSamples[i];
	for (u_int i = 0; i < nxD; ++i)
		delete[] xDSamples[i];
	delete[] oneDSamples;
	delete[] twoDSamples;
	delete[] xDSamples;
}

// LDSampler Method Definitions
LDSampler::LDSampler(int xstart, int xend,
		int ystart, int yend, u_int ps, string pixelsampler, bool useNoise)
	: Sampler(xstart, xend, ystart, yend, RoundUpPow2(ps)), useNoiseAware(useNoise) {
	// Initialize PixelSampler
	pixelSampler = MakePixelSampler(pixelsampler, xstart, xend, ystart, yend);

	totalPixels = pixelSampler->GetTotalPixels();

	// check/round pixelsamples to power of 2
	if (!IsPowerOf2(ps)) {
		LOG(LUX_WARNING,LUX_CONSISTENCY)<<"Pixel samples being rounded up to power of 2";
		pixelSamples = RoundUpPow2(ps);
	} else
		pixelSamples = ps;
	sampPixelPos = 0;

	AddStringConstant(*this, "name", "Name of current sampler", "lowdiscrepancy");
}

LDSampler::~LDSampler() {
}

// return TotalPixels so scene shared thread increment knows total sample positions
u_int LDSampler::GetTotalSamplePos() {
	return totalPixels;
}

bool LDSampler::GetNextSample(Sample *sample) {
	LDData *data = (LDData *)(sample->samplerData);
	const RandomGenerator &rng(*(sample->rng));

	bool haveMoreSamples = true;
	++(data->samplePos);
	if (data->samplePos == pixelSamples) {
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
					LOG(LUX_ERROR, LUX_SYSTEM) << "Internal error in RandomSampler::GetNextSample()";
				}
			}
		}

		if ((data->noiseAwareMapVersion == 0) && (data->userSamplingMapVersion == 0)) {
			// Standard sampler using pixel sampler

			u_int sampPixelPosToUse;
			// Move to the next pixel
			{
				fast_mutex::scoped_lock lock(sampPixelPosMutex);
				sampPixelPosToUse = sampPixelPos;
				sampPixelPos = (sampPixelPos + 1) % totalPixels;
			}

			// fetch next pixel from pixelsampler
			if(!pixelSampler->GetNextPixel(&data->xPos, &data->yPos, sampPixelPosToUse)) {
				// Dade - we are at a valid checkpoint where we can stop the
				// rendering. Check if we have enough samples per pixel in the film.
				if (film->enoughSamplesPerPixel) {
					// Dade - pixelSampler->renderingDone is shared among all rendering threads
					pixelSampler->renderingDone = true;
					haveMoreSamples = false;
				}
			} else
				haveMoreSamples = (!pixelSampler->renderingDone);
		}

		data->samplePos = 0;
		// Generate low-discrepancy samples for pixel
		LDShuffleScrambled2D(rng, 1, pixelSamples, data->imageSamples);
		LDShuffleScrambled2D(rng, 1, pixelSamples, data->lensSamples);
		LDShuffleScrambled1D(rng, 1, pixelSamples, data->timeSamples);
		LDShuffleScrambled1D(rng, 1, pixelSamples, data->wavelengthsSamples);
		for (u_int i = 0; i < n1D.size(); ++i)
			LDShuffleScrambled1D(rng, n1D[i], pixelSamples,
				data->oneDSamples[i]);
		for (u_int i = 0; i < n2D.size(); ++i)
			LDShuffleScrambled2D(rng, n2D[i], pixelSamples,
				data->twoDSamples[i]);
		float *xDSamp;
		for (u_int i = 0; i < nxD.size(); ++i) {
			xDSamp = data->xDSamples[i];
			for (u_int j = 0; j < sxD[i].size(); ++j) {
				switch (sxD[i][j]) {
				case 1: {
					LDShuffleScrambled1D(rng, nxD[i],
						pixelSamples, xDSamp);
					xDSamp += nxD[i] * pixelSamples;
					break; }
				case 2: {
					LDShuffleScrambled2D(rng, nxD[i],
						pixelSamples, xDSamp);
					xDSamp += 2 * nxD[i] * pixelSamples;
					break; }
				default:
					LOG(LUX_ERROR, LUX_LIMIT) << "Unsupported dimension";
					xDSamp += sxD[i][j] * nxD[i] * pixelSamples;
					break;
				}
			}
		}
	}

	if ((data->noiseAwareMapVersion > 0) || (data->userSamplingMapVersion > 0)) {
		float uv[2], pdf;
		data->samplingDistribution2D->SampleContinuous(
			data->imageSamples[2 * data->samplePos],
			data->imageSamples[2 * data->samplePos + 1],
			uv, &pdf);
		sample->imageX = uv[0] * (xPixelEnd - xPixelStart) + xPixelStart;
		sample->imageY = uv[1] * (yPixelEnd - yPixelStart) + yPixelStart;

		if (film->enoughSamplesPerPixel)
			haveMoreSamples = false;
	} else {
		// Copy low-discrepancy samples from tables
		sample->imageX = data->xPos + data->imageSamples[2 * data->samplePos];
		sample->imageY = data->yPos + data->imageSamples[2 * data->samplePos + 1];
	}

	sample->lensU = data->lensSamples[2 * data->samplePos];
	sample->lensV = data->lensSamples[2 * data->samplePos + 1];
	sample->time = data->timeSamples[data->samplePos];
	sample->wavelengths = data->wavelengthsSamples[data->samplePos];

	return haveMoreSamples;
}

float LDSampler::GetOneD(const Sample &sample, u_int num, u_int pos)
{
	LDData *data = (LDData *)(sample.samplerData);
	return data->oneDSamples[num][n1D[num] * data->samplePos + pos];
}

void LDSampler::GetTwoD(const Sample &sample, u_int num, u_int pos, float u[2])
{
	LDData *data = (LDData *)(sample.samplerData);
	const u_int startSamp = 2 * (n2D[num] * data->samplePos + pos);
	u[0] = data->twoDSamples[num][startSamp];
	u[1] = data->twoDSamples[num][startSamp + 1];
}

float *LDSampler::GetLazyValues(const Sample &sample, u_int num, u_int pos)
{
	LDData *data = (LDData *)(sample.samplerData);
	float *sd = data->xD[num];
	float *xDSamp = data->xDSamples[num];
	u_int offset = 0;
	for (u_int i = 0; i < sxD[num].size(); ++i) {
		if (sxD[num][i] == 1) {
			sd[offset] = xDSamp[nxD[num] * data->samplePos + pos];
		} else if (sxD[num][i] == 2) {
			sd[offset] = xDSamp[2 * (nxD[num] * data->samplePos + pos)];
			sd[offset + 1] = xDSamp[2 * (nxD[num] * data->samplePos + pos) + 1];
		}
		xDSamp += sxD[num][i] * nxD[num] * pixelSamples;
		offset += sxD[num][i];
	}
	return sd;
}

Sampler* LDSampler::CreateSampler(const ParamSet &params, Film *film) {
	// Initialize common sampler parameters
	int xstart, xend, ystart, yend;
	film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
	string pixelsampler = params.FindOneString("pixelsampler", "vegas");
	int nsamp = params.FindOneInt("pixelsamples", 4);

	bool useNoiseAware = params.FindOneBool("noiseaware", false);
	if (useNoiseAware) {
		// Enable Film noise-aware map generation
		film->EnableNoiseAwareMap();
	}

	return new LDSampler(xstart, xend, ystart, yend, max(nsamp, 0), pixelsampler, useNoiseAware);
}

static DynamicLoader::RegisterSampler<LDSampler> r("lowdiscrepancy");
