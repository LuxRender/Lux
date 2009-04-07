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

// TODO: add scaling of output image samples

// metrosampler.cpp*
#include "metrosampler.h"
#include "memory.h"
#include "scene.h"
#include "dynload.h"

using namespace lux;

#define SAMPLE_FLOATS 7

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
	const float dx = range / (1.f + s1 * fabsf(2.f * randomValue - 1.f)) -
		range / (1.f + s1);
	if (randomValue < 0.5f) {
		float x1 = x + dx;
		return (x1 < maxi) ? x1 : x1 - maxi + mini;
	} else {
		float x1 = x - dx;
		return (x1 < mini) ? x1 - mini + maxi : x1;
	}
}

// Metropolis method definitions
MetropolisSampler::MetropolisSampler(int xStart, int xEnd, int yStart, int yEnd,
		int maxRej, float largeProb, float microProb, float rng, int sw,
		bool useV) :
 Sampler(xStart, xEnd, yStart, yEnd, 1), large(true), LY(0.f), V(0.f),
 totalSamples(0), totalTimes(0), maxRejects(maxRej), consecRejects(0), stamp(0),
 numMicro(-1), posMicro(-1), pLarge(largeProb), pMicro(microProb), range(rng),
 weight(0.f), alpha(0.f), sampleImage(NULL), timeImage(NULL), strataWidth(sw),
 useVariance(useV)
{
	// Allocate storage for image stratified samples
	strataSqr = sw * sw;
	strataSamples = (float *)AllocAligned(2 * strataSqr * sizeof(float));
	currentStrata = strataSqr;
}

MetropolisSampler::~MetropolisSampler() {
	FreeAligned(sampleImage);
	FreeAligned(strataSamples);
}

// Copy
MetropolisSampler* MetropolisSampler::clone() const
{
	MetropolisSampler *newSampler = new MetropolisSampler(*this);
	newSampler->totalSamples = 0;
	newSampler->sampleImage = NULL;

	return newSampler;
}

static void initMetropolis(MetropolisSampler *sampler, const Sample *sample)
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
	sampler->sampleImage = (float *)AllocAligned(sampler->totalSamples * sizeof(float));
	sampler->timeImage = (int *)AllocAligned(sampler->totalTimes * sizeof(int));

	// Fetch first contribution buffer from pool
	sampler->contribBuffer = sampler->film->scene->contribPool->Next(NULL);
}

// interface for new ray/samples from scene
bool MetropolisSampler::GetNextSample(Sample *sample, u_int *use_pos)
{
	sample->sampler = this;
	const float mutationSelector = tspack->rng->floatValue();
	large = (mutationSelector < pLarge) || initCount < initSamples;
	if (sampleImage == NULL) {
		initMetropolis(this, sample);
		large = true;
	}

	// Dade - we are at a valid checkpoint where we can stop the
	// rendering. Check if we have enough samples per pixel in the film.
	if (film->enoughSamplePerPixel)
		return false;
	if (large) {
		if(currentStrata == strataSqr) {

			// Generate shuffled stratified image samples
			StratifiedSample2D(tspack, strataSamples, strataWidth, strataWidth, true);
			Shuffle(tspack, strataSamples, strataSqr, 2);
			currentStrata = 0;
		}

		// *** large mutation ***
		sample->imageX = strataSamples[currentStrata*2] * (xPixelEnd - xPixelStart) + xPixelStart;
		sample->imageY = strataSamples[(currentStrata*2)+1] * (yPixelEnd - yPixelStart) + yPixelStart;
		currentStrata++;

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
	} else {
		// *** small mutation ***
		// mutate current sample
		if (1.f - mutationSelector < pMicro)
			numMicro = min<int>(sample->nxD.size(), Float2Int((1.f - mutationSelector) / pMicro * (sample->nxD.size() + 1)));
		else
			numMicro = -1;
		if (numMicro > 0) {
			u_int maxPos = 0;
			for (; maxPos < sample->nxD[numMicro - 1]; ++maxPos)
				if (sample->timexD[numMicro - 1][maxPos] < sample->stamp)
					break;
			posMicro = min<int>(sample->nxD[numMicro - 1] - 1, Float2Int(tspack->rng->floatValue() * maxPos));
		} else {
			posMicro = -1;
			sample->imageX = mutateScaled(sampleImage[0], tspack->rng->floatValue(), xPixelStart, xPixelEnd, range);
			sample->imageY = mutateScaled(sampleImage[1], tspack->rng->floatValue(), yPixelStart, yPixelEnd, range);
			sample->lensU = mutate(sampleImage[2], tspack->rng->floatValue());
			sample->lensV = mutate(sampleImage[3], tspack->rng->floatValue());
			sample->time = mutate(sampleImage[4], tspack->rng->floatValue());
			sample->wavelengths = mutate(sampleImage[5], tspack->rng->floatValue());
			sample->singleWavelength = mutateScaled(sampleImage[6], tspack->rng->floatValue(), 0.f, 1.f, 1.f);
			for (int i = SAMPLE_FLOATS; i < normalSamples; ++i)
				sample->oneD[0][i - SAMPLE_FLOATS] = mutate(sampleImage[i], tspack->rng->floatValue());
		}
		++(sample->stamp);
	}

    return true;
}

