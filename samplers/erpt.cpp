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
	static const float s1 = 1.f / 512.f, s2 = 1.f / 16.f;
	const float dx = s1 / (s1 / s2 + fabsf(2.f * randomValue - 1.f)) -
		s1 / (s1 / s2 + 1.f);
	if (randomValue < 0.5f) {
		float x1 = x + dx;
		return (x1 < 1.f) ? x1 : x1 - 1.f;
	} else {
		float x1 = x - dx;
		return (x1 < 0.f) ? x1 + 1.f : x1;
	}
}

// mutate a value in the range [min-max]
static float mutateScaled(const float x, const float randomValue, const float mini, const float maxi, const float range)
{
	static const float s1 = 32.f;
	const float dx = range / (s1 / (1.f + s1) + (s1 * s1) / (1.f + s1) *
		fabsf(2.f * randomValue - 1.f)) - range / s1;
	if (randomValue < 0.5f) {
		float x1 = x + dx;
		return (x1 < maxi) ? x1 : x1 - maxi + mini;
	} else {
		float x1 = x - dx;
		return (x1 < mini) ? x1 - mini + maxi : x1;
	}
}

ERPTSampler::ERPTData::ERPTData(const Sampler &sampler) :
	numChains(0), chain(0), mutation(~0U), stamp(0), currentStamp(0),
	baseLY(0.f), quantum(0.f), weight(0.f), LY(0.f), alpha(0.f),
	totalLY(0.), sampleCount(0.)
{
	u_int i;
	normalSamples = SAMPLE_FLOATS;
	for (i = 0; i < sampler.n1D.size(); ++i)
		normalSamples += sampler.n1D[i];
	for (i = 0; i < sampler.n2D.size(); ++i)
		normalSamples += 2 * sampler.n2D[i];
	totalSamples = normalSamples;
	timeOffset = new u_int[sampler.nxD.size()];
	offset = new u_int[sampler.nxD.size()];
	totalTimes = 0;
	for (i = 0; i < sampler.nxD.size(); ++i) {
		timeOffset[i] = totalTimes;
		offset[i] = totalSamples;
		totalTimes += sampler.nxD[i];
		totalSamples += sampler.dxD[i] * sampler.nxD[i];
	}
	sampleImage = AllocAligned<float>(totalSamples);
	currentImage = AllocAligned<float>(totalSamples);
	baseImage = AllocAligned<float>(totalSamples);
	timeImage = AllocAligned<int>(totalTimes);
	currentTimeImage = AllocAligned<int>(totalTimes);
	baseTimeImage = AllocAligned<int>(totalTimes);
}
ERPTSampler::ERPTData::~ERPTData()
{
	FreeAligned(baseTimeImage);
	FreeAligned(currentTimeImage);
	FreeAligned(timeImage);
	FreeAligned(baseImage);
	FreeAligned(currentImage);
	FreeAligned(sampleImage);
	delete[] offset;
	delete[] timeOffset;
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
bool ERPTSampler::GetNextSample(Sample *sample)
{
	const RandomGenerator *rng = sample->rng;
	ERPTData *data = (ERPTData *)(sample->samplerData);

	if (data->mutation == ~0U) {
		// Dade - we are at a valid checkpoint where we can stop the
		// rendering. Check if we have enough samples per pixel in the film.
		if (film->enoughSamplesPerPixel)
			return false;

		sample->samplerData = data->baseSamplerData;
		const bool ret = baseSampler->GetNextSample(sample);
		float *image = data->baseImage;
		*image++ = sample->imageX;
		*image++ = sample->imageY;
		*image++ = sample->lensU;
		*image++ = sample->lensV;
		*image++ = sample->time;
		*image++ = sample->wavelengths;
		for (u_int i = 0; i < n1D.size(); ++i) {
			for (u_int j = 0; j < n1D[i]; ++j)
				*image++ = baseSampler->GetOneD(*sample, i, j);
		}
		for (u_int i = 0; i < n2D.size(); ++i) {
			for (u_int j = 0; j < n2D[i]; ++j) {
				float u[2];
				baseSampler->GetTwoD(*sample, i, j, u);
				*image++ = u[0];
				*image++ = u[1];
			}
		}
		sample->samplerData = data;
		for (u_int i = 0; i < data->totalTimes; ++i)
			data->baseTimeImage[i] = -1;
		data->currentStamp = 0;
		data->stamp = 0;
		return ret;
	} else {
		if (data->mutation == 0) {
			// *** new chain ***
			memcpy(data->currentImage, data->baseImage, data->totalSamples * sizeof(float));
			memcpy(data->currentTimeImage, data->baseTimeImage, data->totalTimes * sizeof(int));
			for (u_int i = 0; i < data->totalTimes; ++i)
				data->timeImage[i] = -1;
			data->currentStamp = 0;
			data->stamp = 0;
		}
		// *** small mutation ***
		// mutate current sample
		data->sampleImage[0] = mutateScaled(data->currentImage[0], rng->floatValue(), xPixelStart, xPixelEnd, range);
		data->sampleImage[1] = mutateScaled(data->currentImage[1], rng->floatValue(), yPixelStart, yPixelEnd, range);
		for (u_int i = 2; i < data->normalSamples; ++i)
			data->sampleImage[i] = mutate(data->currentImage[i], rng->floatValue());
		data->stamp = data->currentStamp + 1;
		sample->imageX = data->sampleImage[0];
		sample->imageY = data->sampleImage[1];
		sample->lensU = data->sampleImage[2];
		sample->lensV = data->sampleImage[3];
		sample->time = data->sampleImage[4];
		sample->wavelengths = data->sampleImage[5];
	}

	return true;
}

float ERPTSampler::GetOneD(const Sample &sample, u_int num, u_int pos)
{
	ERPTData *data = (ERPTData *)(sample.samplerData);
	u_int offset = SAMPLE_FLOATS;
	for (u_int i = 0; i < num; ++i)
		offset += n1D[i];
	if (data->mutation == ~0U)
		return data->baseImage[offset + pos];
	else
		return data->sampleImage[offset + pos];
}

void ERPTSampler::GetTwoD(const Sample &sample, u_int num, u_int pos, float u[2])
{
	ERPTData *data = (ERPTData *)(sample.samplerData);
	u_int offset = SAMPLE_FLOATS;
	for (u_int i = 0; i < n1D.size(); ++i)
		offset += n1D[i];
	for (u_int i = 0; i < num; ++i)
		offset += n2D[i] * 2;
	if (data->mutation == ~0U) {
		u[0] = data->baseImage[offset + pos * 2];
		u[1] = data->baseImage[offset + pos * 2 + 1];
	} else {
		u[0] = data->sampleImage[offset + pos * 2];
		u[1] = data->sampleImage[offset + pos * 2 + 1];
	}
}

float *ERPTSampler::GetLazyValues(const Sample &sample, u_int num, u_int pos)
{
	ERPTData *data = (ERPTData *)(sample.samplerData);
	const u_int size = dxD[num];
	const u_int first = data->offset[num] + pos * size;
	int &baseTime(data->baseTimeImage[data->timeOffset[num] + pos]);
	if (baseTime == -1) {
		const_cast<Sample&>(sample).samplerData = data->baseSamplerData;
		float *base = baseSampler->GetLazyValues(sample, num, pos);
		const_cast<Sample&>(sample).samplerData = data;
		for (u_int i = 0; i < size; ++i)
			data->baseImage[first + i] = base[i];
		baseTime = 0;
	}
	if (data->mutation == ~0U)
		return data->baseImage + first;
	int &time(data->timeImage[data->timeOffset[num] + pos]);
	const int stampLimit = data->stamp;
	if (time != stampLimit) {
		int &currentTime(data->currentTimeImage[data->timeOffset[num] + pos]);
		if (currentTime == -1) {
			for (u_int i = first; i < first + size; ++i)
				data->currentImage[i] = data->baseImage[i];
			currentTime = 0;
		}
		for (u_int i = first; i < first + size; ++i) {
			data->sampleImage[i] = data->currentImage[i];
			for (time = currentTime; time < stampLimit; ++time)
				data->sampleImage[i] = mutate(data->sampleImage[i], sample.rng->floatValue());
		}
	}
	return data->sampleImage + first;
}

// interface for adding/accepting a new image sample.
void ERPTSampler::AddSample(const Sample &sample)
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
				for(u_int i = 0; i < data->oldContributions.size(); ++i)
					sample.contribBuffer->Add(data->oldContributions[i], data->weight);
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
//			data->numChains = min(data->numChains, totalMutations);
			data->quantum /= data->numChains * totalMutations;
			data->baseLY = newLY;
			data->baseContributions = newContributions;
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
			for(u_int i = 0; i < data->oldContributions.size(); ++i)
				sample.contribBuffer->Add(data->oldContributions[i], data->weight);
		}
		data->weight = newWeight;
		data->LY = newLY;
		data->oldContributions = newContributions;

		// Save new contributions for reference
		swap(data->sampleImage, data->currentImage);
		swap(data->timeImage, data->currentTimeImage);
		data->currentStamp = data->stamp;
	} else {
		// Add contribution of new sample before rejecting it
		newWeight *= data->quantum / newLY;
		if (!isinf(newWeight) && newLY > 0.f) {
			for(u_int i = 0; i < newContributions.size(); ++i)
				sample.contribBuffer->Add(newContributions[i], newWeight);
		}

		data->stamp = data->currentStamp;
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

Sampler* ERPTSampler::CreateSampler(const ParamSet &params, Film *film)
{
	int xStart, xEnd, yStart, yEnd;
	film->GetSampleExtent(&xStart, &xEnd, &yStart, &yEnd);
	int totMutations = params.FindOneInt("chainlength", 100);	// number of mutations from a given seed
	float range = params.FindOneFloat("mutationrange", (xEnd - xStart + yEnd - yStart) / 50.f);	// maximum distance in pixel for a small mutation
	string base = params.FindOneString("basesampler", "random");	// sampler for new chain seed
	Sampler *sampler = MakeSampler(base, params, film);
	if (sampler == NULL) {
		LOG( LUX_SEVERE,LUX_SYSTEM)<< "ERPTSampler: Could not obtain a valid sampler";
		return NULL;
	}
	return new ERPTSampler(max(totMutations, 0), range, sampler);
}

static DynamicLoader::RegisterSampler<ERPTSampler> r("erpt");
