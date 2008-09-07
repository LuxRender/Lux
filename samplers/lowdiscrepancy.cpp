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
 
// lowdiscrepancy.cpp*
#include "lowdiscrepancy.h"
#include "error.h"
#include "vegas.h"
//#include "randompx.h"
#include "lowdiscrepancypx.h"
#include "tilepx.h"
#include "linear.h"
#include "hilbertpx.h"
#include "dynload.h"

using namespace lux;

// Lux (copy) constructor
LDSampler* LDSampler::clone() const
 {
   return new LDSampler(*this);
 }

// LDSampler Method Definitions
LDSampler::LDSampler(int xstart, int xend,
		int ystart, int yend, int ps, string pixelsampler)
	: Sampler(xstart, xend, ystart, yend, RoundUpPow2(ps)) {
	xPos = xPixelStart - 1;
	yPos = yPixelStart;

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

	TotalPixels = pixelSampler->GetTotalPixels();

	// check/round pixelsamples to power of 2
	if (!IsPowerOf2(ps)) {
		luxError(LUX_CONSISTENCY,LUX_WARNING,"Pixel samples being rounded up to power of 2");
		pixelSamples = RoundUpPow2(ps);
	}
	else
		pixelSamples = ps;
	samplePos = pixelSamples;
	oneDSamples = twoDSamples = xDSamples = NULL;
	// Dade - should use AllocAligned()
	imageSamples = new float[7*pixelSamples];
	lensSamples = imageSamples + 2*pixelSamples;
	timeSamples = imageSamples + 4*pixelSamples;
	wavelengthsSamples = imageSamples + 5*pixelSamples;
	singleWavelengthSamples = imageSamples + 6*pixelSamples;
	n1D = n2D = nxD = 0;
}

LDSampler::~LDSampler() {
	delete[] imageSamples;
	for (int i = 0; i < n1D; ++i)
		delete[] oneDSamples[i];
	for (int i = 0; i < n2D; ++i)
		delete[] twoDSamples[i];
	for (int i = 0; i < nxD; ++i)
		delete[] xDSamples[i];
	delete[] oneDSamples;
	delete[] twoDSamples;
	delete[] xDSamples;
}

// return TotalPixels so scene shared thread increment knows total sample positions
u_int LDSampler::GetTotalSamplePos() {
	return TotalPixels;
}

