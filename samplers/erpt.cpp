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

// Metropolis method definitions
ERPTSampler::ERPTSampler(u_int totMutations, float rng,
	Sampler *sampler) :
 Sampler(sampler->xPixelStart, sampler->xPixelEnd,
	sampler->yPixelStart, sampler->yPixelEnd, sampler->samplesPerPixel),
 normalSamples(0), totalSamples(0), totalTimes(0), totalMutations(totMutations),
 range(rng), baseSampler(sampler),
 baseImage(NULL), sampleImage(NULL), currentImage(NULL),
 baseTimeImage(NULL), timeImage(NULL), currentTimeImage(NULL), offset(NULL),
 numChains(0), chain(0), mutation(~0U), stamp(0),
 baseLY(0.f), quantum(0.f), weight(0.f), LY(0.f), alpha(0.f),
 totalLY(0.), sampleCount(0.)
{
}

ERPTSampler::~ERPTSampler() {
	FreeAligned(sampleImage);
	FreeAligned(baseImage);
	FreeAligned(timeImage);
	FreeAligned(baseTimeImage);
	delete baseSampler;
}

// Copy
ERPTSampler* ERPTSampler::clone() const
{
	ERPTSampler *newSampler = new ERPTSampler(*this);
	newSampler->baseSampler = baseSampler->clone();
	newSampler->totalSamples = 0;
	newSampler->sampleImage = NULL;
	return newSampler;
}

static void initERPT(ERPTSampler *sampler, const Sample *sample)
{
	u_int i;
	sampler->normalSamples = SAMPLE_FLOATS;
	for (i = 0; i < sample->n1D.size(); ++i)
		sampler->normalSamples += sample->n1D[i];
	for (i = 0; i < sample->n2D.size(); ++i)
		sampler->normalSamples += 2 * sample->n2D[i];
	sampler->totalSamples = sampler->normalSamples;
	sampler->offset = new u_int[sample->nxD.size()];
	sampler->totalTimes = 0;
	for (i = 0; i < sample->nxD.size(); ++i) {
		sampler->offset[i] = sampler->totalSamples;
		sampler->totalTimes += sample->nxD[i];
		sampler->totalSamples += sample->dxD[i] * sample->nxD[i];
	}
	sampler->sampleImage = AllocAligned<float>(sampler->totalSamples);
	sampler->baseImage = AllocAligned<float>(sampler->totalSamples);
	sampler->timeImage = AllocAligned<int>(sampler->totalTimes);
	sampler->baseTimeImage = AllocAligned<int>(sampler->totalTimes);
	sampler->baseSampler->SetTsPack(sampler->tspack);
	sampler->baseSampler->SetFilm(sampler->film);
	sampler->mutation = ~0U;

	// Fetch first contribution buffer from pool
	sampler->contribBuffer = sampler->film->scene->contribPool->Next(NULL);
}

// interface for new ray/samples from scene
bool ERPTSampler::GetNextSample(Sample *sample, u_int *use_pos)
{
	sample->sampler = this;
	if (sampleImage == NULL) {
		initERPT(this, sample);
	}

	if (mutation == ~0U) {
		// Dade - we are at a valid checkpoint where we can stop the
		// rendering. Check if we have enough samples per pixel in the film.
		if (film->enoughSamplePerPixel)
			return false;

		const bool ret = baseSampler->GetNextSample(sample, use_pos);
		sample->sampler = this;
		for (u_int i = 0; i < totalTimes; ++i)
			sample->timexD[0][i] = -1;
		sample->stamp = 0;
		currentImage = baseImage;
		currentTimeImage = baseTimeImage;
		stamp = 0;
		return ret;
	} else {
		if (mutation == 0) {
			// *** new chain ***
			for (u_int i = 0; i < totalTimes; ++i)
				sample->timexD[0][i] = baseTimeImage[i];
			sample->stamp = 0;
			currentImage = baseImage;
			currentTimeImage = baseTimeImage;
			stamp = 0;
		}
		// *** small mutation ***
		// mutate current sample
		sample->imageX = mutateScaled(currentImage[0], tspack->rng->floatValue(), xPixelStart, xPixelEnd, range);
		sample->imageY = mutateScaled(currentImage[1], tspack->rng->floatValue(), yPixelStart, yPixelEnd, range);
		sample->lensU = mutate(currentImage[2], tspack->rng->floatValue());
		sample->lensV = mutate(currentImage[3], tspack->rng->floatValue());
		sample->time = mutate(currentImage[4], tspack->rng->floatValue());
		sample->wavelengths = mutate(currentImage[5], tspack->rng->floatValue());
		for (u_int i = SAMPLE_FLOATS; i < normalSamples; ++i)
				sample->oneD[0][i - SAMPLE_FLOATS] = mutate(currentImage[i], tspack->rng->floatValue());
		++(sample->stamp);
	}

	return true;
}

