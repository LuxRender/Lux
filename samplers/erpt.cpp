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

#define SAMPLE_FLOATS 7

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
	float dx = range * exp(-log(2.f * range) * fabsf(2.f * randomValue - 1.f));
	if (randomValue < 0.5f) {
		float x1 = x + dx;
		return (x1 > maxi) ? x1 - maxi + mini : x1;
	} else {
		float x1 = x - dx;
		return (x1 < mini) ? x1 - mini + maxi : x1;
	}
}

// Metropolis method definitions
ERPTSampler::ERPTSampler(int totMutations, float microProb, float rng,
	Sampler *sampler) :
 Sampler(sampler->xPixelStart, sampler->xPixelEnd,
	sampler->yPixelStart, sampler->yPixelEnd, sampler->samplesPerPixel),
 LY(0.), baseLY(0.f), gain(0.f), quantum(0.f),
 totalSamples(0), totalTimes(0), totalMutations(totMutations), chain(0),
 numChains(0), mutation(-1), stamp(0), numMicro(-1), posMicro(-1),
 pMicro(microProb), range(rng), weight(0.), alpha(0.),
 baseImage(NULL), sampleImage(NULL), currentImage(NULL),
 timeImage(NULL), baseTimeImage(NULL), currentTimeImage(NULL),
 baseSampler(sampler)
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
	sampler->offset = new int[sample->nxD.size()];
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
	sampler->mutation = -1;

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

	if (initCount < initSamples) {
		sample->imageX = tspack->rng->floatValue() * (xPixelEnd - xPixelStart) + xPixelStart;
		sample->imageY = tspack->rng->floatValue() * (yPixelEnd - yPixelStart) + yPixelStart;
		sample->lensU = tspack->rng->floatValue();
		sample->lensV = tspack->rng->floatValue();
		sample->time = tspack->rng->floatValue();
		sample->wavelengths = tspack->rng->floatValue();
		sample->singleWavelength = tspack->rng->floatValue();
		for (int i = SAMPLE_FLOATS; i < normalSamples; ++i)
			sample->oneD[0][i - SAMPLE_FLOATS] = tspack->rng->floatValue();
		for (int i = 0; i < totalTimes; ++i)
			sample->timexD[0][i] = -1;
		sample->stamp = 0;
		numMicro = -1;
	} else if (mutation == -1) {
		// Dade - we are at a valid checkpoint where we can stop the
		// rendering. Check if we have enough samples per pixel in the film.
		if (film->enoughSamplePerPixel)
			return false;

		const bool ret = baseSampler->GetNextSample(sample, use_pos);
		sample->sampler = this;
		for (int i = 0; i < totalTimes; ++i)
			sample->timexD[0][i] = -1;
		sample->stamp = 0;
		currentImage = baseImage;
		currentTimeImage = baseTimeImage;
		stamp = 0;
		numMicro = -1;
		return ret;
	} else {
		if (mutation == 0) {
			// *** new chain ***
			for (int i = 0; i < totalTimes; ++i)
				sample->timexD[0][i] = baseTimeImage[i];
			sample->stamp = 0;
			currentImage = baseImage;
			currentTimeImage = baseTimeImage;
			stamp = 0;
		}
		// *** small mutation ***
		// mutate current sample
		float mutationSelector = tspack->rng->floatValue();
		if (mutationSelector < pMicro)
			numMicro = min<int>(sample->nxD.size(), Float2Int(mutationSelector / pMicro * (sample->nxD.size() + 1)));
		else
			numMicro = -1;
		if (numMicro > 0) {
			u_int maxPos = 0;
			for(; maxPos < sample->nxD[numMicro - 1]; ++maxPos)
				if (sample->timexD[numMicro - 1][maxPos] < sample->stamp)
					break;
			posMicro = min<int>(sample->nxD[numMicro - 1] - 1, Float2Int(tspack->rng->floatValue() * maxPos));
		} else {
			posMicro = -1;
			sample->imageX = mutateScaled(currentImage[0], tspack->rng->floatValue(), xPixelStart, xPixelEnd, range);
			sample->imageY = mutateScaled(currentImage[1], tspack->rng->floatValue(), yPixelStart, yPixelEnd, range);
			sample->lensU = mutate(currentImage[2], tspack->rng->floatValue());
			sample->lensV = mutate(currentImage[3], tspack->rng->floatValue());
			sample->time = mutate(currentImage[4], tspack->rng->floatValue());
			sample->wavelengths = mutate(currentImage[5], tspack->rng->floatValue());
			sample->singleWavelength = /*tspack->rng->floatValue();*/mutateScaled(currentImage[6], tspack->rng->floatValue(), 0.f, 1.f, 1.f);
			for (int i = SAMPLE_FLOATS; i < normalSamples; ++i)
				sample->oneD[0][i - SAMPLE_FLOATS] = mutate(currentImage[i], tspack->rng->floatValue());
		}
		++(sample->stamp);
	}

    return true;
}

