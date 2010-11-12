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

// random.cpp*
#include "random.h"
#include "pixelsamplers/vegas.h"
//#include "pixelsamplers/randompx.h"
#include "pixelsamplers/hilbertpx.h"
#include "pixelsamplers/lowdiscrepancypx.h"
#include "pixelsamplers/linear.h"
#include "pixelsamplers/tilepx.h"
#include "scene.h"
#include "dynload.h"
#include "error.h"

using namespace lux;

RandomSampler::RandomData::RandomData(int xPixelStart, int yPixelStart,
	u_int pixelSamples)
{
	xPos = xPixelStart;
	yPos = yPixelStart;
	samplePos = pixelSamples;
}
RandomSampler::RandomData::~RandomData()
{
}

RandomSampler::RandomSampler(int xstart, int xend,
                             int ystart, int yend, u_int ps, string pixelsampler)
        : Sampler(xstart, xend, ystart, yend, ps)
{
	pixelSamples = ps;

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
	sampPixelPos = 0;
}

RandomSampler::~RandomSampler()
{
}

// return TotalPixels so scene shared thread increment knows total sample positions
u_int RandomSampler::GetTotalSamplePos()
{
	return totalPixels;
}

bool RandomSampler::GetNextSample(Sample *sample)
{
	RandomData *data = (RandomData *)(sample->samplerData);

	// Compute new set of samples if needed for next pixel
	bool haveMoreSamples = true;
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
			if (film->enoughSamplesPerPixel) {
				// Dade - pixelSampler->renderingDone is shared among all rendering threads
				pixelSampler->renderingDone = true;
				haveMoreSamples = false;
			}
		} else
			haveMoreSamples = (!pixelSampler->renderingDone);

		data->samplePos = 0;
	}

	// Return next \mono{RandomSampler} sample point
	sample->imageX = data->xPos + sample->rng->floatValue();
	sample->imageY = data->yPos + sample->rng->floatValue();
	sample->lensU = sample->rng->floatValue();
	sample->lensV = sample->rng->floatValue();
	sample->time = sample->rng->floatValue();
	sample->wavelengths = sample->rng->floatValue();
	// Generate stratified samples for integrators
	for (u_int i = 0; i < sample->n1D.size(); ++i) {
		for (u_int j = 0; j < sample->n1D[i]; ++j)
			sample->oneD[i][j] = sample->rng->floatValue();
	}
	for (u_int i = 0; i < sample->n2D.size(); ++i) {
		for (u_int j = 0; j < 2*sample->n2D[i]; ++j)
			sample->twoD[i][j] = sample->rng->floatValue();
	}

	++(data->samplePos);

	return haveMoreSamples;
}

float *RandomSampler::GetLazyValues(const Sample &sample, u_int num, u_int pos)
{
	float *data = sample.xD[num] + pos * sample.dxD[num];
	for (u_int i = 0; i < sample.dxD[num]; ++i)
		data[i] = sample.rng->floatValue();
	return data;
}

Sampler* RandomSampler::CreateSampler(const ParamSet &params, const Film *film)
{
	int nsamp = params.FindOneInt("pixelsamples", -1);
	// for backwards compatibility
	if (nsamp < 0) {
		LOG( LUX_WARNING,LUX_NOERROR)<< "Parameters 'xsamples' and 'ysamples' are deprecated, use 'pixelsamples' instead";
		int xsamp = params.FindOneInt("xsamples", 2);
		int ysamp = params.FindOneInt("ysamples", 2);
		nsamp = xsamp*ysamp;
	}

	string pixelsampler = params.FindOneString("pixelsampler", "vegas");
    int xstart, xend, ystart, yend;
    film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
    return new RandomSampler(xstart, xend,
                             ystart, yend,
                             max(nsamp, 0), pixelsampler);
}

static DynamicLoader::RegisterSampler<RandomSampler> r("random");
