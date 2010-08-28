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

// initial metropolis light transport sample integrator
// by radiance

// TODO: add scaling of output image samples

// erpt.cpp*
#include "erpt.h"
#include "dynload.h"
#include "scene.h"
#include "error.h"

using namespace lux;

#define SAMPLE_FLOATS 6

// mutate a value in the range [0-1]
static float mutate(const float x, const float randomValue)
{
	static const float s1 = 1.f / 1024.f, s2 = 1.f / 16.f;
	float dx = s2 * powf(s1 / s2, fabsf(2.f * randomValue - 1.f));
	if (randomValue < 0.5f) {
		float x1 = x + dx;
		return (x1 > 1.f) ? x1 - 1.f : x1;
	} else {
		float x1 = x - dx;
		return (x1 < 0.f) ? x1 + 1.f : x1;
	}
}

// mutate a value in the range [min-max]
static float mutateScaled(const float x, const float randomValue, const float mini, const float maxi, const float range)
{
	float dx = range * expf(-logf(2.f * range) * fabsf(2.f * randomValue - 1.f));
	if (randomValue < 0.5f) {
		float x1 = x + dx;
		return (x1 > maxi) ? x1 - maxi + mini : x1;
	} else {
		float x1 = x - dx;
		return (x1 < mini) ? x1 - mini + maxi : x1;
	}
}

ERPTSampler::ERPTData::ERPTData(const Sample &sample) :
	currentImage(NULL), currentTimeImage(NULL), numChains(0), chain(0),
	mutation(~0U), stamp(0), baseLY(0.f), quantum(0.f), weight(0.f),
	LY(0.f), alpha(0.f), totalLY(0.), sampleCount(0.)
{
	u_int i;
	normalSamples = SAMPLE_FLOATS;
	for (i = 0; i < sample.n1D.size(); ++i)
		normalSamples += sample.n1D[i];
	for (i = 0; i < sample.n2D.size(); ++i)
		normalSamples += 2 * sample.n2D[i];
	totalSamples = normalSamples;
	offset = new u_int[sample.nxD.size()];
	totalTimes = 0;
	for (i = 0; i < sample.nxD.size(); ++i) {
		offset[i] = totalSamples;
		totalTimes += sample.nxD[i];
		totalSamples += sample.dxD[i] * sample.nxD[i];
	}
	sampleImage = AllocAligned<float>(totalSamples);
	baseImage = AllocAligned<float>(totalSamples);
	timeImage = AllocAligned<int>(totalTimes);
	baseTimeImage = AllocAligned<int>(totalTimes);
}
ERPTSampler::ERPTData::~ERPTData()
{
	FreeAligned(baseTimeImage);
	FreeAligned(timeImage);
	FreeAligned(baseImage);
	FreeAligned(sampleImage);
	delete[] offset;
}

// Metropolis method definitions
ERPTSampler::ERPTSampler(u_int totMutations, float rng, Sampler *sampler) :
	Sampler(sampler->xPixelStart, sampler->xPixelEnd,
	sampler->yPixelStart, sampler->yPixelEnd, sampler->samplesPerPixel),
	totalMutations(totMutations), range(rng), baseSampler(sampler)
{
}

ERPTSampler::~ERPTSampler() {
	delete baseSampler;
}