bool LDSampler::GetNextSample(Sample *sample, u_int *use_pos) {
	sample->sampler = this;
	if (!oneDSamples) {
		// Allocate space for pixel's low-discrepancy sample tables
		oneDSamples = new float *[sample->n1D.size()];
		n1D = sample->n1D.size();
		for (u_int i = 0; i < sample->n1D.size(); ++i)
			oneDSamples[i] = new float[sample->n1D[i] *
		                               pixelSamples];
		twoDSamples = new float *[sample->n2D.size()];
		n2D = sample->n2D.size();
		for (u_int i = 0; i < sample->n2D.size(); ++i)
			twoDSamples[i] = new float[2 * sample->n2D[i] *
		                               pixelSamples];
		xDSamples = new float *[sample->nxD.size()];
		nxD = sample->nxD.size();
		for (u_int i = 0; i < sample->nxD.size(); ++i)
			xDSamples[i] = new float[sample->dxD[i] * sample->nxD[i] *
		                               pixelSamples];
	}

	bool haveMoreSample = true;
	if (samplePos == pixelSamples) {
		// fetch next pixel from pixelsampler
		if(!pixelSampler->GetNextPixel(xPos, yPos, use_pos)) {
			// Dade - we are at a valid checkpoint where we can stop the
			// rendering. Check if we have enough samples per pixel in the film.
			if (film->enoughSamplePerPixel) {
				// Dade - pixelSampler->renderingDone is shared among all rendering threads
				pixelSampler->renderingDone = true;
				haveMoreSample = false;
			}
		} else
			haveMoreSample = (!pixelSampler->renderingDone);

		samplePos = 0;
		// Generate low-discrepancy samples for pixel
		LDShuffleScrambled2D(tspack, 1, pixelSamples, imageSamples);
		LDShuffleScrambled2D(tspack, 1, pixelSamples, lensSamples);
		LDShuffleScrambled1D(tspack, 1, pixelSamples, timeSamples);
		LDShuffleScrambled1D(tspack, 1, pixelSamples, wavelengthsSamples);
		for (u_int i = 0; i < sample->n1D.size(); ++i)
			LDShuffleScrambled1D(tspack, sample->n1D[i], pixelSamples,
				oneDSamples[i]);
		for (u_int i = 0; i < sample->n2D.size(); ++i)
			LDShuffleScrambled2D(tspack, sample->n2D[i], pixelSamples,
				twoDSamples[i]);
		float *xDSamp;
		for (u_int i = 0; i < sample->nxD.size(); ++i) {
			xDSamp = xDSamples[i];
			for (u_int j = 0; j < sample->sxD[i].size(); ++j) {
				switch (sample->sxD[i][j]) {
				case 1: {
					LDShuffleScrambled1D(tspack, sample->nxD[i],
						pixelSamples, xDSamp);
					xDSamp += sample->nxD[i] * pixelSamples;
					break; }
				case 2: {
					LDShuffleScrambled2D(tspack, sample->nxD[i],
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

	// reset so scene knows to increment
	if (samplePos >= pixelSamples - 1)
		*use_pos = -1;
	// Copy low-discrepancy samples from tables
	sample->imageX = xPos + imageSamples[2*samplePos];
	sample->imageY = yPos + imageSamples[2*samplePos+1];
	sample->lensU = lensSamples[2*samplePos];
	sample->lensV = lensSamples[2*samplePos+1];
	sample->time = timeSamples[samplePos];
	sample->wavelengths = wavelengthsSamples[samplePos];
	sample->singleWavelength = tspack->rng->floatValue();//singleWavelengthSamples[samplePos]
	for (u_int i = 0; i < sample->n1D.size(); ++i) {
		int startSamp = sample->n1D[i] * samplePos;
		for (u_int j = 0; j < sample->n1D[i]; ++j)
			sample->oneD[i][j] = oneDSamples[i][startSamp+j];
	}
	for (u_int i = 0; i < sample->n2D.size(); ++i) {
		int startSamp = 2 * sample->n2D[i] * samplePos;
		for (u_int j = 0; j < 2*sample->n2D[i]; ++j)
			sample->twoD[i][j] = twoDSamples[i][startSamp+j];
	}
	++samplePos;

	return haveMoreSample;
}

float *LDSampler::GetLazyValues(Sample *sample, u_int num, u_int pos)
{
	float *data = sample->xD[num] + pos * sample->dxD[num];
	float *xDSamp = xDSamples[num];
	int offset = 0;
	for (u_int i = 0; i < sample->sxD[num].size(); ++i) {
		if (sample->sxD[num][i] == 1) {
			data[offset] = xDSamp[sample->nxD[num] * (samplePos - 1) + pos];
		} else if (sample->sxD[num][i] == 2) {
			data[offset] = xDSamp[2 * (sample->nxD[num] * (samplePos - 1) + pos)];
			data[offset + 1] = xDSamp[2 * (sample->nxD[num] * (samplePos - 1) + pos) + 1];
		}
		xDSamp += sample->sxD[num][i] * sample->nxD[num] * pixelSamples;
		offset += sample->sxD[num][i];
	}
	return data;
}

Sampler* LDSampler::CreateSampler(const ParamSet &params, const Film *film) {
	// Initialize common sampler parameters
	int xstart, xend, ystart, yend;
	film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
	string pixelsampler = params.FindOneString("pixelsampler", "vegas");
	int nsamp = params.FindOneInt("pixelsamples", 4);
	return new LDSampler(xstart, xend, ystart, yend, nsamp, pixelsampler);
}

static DynamicLoader::RegisterSampler<LDSampler> r("lowdiscrepancy");
