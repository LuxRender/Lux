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

// random.cpp*
#include "random.h"
#include "vegas.h"
#include "randompx.h"
#include "lowdiscrepancypx.h"
#include "linear.h"
#include "tilepx.h"

using namespace lux;

// Lux (copy) constructor
RandomSampler* RandomSampler::clone() const
 {
   return new RandomSampler(*this);
 }
RandomSampler::RandomSampler(int xstart, int xend,
                             int ystart, int yend, int xs, int ys, string pixelsampler)
        : Sampler(xstart, xend, ystart, yend, xs * ys)
{
    xPos = xPixelStart;
    yPos = yPixelStart;
    xPixelSamples = xs;
    yPixelSamples = ys;

	// Initialize PixelSampler
	if(pixelsampler == "vegas")
		pixelSampler = new VegasPixelSampler(xstart, xend, ystart, yend);
	else if(pixelsampler == "lowdiscrepancy")
		pixelSampler = new LowdiscrepancyPixelSampler(xstart, xend, ystart, yend);
	else if(pixelsampler == "random")
		pixelSampler = new RandomPixelSampler(xstart, xend, ystart, yend);
    else if((pixelsampler == "tile") || (pixelsampler == "grid"))
		pixelSampler = new TilePixelSampler(xstart, xend, ystart, yend);
	else
		pixelSampler = new LinearPixelSampler(xstart, xend, ystart, yend);

	TotalPixels = pixelSampler->GetTotalPixels();

	// Get storage for a pixel's worth of stratified samples
	imageSamples = (float *)AllocAligned(7 * xPixelSamples * yPixelSamples *
		sizeof(float));
	lensSamples = imageSamples + 2 * xPixelSamples * yPixelSamples;
	timeSamples = lensSamples +  2 * xPixelSamples * yPixelSamples;
	wavelengthsSamples = timeSamples + xPixelSamples * yPixelSamples;
	singleWavelengthSamples = wavelengthsSamples + xPixelSamples * yPixelSamples;

	samplePos = xPixelSamples * yPixelSamples;
}

// return TotalPixels so scene shared thread increment knows total sample positions
u_int RandomSampler::GetTotalSamplePos()
{
	return TotalPixels;
}

bool RandomSampler::GetNextSample(Sample *sample, u_int *use_pos)
{
	sample->sampler = this;

	// Compute new set of samples if needed for next pixel
	bool haveMoreSample = true;
	if (samplePos == xPixelSamples * yPixelSamples) {
		// fetch next pixel from pixelsampler
		if(!pixelSampler->GetNextPixel(xPos, yPos, use_pos)) {
			// Dade - we are at a valid checkpoint where we can stop the
			// rendering. Check if we have enough samples per pixel in the film.
			if ((film->haltSamplePerPixel > 0)  && film->enoughSamplePerPixel)
				haveMoreSample = false;
		}

		for (int i = 0; i < 7 * xPixelSamples * yPixelSamples; ++i) {
			imageSamples[i] = lux::random::floatValue();
		}

		// Shift image samples to pixel coordinates
		for (int o = 0; o < 2 * xPixelSamples * yPixelSamples; o += 2) {
			imageSamples[o] += xPos;
			imageSamples[o + 1] += yPos;
		}
		samplePos = 0;
	}
	// reset so scene knows to increment
	if (samplePos >= xPixelSamples * yPixelSamples-1)
		*use_pos = -1;
	// Return next \mono{RandomSampler} sample point
	sample->imageX = imageSamples[2*samplePos];
	sample->imageY = imageSamples[2*samplePos+1];
	sample->lensU = lensSamples[2*samplePos];
	sample->lensV = lensSamples[2*samplePos+1];
	sample->time = timeSamples[samplePos];
	sample->wavelengths = wavelengthsSamples[samplePos];
	sample->singleWavelength = singleWavelengthSamples[samplePos];
	// Generate stratified samples for integrators
	for (u_int i = 0; i < sample->n1D.size(); ++i) {
		for (u_int j = 0; j < sample->n1D[i]; ++j)
			sample->oneD[i][j] = lux::random::floatValue();
	}
	for (u_int i = 0; i < sample->n2D.size(); ++i) {
		for (u_int j = 0; j < 2*sample->n2D[i]; ++j)
			sample->twoD[i][j] = lux::random::floatValue();
	}

	++samplePos;

	return haveMoreSample;
}

float *RandomSampler::GetLazyValues(Sample *sample, u_int num, u_int pos)
{
	float *data = sample->xD[num] + pos * sample->dxD[num];
	for (u_int i = 0; i < sample->dxD[num]; ++i)
		data[i] = lux::random::floatValue();
	return data;
}

Sampler* RandomSampler::CreateSampler(const ParamSet &params, const Film *film)
{
    int xsamp = params.FindOneInt("xsamples", 2);
    int ysamp = params.FindOneInt("ysamples", 2);
	string pixelsampler = params.FindOneString("pixelsampler", "vegas");
    int xstart, xend, ystart, yend;
    film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
    return new RandomSampler(xstart, xend,
                             ystart, yend,
                             xsamp, ysamp, pixelsampler);
}