// interface for new ray/samples from scene
bool ERPTSampler::GetNextSample(Sample *sample, u_int *use_pos)
{
	const RandomGenerator *rng = sample->rng;
	ERPTData *data = (ERPTData *)(sample->samplerData);

	if (data->mutation == ~0U) {
		// Dade - we are at a valid checkpoint where we can stop the
		// rendering. Check if we have enough samples per pixel in the film.
		if (film->enoughSamplePerPixel)
			return false;

		const bool ret = baseSampler->GetNextSample(sample, use_pos);
		for (u_int i = 0; i < data->totalTimes; ++i)
			sample->timexD[0][i] = -1;
		sample->stamp = 0;
		data->currentImage = data->baseImage;
		data->currentTimeImage = data->baseTimeImage;
		data->stamp = 0;
		return ret;
	} else {
		if (data->mutation == 0) {
			// *** new chain ***
			for (u_int i = 0; i < data->totalTimes; ++i)
				sample->timexD[0][i] = data->baseTimeImage[i];
			sample->stamp = 0;
			data->currentImage = data->baseImage;
			data->currentTimeImage = data->baseTimeImage;
			data->stamp = 0;
		}
		// *** small mutation ***
		// mutate current sample
		sample->imageX = mutateScaled(data->currentImage[0], rng->floatValue(), xPixelStart, xPixelEnd, range);
		sample->imageY = mutateScaled(data->currentImage[1], rng->floatValue(), yPixelStart, yPixelEnd, range);
		sample->lensU = mutate(data->currentImage[2], rng->floatValue());
		sample->lensV = mutate(data->currentImage[3], rng->floatValue());
		sample->time = mutate(data->currentImage[4], rng->floatValue());
		sample->wavelengths = mutate(data->currentImage[5], rng->floatValue());
		for (u_int i = SAMPLE_FLOATS; i < data->normalSamples; ++i)
				sample->oneD[0][i - SAMPLE_FLOATS] = mutate(data->currentImage[i], rng->floatValue());
		++(sample->stamp);
	}

	return true;
}

float *ERPTSampler::GetLazyValues(const Sample &sample, u_int num, u_int pos)
{
	ERPTData *data = (ERPTData *)(sample.samplerData);
	const u_int size = sample.dxD[num];
	float *sd = sample.xD[num] + pos * size;
	const int stampLimit = sample.stamp;
	if (sample.timexD[num][pos] != stampLimit) {
		if (sample.timexD[num][pos] == -1) {
			baseSampler->GetLazyValues(sample, num, pos);
			sample.timexD[num][pos] = 0;
		} else {
			const u_int start = data->offset[num] + pos * size;
			float *image = data->currentImage + start;
			for (u_int i = 0; i < size; ++i)
				sd[i] = image[i];
		}
		for (int &time = sample.timexD[num][pos]; time < stampLimit; ++time) {
			for (u_int i = 0; i < size; ++i)
				sd[i] = mutate(sd[i], sample.rng->floatValue());
		}
	}
	return sd;
}