float *ERPTSampler::GetLazyValues(Sample *sample, u_int num, u_int pos)
{
	const u_int size = sample->dxD[num];
	float *data = sample->xD[num] + pos * size;
	int stampLimit = sample->stamp;
	if (numMicro >= 0 && num != numMicro - 1 && pos != posMicro)
		--stampLimit;
	if (sample->timexD[num][pos] != stampLimit) {
		if (sample->timexD[num][pos] == -1) {
			if (initCount < initSamples) {
				for (u_int i = 0; i < size; ++i)
					data[i] = tspack->rng->floatValue();
				return data;
			} else {
				baseSampler->GetLazyValues(sample, num, pos);
				sample->timexD[num][pos] = 0;
			}
		} else {
			int start = offset[num] + pos * size;
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
		newLY += newContributions[i].color.y();
	// calculate meanIntensity
	if (initCount < initSamples) {
		if (newLY > 0.f)
			meanIntensity += newLY;
		++(initCount);
		if (initCount < initSamples)
			return;
		meanIntensity /= initSamples;
		if (!(meanIntensity > 0.f))
			meanIntensity = 1.f;
		mutation = -1;
		return;
	}
	if (mutation <= 0) {
		if (weight > 0.f) {
			// Add accumulated contribution of previous reference sample
			weight *= gain * quantum / LY;
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
		if (mutation == -1) {
			meanIntensity = Lerp(1.f / initSamples, meanIntensity, newLY);
			// calculate the number of chains on a new seed
			contribBuffer->AddSampleCount(1.f);
			if (!(newLY > 0.f))
				return;
			quantum = meanIntensity;
			gain = newLY / quantum;
			numChains = max(1, Floor2Int(gain + .5f));
			if (numChains > 100) {
				printf("%d chains -> %d\n", numChains, totalMutations);
				numChains = totalMutations;
			}
			gain /= numChains;
			quantum /= totalSamples;
			baseLY = newLY;
			baseContributions = newContributions;
			baseImage[0] = sample.imageX;
			baseImage[1] = sample.imageY;
			baseImage[2] = sample.lensU;
			baseImage[3] = sample.lensV;
			baseImage[4] = sample.time;
			baseImage[5] = sample.wavelengths;
			baseImage[6] = sample.singleWavelength;
			for (int i = SAMPLE_FLOATS; i < totalSamples; ++i)
				baseImage[i] = sample.oneD[0][i - SAMPLE_FLOATS];
			for (int i = 0 ; i < totalTimes; ++i)
				baseTimeImage[i] = sample.timexD[0][i];
			mutation = 0;
			return;
		}
		LY = baseLY;
		oldContributions = baseContributions;
	}
	// calculate accept probability from old and new image sample
	float accProb = min(1.0f, newLY / LY);
	float newWeight = accProb;
	weight += 1.f - accProb;

	const bool accept = accProb == 1.f || tspack->rng->floatValue() < accProb;

	// try accepting of the new sample
	if (accept) {
		// Add accumulated contribution of previous reference sample
		weight *= gain * quantum / LY;
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
		sampleImage[6] = sample.singleWavelength;
		for (int i = SAMPLE_FLOATS; i < totalSamples; ++i)
			sampleImage[i] = sample.oneD[0][i - SAMPLE_FLOATS];
		for (int i = 0 ; i < totalTimes; ++i)
			timeImage[i] = sample.timexD[0][i];
		stamp = sample.stamp;
		currentImage = sampleImage;
		currentTimeImage = timeImage;
	} else {
		// Add contribution of new sample before rejecting it
		newWeight *= gain * quantum / newLY;
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
		for (int i = 0; i < totalTimes; ++i)
			sample.timexD[0][i] = currentTimeImage[i];
		sample.stamp = stamp;
	}
	if (++mutation >= totalMutations) {
		mutation = 0;
		if (++chain >= numChains) {
			chain = 0;
			mutation = -1;
		}
	}
}

Sampler* ERPTSampler::CreateSampler(const ParamSet &params, const Film *film)
{
	int xStart, xEnd, yStart, yEnd;
	film->GetSampleExtent(&xStart, &xEnd, &yStart, &yEnd);
	initSamples = params.FindOneInt("initsamples", 100000);
	initCount = 0;
	meanIntensity = 0.;
	int totMutations = params.FindOneInt("chainlength", 100);	// number of mutations from a given seed
	float microMutationProb = params.FindOneFloat("micromutationprob", .5f);	//probability of generating a micro sample mutation
	float range = params.FindOneFloat("mutationrange", (xEnd - xStart + yEnd - yStart) / 50.);	// maximum distance in pixel for a small mutation
	string base = params.FindOneString("basesampler", "random");	// sampler for new chain seed
	Sampler *sampler = MakeSampler(base, params, film);
	if (sampler == NULL) {
		luxError(LUX_SYSTEM, LUX_SEVERE, "ERPTSampler: Could not obtain a valid sampler");
		return NULL;
	}
	return new ERPTSampler(totMutations, microMutationProb, range, sampler);
}

int ERPTSampler::initCount, ERPTSampler::initSamples;
float ERPTSampler::meanIntensity = 0.f;

static DynamicLoader::RegisterSampler<ERPTSampler> r("erpt");
