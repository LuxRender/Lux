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
 
// lowdiscrepancy.cpp*
#include "lowdiscrepancy.h"
#include "error.h"
#include "pixelsamplers/vegas.h"
//#include "pixelsamplers/randompx.h"
#include "pixelsamplers/hilbertpx.h"
#include "pixelsamplers/linear.h"
#include "pixelsamplers/lowdiscrepancypx.h"
#include "pixelsamplers/tilepx.h"
#include "scene.h"
#include "dynload.h"

using namespace lux;

LDSampler::LDData::LDData(const Sample &sample, int xPixelStart, int yPixelStart, u_int pixelSamples)
{
	xPos = xPixelStart - 1;
	yPos = yPixelStart;
	samplePos = pixelSamples;

	// Allocate space for pixel's low-discrepancy sample tables
	imageSamples = new float[6 * pixelSamples];
	lensSamples = imageSamples + 2 * pixelSamples;
	timeSamples = imageSamples + 4 * pixelSamples;
	wavelengthsSamples = imageSamples + 5 * pixelSamples;
	oneDSamples = new float *[sample.n1D.size()];
	n1D = sample.n1D.size();
	for (u_int i = 0; i < sample.n1D.size(); ++i)
		oneDSamples[i] = new float[sample.n1D[i] * pixelSamples];
	twoDSamples = new float *[sample.n2D.size()];
	n2D = sample.n2D.size();
	for (u_int i = 0; i < sample.n2D.size(); ++i)
		twoDSamples[i] = new float[2 * sample.n2D[i] * pixelSamples];
	xDSamples = new float *[sample.nxD.size()];
	nxD = sample.nxD.size();
	for (u_int i = 0; i < sample.nxD.size(); ++i)
		xDSamples[i] = new float[sample.dxD[i] * sample.nxD[i] *
			pixelSamples];
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
		int ystart, int yend, u_int ps, string pixelsampler)
	: Sampler(xstart, xend, ystart, yend, RoundUpPow2(ps)) {
	// Initialize PixelSampler
	if(pixelsampler == "vegas")
		pixelSampler = new VegasPixelSampler(xstart, xend, ystart, yend);
	else if(pixelsampler == "lowdiscrepancy")
		pixelSampler = new LowdiscrepancyPixelSampler(xstart, xend, ystart, yend);
//	else if(pixelsampler == "random")
//		pixelSampler = new RandomPixelSampler(xstart, xend, ystart, yend);
	else if((pixelsampler == "tile") || (pixelsampler == "grid"))
		pixelSampler = new TilePixelSampler(xstart, xend, ystart, yend);
	else if(pixelsampler == "hilbert")
		pixelSampler = new HilbertPixelSampler(xstart, xend, ystart, yend);
	else
		pixelSampler = new LinearPixelSampler(xstart, xend, ystart, yend);

	totalPixels = pixelSampler->GetTotalPixels();

	// check/round pixelsamples to power of 2
	if (!IsPowerOf2(ps)) {
		luxError(LUX_CONSISTENCY,LUX_WARNING,"Pixel samples being rounded up to power of 2");
		pixelSamples = RoundUpPow2(ps);
	} else
		pixelSamples = ps;
	sampPixelPos = 0;
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

	bool haveMoreSample = true;
	if (data->samplePos == pixelSamples) {
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
			if (film->enoughSamplePerPixel) {
				// Dade - pixelSampler->renderingDone is shared among all rendering threads
				pixelSampler->renderingDone = true;
				haveMoreSample = false;
			}
		} else
			haveMoreSample = (!pixelSampler->renderingDone);

		data->samplePos = 0;
		// Generate low-discrepancy samples for pixel
		LDShuffleScrambled2D(rng, 1, pixelSamples, data->imageSamples);
		LDShuffleScrambled2D(rng, 1, pixelSamples, data->lensSamples);
		LDShuffleScrambled1D(rng, 1, pixelSamples, data->timeSamples);
		LDShuffleScrambled1D(rng, 1, pixelSamples, data->wavelengthsSamples);
		for (u_int i = 0; i < sample->n1D.size(); ++i)
			LDShuffleScrambled1D(rng, sample->n1D[i], pixelSamples,
				data->oneDSamples[i]);
		for (u_int i = 0; i < sample->n2D.size(); ++i)
			LDShuffleScrambled2D(rng, sample->n2D[i], pixelSamples,
				data->twoDSamples[i]);
		float *xDSamp;
		for (u_int i = 0; i < sample->nxD.size(); ++i) {
			xDSamp = data->xDSamples[i];
			for (u_int j = 0; j < sample->sxD[i].size(); ++j) {
				switch (sample->sxD[i][j]) {
				case 1: {
					LDShuffleScrambled1D(rng, sample->nxD[i],
						pixelSamples, xDSamp);
					xDSamp += sample->nxD[i] * pixelSamples;
					break; }
				case 2: {
					LDShuffleScrambled2D(rng, sample->nxD[i],
						pixelSamples, xDSamp);
					xDSamp += 2 * sample->nxD[i] * pixelSamples;
					break; }
				default:
					printf("Unsupported dimension\n");
					xDSamp += sample->sxD[i][j] * sample->nxD[i] * pixelSamples;
					break;
				}
			}
		}
	}

	// Copy low-discrepancy samples from tables
	sample->imageX = data->xPos + data->imageSamples[2 * data->samplePos];
	sample->imageY = data->yPos + data->imageSamples[2 * data->samplePos + 1];
	sample->lensU = data->lensSamples[2 * data->samplePos];
	sample->lensV = data->lensSamples[2 * data->samplePos + 1];
	sample->time = data->timeSamples[data->samplePos];
	sample->wavelengths = data->wavelengthsSamples[data->samplePos];
	for (u_int i = 0; i < sample->n1D.size(); ++i) {
		u_int startSamp = sample->n1D[i] * data->samplePos;
		for (u_int j = 0; j < sample->n1D[i]; ++j)
			sample->oneD[i][j] = data->oneDSamples[i][startSamp+j];
	}
	for (u_int i = 0; i < sample->n2D.size(); ++i) {
		u_int startSamp = 2 * sample->n2D[i] * data->samplePos;
		for (u_int j = 0; j < 2 * sample->n2D[i]; ++j)
			sample->twoD[i][j] = data->twoDSamples[i][startSamp+j];
	}
	++(data->samplePos);

	return haveMoreSample;
}

