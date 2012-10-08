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
#include "scene.h"
#include "dynload.h"
#include "error.h"

using namespace lux;

RandomSampler::RandomData::RandomData(const Sampler &sampler, int xPixelStart,
	int yPixelStart, u_int pixelSamples) :
	samplingMap(NULL), noiseAwareMapVersion(0), userSamplingMapVersion(0)
{
	xPos = xPixelStart;
	yPos = yPixelStart;
	samplePos = pixelSamples;
	xD = new float *[sampler.nxD.size()];
	nxD = sampler.nxD.size();
	for (u_int i = 0; i < sampler.nxD.size(); ++i)
		xD[i] = new float[sampler.dxD[i]];
}
RandomSampler::RandomData::~RandomData()
{
	for (u_int i = 0; i < nxD; ++i)
		delete[] xD[i];
	delete[] xD;
}

RandomSampler::RandomSampler(int xstart, int xend, int ystart, int yend,
	u_int ps, string pixelsampler, bool useNoise) :
	Sampler(xstart, xend, ystart, yend, ps), useNoiseAware(useNoise)
{
	pixelSamples = ps;

	// Initialize PixelSampler
	pixelSampler = MakePixelSampler(pixelsampler, xstart, xend, ystart, yend);

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

		if ((data->noiseAwareMapVersion > 0) || (data->userSamplingMapVersion > 0)) {
			float uv[2], pdf;
			data->samplingDistribution2D->SampleContinuous(sample->rng->floatValue(), sample->rng->floatValue(), uv, &pdf);
			sample->imageX = uv[0] * (xPixelEnd - xPixelStart) + xPixelStart;
			sample->imageY = uv[1] * (yPixelEnd - yPixelStart) + yPixelStart;

			if (film->enoughSamplesPerPixel) {
				// Dade - pixelSampler->renderingDone is shared among all rendering threads
				pixelSampler->renderingDone = true;
				haveMoreSamples = false;
			}
		} else {
			// Maps aren't yet ready, use pixel sampler

			u_int sampPixelPosToUse;
			// Move to the next pixel
			{
				fast_mutex::scoped_lock lock(sampPixelPosMutex);
				sampPixelPosToUse = sampPixelPos;
				sampPixelPos = (sampPixelPos + 1) % totalPixels;
			}

			pixelSampler->GetNextPixel(&data->xPos, &data->yPos, sampPixelPosToUse);
			sample->imageX = data->xPos + sample->rng->floatValue();
			sample->imageY = data->yPos + sample->rng->floatValue();
			++(data->samplePos);

			haveMoreSamples = true;
		}
	} else {
		// Standard sampler using pixel sampler

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

		sample->imageX = data->xPos + sample->rng->floatValue();
		sample->imageY = data->yPos + sample->rng->floatValue();
		++(data->samplePos);
	}

	// Return next \mono{RandomSampler} sample point
	sample->lensU = sample->rng->floatValue();
	sample->lensV = sample->rng->floatValue();
	sample->time = sample->rng->floatValue();
	sample->wavelengths = sample->rng->floatValue();

	return haveMoreSamples;
}

float RandomSampler::GetOneD(const Sample &sample, u_int num, u_int pos)
{
	return sample.rng->floatValue();
}

void RandomSampler::GetTwoD(const Sample &sample, u_int num, u_int pos, float u[2])
{
	u[0] = sample.rng->floatValue();
	u[1] = sample.rng->floatValue();
}

float *RandomSampler::GetLazyValues(const Sample &sample, u_int num, u_int pos)
{
	RandomData *data = (RandomData *)(sample.samplerData);
	float *sd = data->xD[num];
	for (u_int i = 0; i < dxD[num]; ++i)
		sd[i] = sample.rng->floatValue();
	return sd;
}

Sampler* RandomSampler::CreateSampler(const ParamSet &params, Film *film)
{
	// for backwards compatibility
	int nsamp = params.FindOneInt("pixelsamples", 4);

	int xsamp = params.FindOneInt("xsamples", -1);
	int ysamp = params.FindOneInt("ysamples", -1);

	if (xsamp >= 0 || ysamp >= 0) {
		LOG(LUX_WARNING, LUX_NOERROR) << "Parameters 'xsamples' and 'ysamples' are deprecated, use 'pixelsamples' instead";		
		nsamp = (xsamp < 0 ? 2 : xsamp) * (ysamp < 0 ? 2 : ysamp);
	}

	bool useNoiseAware = params.FindOneBool("noiseaware", false);
	if (useNoiseAware) {
		// Enable Film noise-aware map generation
		film->EnableNoiseAwareMap();
	}

	string pixelsampler = params.FindOneString("pixelsampler", "vegas");
    int xstart, xend, ystart, yend;
    film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
    return new RandomSampler(xstart, xend,
                             ystart, yend,
                             max(nsamp, 1), pixelsampler, useNoiseAware);
}

static DynamicLoader::RegisterSampler<RandomSampler> r("random");