// interface for adding/accepting a new image sample.
void ERPTSampler::AddSample(const Sample &sample, Scene &scene)
{
	ERPTData *data = (ERPTData *)(sample.samplerData);
	vector<Contribution> &newContributions(sample.contributions);
	float newLY = 0.0f;
	for(u_int i = 0; i < newContributions.size(); ++i)
		newLY += newContributions[i].color.Y();
	if (data->mutation == 0U || data->mutation == ~0U) {
		if (data->weight > 0.f) {
			// Add accumulated contribution of previous reference sample
			data->weight *= data->quantum / data->LY;
			if (!isinf(data->weight) && data->LY > 0.f) {
				for(u_int i = 0; i < data->oldContributions.size(); ++i) {
					// Radiance - added new use of contributionpool/buffers
					if(!sample.contribBuffer->Add(&(data->oldContributions[i]), data->weight)) {
						sample.contribBuffer = scene.contribPool->Next(sample.contribBuffer);
						sample.contribBuffer->Add(&(data->oldContributions[i]), data->weight);
					}
				}
			}
			data->weight = 0.f;
		}
		if (data->mutation == ~0U) {
			if (!(newLY > 0.f)) {
				newContributions.clear();
				return;
			}
			sample.contribBuffer->AddSampleCount(1.f);
			++(data->sampleCount);
			data->totalLY += newLY;
			const float meanIntensity = data->totalLY > 0. ? static_cast<float>(data->totalLY / data->sampleCount) : 1.f;
			// calculate the number of chains on a new seed
			data->quantum = newLY / meanIntensity;
			data->numChains = max(1U, Floor2UInt(data->quantum + .5f));
			// The following line avoids to block on a pixel
			// if the initial sample is extremely bright
			data->numChains = min(data->numChains, totalMutations);
			data->quantum /= (data->numChains * totalMutations);
			data->baseLY = newLY;
			data->baseContributions = newContributions;
			data->baseImage[0] = sample.imageX;
			data->baseImage[1] = sample.imageY;
			data->baseImage[2] = sample.lensU;
			data->baseImage[3] = sample.lensV;
			data->baseImage[4] = sample.time;
			data->baseImage[5] = sample.wavelengths;
			for (u_int i = SAMPLE_FLOATS; i < data->totalSamples; ++i)
				data->baseImage[i] = sample.oneD[0][i - SAMPLE_FLOATS];
			for (u_int i = 0 ; i < data->totalTimes; ++i)
				data->baseTimeImage[i] = sample.timexD[0][i];
			data->mutation = 0;
			newContributions.clear();
			return;
		}
		data->LY = data->baseLY;
		data->oldContributions = data->baseContributions;
	}
	// calculate accept probability from old and new image sample
	float accProb;
	if (data->LY > 0.f)
		accProb = min(1.f, newLY / data->LY);
	else
		accProb = 1.f;
	float newWeight = accProb;
	data->weight += 1.f - accProb;

	// try accepting of the new sample
	if (accProb == 1.f || sample.rng->floatValue() < accProb) {
		// Add accumulated contribution of previous reference sample
		data->weight *= data->quantum / data->LY;
		if (!isinf(data->weight) && data->LY > 0.f) {
			for(u_int i = 0; i < data->oldContributions.size(); ++i) {
				// Radiance - added new use of contributionpool/buffers
				if(!sample.contribBuffer->Add(&(data->oldContributions[i]), data->weight)) {
					sample.contribBuffer = scene.contribPool->Next(sample.contribBuffer);
					sample.contribBuffer->Add(&(data->oldContributions[i]), data->weight);
				}
			}
		}
		data->weight = newWeight;
		data->LY = newLY;
		data->oldContributions = newContributions;

		// Save new contributions for reference
		data->sampleImage[0] = sample.imageX;
		data->sampleImage[1] = sample.imageY;
		data->sampleImage[2] = sample.lensU;
		data->sampleImage[3] = sample.lensV;
		data->sampleImage[4] = sample.time;
		data->sampleImage[5] = sample.wavelengths;
		for (u_int i = SAMPLE_FLOATS; i < data->totalSamples; ++i)
			data->sampleImage[i] = sample.oneD[0][i - SAMPLE_FLOATS];
		for (u_int i = 0 ; i < data->totalTimes; ++i)
			data->timeImage[i] = sample.timexD[0][i];
		data->stamp = sample.stamp;
		data->currentImage = data->sampleImage;
		data->currentTimeImage = data->timeImage;
	} else {
		// Add contribution of new sample before rejecting it
		newWeight *= data->quantum / newLY;
		if (!isinf(newWeight) && newLY > 0.f) {
			for(u_int i = 0; i < newContributions.size(); ++i) {
				// Radiance - added new use of contributionpool/buffers
				if(!sample.contribBuffer->Add(&newContributions[i], newWeight)) {
					sample.contribBuffer = scene.contribPool->Next(sample.contribBuffer);
					sample.contribBuffer->Add(&newContributions[i], newWeight);
				}
			}
		}

		// Restart from previous reference
		for (u_int i = 0; i < data->totalTimes; ++i)
			sample.timexD[0][i] = data->currentTimeImage[i];
		sample.stamp = data->stamp;
	}
	if (++(data->mutation) >= totalMutations) {
		data->mutation = 0;
		if (++(data->chain) >= data->numChains) {
			data->chain = 0;
			data->mutation = ~0U;
		}
	}
	newContributions.clear();
}

Sampler* ERPTSampler::CreateSampler(const ParamSet &params, const Film *film)
{
	int xStart, xEnd, yStart, yEnd;
	film->GetSampleExtent(&xStart, &xEnd, &yStart, &yEnd);
	int totMutations = params.FindOneInt("chainlength", 100);	// number of mutations from a given seed
	float range = params.FindOneFloat("mutationrange", (xEnd - xStart + yEnd - yStart) / 50.f);	// maximum distance in pixel for a small mutation
	string base = params.FindOneString("basesampler", "random");	// sampler for new chain seed
	Sampler *sampler = MakeSampler(base, params, film);
	if (sampler == NULL) {
		luxError(LUX_SYSTEM, LUX_SEVERE, "ERPTSampler: Could not obtain a valid sampler");
		return NULL;
	}
	return new ERPTSampler(max(totMutations, 0), range, sampler);
}

static DynamicLoader::RegisterSampler<ERPTSampler> r("erpt");