float *LDSampler::GetLazyValues(const Sample &sample, u_int num, u_int pos)
{
	LDData *data = (LDData *)(sample.samplerData);
	float *sd = sample.xD[num] + pos * sample.dxD[num];
	float *xDSamp = data->xDSamples[num];
	u_int offset = 0;
	for (u_int i = 0; i < sample.sxD[num].size(); ++i) {
		if (sample.sxD[num][i] == 1) {
			sd[offset] = xDSamp[sample.nxD[num] * (data->samplePos - 1) + pos];
		} else if (sample.sxD[num][i] == 2) {
			sd[offset] = xDSamp[2 * (sample.nxD[num] * (data->samplePos - 1) + pos)];
			sd[offset + 1] = xDSamp[2 * (sample.nxD[num] * (data->samplePos - 1) + pos) + 1];
		}
		xDSamp += sample.sxD[num][i] * sample.nxD[num] * pixelSamples;
		offset += sample.sxD[num][i];
	}
	return sd;
}

Sampler* LDSampler::CreateSampler(const ParamSet &params, const Film *film) {
	// Initialize common sampler parameters
	int xstart, xend, ystart, yend;
	film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
	string pixelsampler = params.FindOneString("pixelsampler", "vegas");
	int nsamp = params.FindOneInt("pixelsamples", 4);
	return new LDSampler(xstart, xend, ystart, yend, max(nsamp, 0), pixelsampler);
}

static DynamicLoader::RegisterSampler<LDSampler> r("lowdiscrepancy");