float *ERPTSampler::GetLazyValues(Sample *sample, u_int num, u_int pos)
{
	const u_int size = sample->dxD[num];
	float *data = sample->xD[num] + pos * size;
	const int stampLimit = sample->stamp;
	if (sample->timexD[num][pos] != stampLimit) {
		if (sample->timexD[num][pos] == -1) {
			baseSampler->GetLazyValues(sample, num, pos);
			sample->timexD[num][pos] = 0;
		} else {
			const u_int start = offset[num] + pos * size;
			float *image = currentImage + start;
			for (u_int i = 0; i < size; ++i)
				data[i] = image[i];
		}
		for (int &time = sample->timexD[num][pos]; time < stampLimit; ++time) {
			for (u_int i = 0; i < size; ++i)
				data[i] = mutate(data[i], tspack->rng->floatValue());
		}
	}
	return data;
}

// interface for adding/accepting a new image sample.
void ERPTSampler::AddSample(const Sample &sample)
{
	vector<Contribution> &newContributions(sample.contributions);
	float newLY = 0.0f;
	for(u_int i = 0; i < newContributions.size(); ++i)
		newLY += newContributions[i].color.Y();
	if (mutation == 0U || mutation == ~0U) {
		if (weight > 0.f) {
			// Add accumulated contribution of previous reference sample
			weight *= quantum / LY;
			if (!isinf(weight) && LY > 0.f) {
				for(u_int i = 0; i < oldContributions.size(); ++i) {
					// Radiance - added new use of contributionpool/buffers
					if(&oldContributions && !contribBuffer->Add(&oldContributions[i], weight)) {
						contribBuffer = film->scene->contribPool->Next(contribBuffer);
						contribBuffer->Add(&oldContributions[i], weight);
					}
				}
			}
			weight = 0.f;
		}
		if (mutation == ~0U) {
			contribBuffer->AddSampleCount(1.f);
			++sampleCount;
			if (!(newLY > 0.f)) {
				newContributions.clear();
				return;
			}
			totalLY += newLY;
			const float meanIntensity = totalLY > 0. ? static_cast<float>(totalLY / sampleCount) : 1.f;
			// calculate the number of chains on a new seed
			quantum = newLY / meanIntensity;
			numChains = max(1U, Floor2UInt(quantum + .5f));
			if (numChains > 100) {
				printf("%d chains -> %d\n", numChains, totalMutations);
				numChains = totalMutations;
			}
			quantum /= (numChains * totalSamples);
			baseLY = newLY;
			baseContributions = newContributions;
			baseImage[0] = sample.imageX;
			baseImage[1] = sample.imageY;
			baseImage[2] = sample.lensU;
			baseImage[3] = sample.lensV;
			baseImage[4] = sample.time;
			baseImage[5] = sample.wavelengths;
			for (u_int i = SAMPLE_FLOATS; i < totalSamples; ++i)
				baseImage[i] = sample.oneD[0][i - SAMPLE_FLOATS];
			for (u_int i = 0 ; i < totalTimes; ++i)
				baseTimeImage[i] = sample.timexD[0][i];
			mutation = 0;
			newContributions.clear();
			return;
		}
		LY = baseLY;
		oldContributions = baseContributions;
	}
	// calculate accept probability from old and new image sample
	float accProb;
	if (LY > 0.f)
		accProb = min(1.f, newLY / LY);
	else
		accProb = 1.f;
	float newWeight = accProb;
	weight += 1.f - accProb;

	// try accepting of the new sample
	if (accProb == 1.f || tspack->rng->floatValue() < accProb) {
		// Add accumulated contribution of previous reference sample
		weight *= quantum / LY;
		if (!isinf(weight) && LY > 0.f) {
			for(u_int i = 0; i < oldContributions.size(); ++i) {
				// Radiance - added new use of contributionpool/buffers
				if(&oldContributions && !contribBuffer->Add(&oldContributions[i], weight)) {
					contribBuffer = film->scene->contribPool->Next(contribBuffer);
					contribBuffer->Add(&oldContributions[i], weight);
				}
			}
		}
		weight = newWeight;
		LY = newLY;
		oldContributions = newContributions;

		// Save new contributions for reference
		sampleImage[0] = sample.imageX;
		sampleImage[1] = sample.imageY;
		sampleImage[2] = sample.lensU;
		sampleImage[3] = sample.lensV;
		sampleImage[4] = sample.time;
		sampleImage[5] = sample.wavelengths;
		for (u_int i = SAMPLE_FLOATS; i < totalSamples; ++i)
			sampleImage[i] = sample.oneD[0][i - SAMPLE_FLOATS];
		for (u_int i = 0 ; i < totalTimes; ++i)
			timeImage[i] = sample.timexD[0][i];
		stamp = sample.stamp;
		currentImage = sampleImage;
		currentTimeImage = timeImage;
	} else {
		// Add contribution of new sample before rejecting it
		newWeight *= quantum / newLY;
		if (!isinf(newWeight) && newLY > 0.f) {
			for(u_int i = 0; i < newContributions.size(); ++i) {
				// Radiance - added new use of contributionpool/buffers
				if(!contribBuffer->Add(&newContributions[i], newWeight)) {
					contribBuffer = film->scene->contribPool->Next(contribBuffer);
					contribBuffer->Add(&newContributions[i], newWeight);
				}
			}
		}

		// Restart from previous reference
		for (u_int i = 0; i < totalTimes; ++i)
			sample.timexD[0][i] = currentTimeImage[i];
		sample.stamp = stamp;
	}
	if (++mutation >= totalMutations) {
		mutation = 0;
		if (++chain >= numChains) {
			chain = 0;
			mutation = ~0U;
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