float *MetropolisSampler::GetLazyValues(Sample *sample, u_int num, u_int pos)
{
	const u_int size = sample->dxD[num];
	float *data = sample->xD[num] + pos * size;
//	const int scrambleOffset = data - sample->oneD[0];
	int stampLimit = sample->stamp;
	if (numMicro >= 0 && num != numMicro - 1 && pos != posMicro)
		--stampLimit;
	if (sample->timexD[num][pos] != stampLimit) {
		if (sample->timexD[num][pos] == -1) {
			for (u_int i = 0; i < size; ++i)
				data[i] = tspack->rng->floatValue();
			sample->timexD[num][pos] = 0;
		} else {
			float *image = sampleImage + offset[num] + pos * size;
			for (u_int i = 0; i < size; ++i)
				data[i] = image[i];
		}
		for (; sample->timexD[num][pos] < stampLimit; ++(sample->timexD[num][pos])) {
			for (u_int i = 0; i < size; ++i)
				data[i] = mutate(data[i], tspack->rng->floatValue());
		}
	}
	return data;
}

void MetropolisSampler::AddSample(const Sample &sample)
{
	vector<Contribution> &newContributions(sample.contributions);
	float newLY = 0.f, newV = 0.f;
	for(u_int i = 0; i < newContributions.size(); ++i) {
		if (newContributions[i].color.y() > 0.f) {
			newLY += newContributions[i].color.y();
			newV += newContributions[i].color.y() * newContributions[i].variance;
		}
	}
	// calculate meanIntensity
	if (initCount < initSamples) {
		if (useVariance && newV > 0.f)
			meanIntensity += newV / initSamples;
		else if (newLY > 0.f)
			meanIntensity += newLY / initSamples;
		++(initCount);
		if (initCount < initSamples)
			return;
		if (!(meanIntensity > 0.f))
			meanIntensity = 1.f;
	}

	contribBuffer->AddSampleCount(1.f);

	// calculate accept probability from old and new image sample
	float accProb, accProb2, factor;
	if (LY > 0.f) {
		accProb = min(1.f, newLY / LY);
		if (useVariance && V > 0.f && newV > 0.f && newLY > 0.f)
			factor = Clamp(newV / (V * accProb), min(LY / newLY, newLY / LY), 1.f / accProb);
		else
			factor = 1.f;
	} else {
		accProb = 1.f;
		factor = 1.f;
	}
	accProb2 = accProb * factor;
	float newWeight = accProb + (large ? 1.f : 0.f);
	weight += 1.f - accProb;
	// try or force accepting of the new sample
	if (accProb2 == 1.f || consecRejects >= maxRejects || tspack->rng->floatValue() < accProb2) {
		// Add accumulated contribution of previous reference sample
		const float norm = 1.f / ((useVariance ? V : LY) / meanIntensity + pLarge);
		for(u_int i = 0; i < oldContributions.size(); ++i) {
			oldContributions[i].color *= norm;
			// Radiance - added new use of contributionpool/buffers
			if(&oldContributions && !contribBuffer->Add(&oldContributions[i], weight)) {
				contribBuffer = film->scene->contribPool->Next(contribBuffer);
				contribBuffer->Add(&oldContributions[i], weight);
			}
		}
		// Save new contributions for reference
		weight = newWeight;
		LY = newLY;
		V = newV;
		oldContributions = newContributions;
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

		consecRejects = 0;
	} else {
		// Add contribution of new sample before rejecting it
		const float norm = 1.f / ((useVariance ? newV : newLY) / meanIntensity + pLarge);
		for(u_int i = 0; i < newContributions.size(); ++i) {
			newContributions[i].color *= norm;
			// Radiance - added new use of contributionpool/buffers
			if(!contribBuffer->Add(&newContributions[i], newWeight)) {
				contribBuffer = film->scene->contribPool->Next(contribBuffer);
				contribBuffer->Add(&newContributions[i], newWeight);
			}
		}
		// Restart from previous reference
		for (int i = 0; i < totalTimes; ++i)
			sample.timexD[0][i] = timeImage[i];
		sample.stamp = stamp;

		consecRejects++;
	}
}

Sampler* MetropolisSampler::CreateSampler(const ParamSet &params, const Film *film)
{
	int xStart, xEnd, yStart, yEnd;
	film->GetSampleExtent(&xStart, &xEnd, &yStart, &yEnd);
	initSamples = params.FindOneInt("initsamples", 100000);
	initCount = 0;
	meanIntensity = 0.;
	int maxConsecRejects = params.FindOneInt("maxconsecrejects", 512);	// number of consecutive rejects before a next mutation is forced
	float largeMutationProb = params.FindOneFloat("largemutationprob", 0.4f);	// probability of generating a large sample mutation
	float microMutationProb = params.FindOneFloat("micromutationprob", 0.f);	// probability of generating a micro sample mutation
	float range = params.FindOneFloat("mutationrange", (xEnd - xStart + yEnd - yStart) / 32.);	// maximum distance in pixel for a small mutation
	int stratawidth = params.FindOneInt("stratawidth", 256);	// stratification of large mutation image samples (stratawidth*stratawidth)
	bool useVariance = params.FindOneBool("usevariance", false);

	return new MetropolisSampler(xStart, xEnd, yStart, yEnd, maxConsecRejects,
			largeMutationProb, microMutationProb, range, stratawidth, useVariance);
}

int MetropolisSampler::initCount, MetropolisSampler::initSamples;
float MetropolisSampler::meanIntensity;

static DynamicLoader::RegisterSampler<MetropolisSampler> r("metropolis");
