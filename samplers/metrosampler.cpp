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



// initial metropolis light transport sample integrator
// by radiance

// TODO: add scaling of output image samples

// metrosampler.cpp*
#include "metrosampler.h"
#include "memory.h"
#include "dynload.h"

using namespace lux;

#define SAMPLE_FLOATS 7

// quasi random number generator
static float qrNumber(const TsPack *tspack, u_int generation, u_int scramble, u_int order)
{
	switch (order) {
	case 0:
//		return VanDerCorput(generation, scramble);
		return Halton(generation, scramble);
	case 1:
//		return Sobol2(generation, scramble);
		return Halton2(generation, scramble);
	default:
		return tspack->rng->floatValue();
	}
}

// mutate a value in the range [0-1]
static float mutate(const float x, const float randomValue)
{
	static const float s1 = 1/1024., s2 = 1/16.;
	float dx = s2 * exp(-log(s2/s1) * fabsf(2.f * randomValue - 1.f));
	if (randomValue < 0.5) {
		float x1 = x + dx;
		return (x1 > 1) ? x1 - 1 : x1;
	} else {
		float x1 = x - dx;
		return (x1 < 0) ? x1 + 1 : x1;
	}
}

// mutate a value in the range [min-max]
static float mutateScaled(const float x, const float randomValue, const float mini, const float maxi, const float range)
{
	static const float s1 = 64.;
	float dx = range * exp(-log(s1) * fabsf(2.f * randomValue - 1.f));
	if (randomValue < 0.5) {
		float x1 = x + dx;
		return (x1 > maxi) ? x1 - maxi + mini : x1;
	} else {
		float x1 = x - dx;
		return (x1 < mini) ? x1 - mini + maxi : x1;
	}
}

// Metropolis method definitions
MetropolisSampler::MetropolisSampler(int xStart, int xEnd, int yStart, int yEnd,
		int maxRej, float largeProb, float microProb, float rng, int sw,
		bool useV, bool useQR) :
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
	if (useQR)
		orderOffset = 0;
	else
		orderOffset = 10;
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
	sampler->scramble = (u_int *)AllocAligned(sampler->totalSamples * sizeof(u_int));
	for (int i = 0; i < sampler->totalSamples; ++i)
		sampler->scramble[i] = sampler->tspack->rng->uintValue();
}

// interface for new ray/samples from scene
bool MetropolisSampler::GetNextSample(Sample *sample, u_int *use_pos)
{
	sample->sampler = this;
	++generation;
	const float mutationSelector = tspack->rng->floatValue();
	large = (mutationSelector < pLarge) || initCount < initSamples;
	if (sampleImage == NULL) {
		initMetropolis(this, sample);
		generation = 0;
		large = true;
	}
	if (generation >= 1024) {
		// Change randomization to avoid artefacts
		for (int i = 0; i < totalSamples; ++i)
			scramble[i] = tspack->rng->uintValue();
		generation = 0;
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

		sample->lensU = qrNumber(tspack, generation, scramble[2], 0 + orderOffset);
		sample->lensV = qrNumber(tspack, generation, scramble[3], 1 + orderOffset);
		sample->time = qrNumber(tspack, generation, scramble[4], 0 + orderOffset);
		sample->wavelengths = qrNumber(tspack, generation, scramble[5], 0 + orderOffset);
		sample->singleWavelength = qrNumber(tspack, generation, scramble[6], 0 + orderOffset);
		for (int i = SAMPLE_FLOATS; i < normalSamples; ++i)
			sample->oneD[0][i - SAMPLE_FLOATS] = qrNumber(tspack, generation, scramble[i], (i & 1) + orderOffset);
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
			sample->imageX = mutateScaled(sampleImage[0], qrNumber(tspack, generation, scramble[0], 0 + orderOffset), xPixelStart, xPixelEnd, range);
			sample->imageY = mutateScaled(sampleImage[1], qrNumber(tspack, generation, scramble[1], 1 + orderOffset), yPixelStart, yPixelEnd, range);
			sample->lensU = mutate(sampleImage[2], qrNumber(tspack, generation, scramble[2], 0 + orderOffset));
			sample->lensV = mutate(sampleImage[3], qrNumber(tspack, generation, scramble[3], 1 + orderOffset));
			sample->time = mutate(sampleImage[4], qrNumber(tspack, generation, scramble[4], 0 + orderOffset));
			sample->wavelengths = mutate(sampleImage[5], qrNumber(tspack, generation, scramble[5], 0 + orderOffset));
			sample->singleWavelength = qrNumber(tspack, generation, scramble[6], 0 + orderOffset);//mutate(sampleImage[6], qrNumber(tspack, generation, scramble[6], 0 + orderOffset))
			for (int i = SAMPLE_FLOATS; i < normalSamples; ++i)
				sample->oneD[0][i - SAMPLE_FLOATS] = mutate(sampleImage[i], qrNumber(tspack, generation, scramble[i], (i & 1) + orderOffset));
		}
		++(sample->stamp);
	}

    return true;
}

float *MetropolisSampler::GetLazyValues(Sample *sample, u_int num, u_int pos)
{
	const u_int size = sample->dxD[num];
	float *data = sample->xD[num] + pos * size;
	const int scrambleOffset = data - sample->oneD[0];
	int stampLimit = sample->stamp;
	if (numMicro >= 0 && num != numMicro - 1 && pos != posMicro)
		--stampLimit;
	if (sample->timexD[num][pos] != stampLimit) {
		if (sample->timexD[num][pos] == -1) {
			for (u_int i = 0; i < size; ++i)
				data[i] = qrNumber(tspack, generation - sample->stamp, scramble[scrambleOffset + i], (i & 1) + orderOffset);
			sample->timexD[num][pos] = 0;
		} else {
			float *image = sampleImage + offset[num] + pos * size;
			for (u_int i = 0; i < size; ++i)
				data[i] = image[i];
		}
		for (; sample->timexD[num][pos] < stampLimit; ++(sample->timexD[num][pos])) {
			for (u_int i = 0; i < size; ++i)
				data[i] = mutate(data[i], qrNumber(tspack, generation - sample->stamp + sample->timexD[num][pos] + 1, scramble[scrambleOffset + i], (i & 1) + orderOffset));
		}
	}
	return data;
}

// interface for adding/accepting a new image sample.
void MetropolisSampler::AddSample(float imageX, float imageY, const Sample &sample, const Ray &ray, const XYZColor &newL, float newAlpha, int id)
{
	if (!isSampleEnd) {
		sample.AddContribution(imageX, imageY, newL, newAlpha, id);
	} else {	// backward compatible with multiimage.cpp and path.cpp
		sample.contributions.clear();
		sample.AddContribution(imageX, imageY, newL, newAlpha, id);
	}
}

void MetropolisSampler::AddSample(const Sample &sample)
{
	vector<Sample::Contribution> &newContributions(sample.contributions);
	float newLY = 0.f, newV = 0.f;
	for(u_int i = 0; i < newContributions.size(); ++i) {
		newLY += newContributions[i].color.y();
		if (newContributions[i].color.y() > 0.f)
			newV += newContributions[i].color.y() * newContributions[i].variance;
	}
	// calculate meanIntensity
	if (initCount < initSamples) {
		meanIntensity += newLY / initSamples;
		++(initCount);
		if (initCount < initSamples)
			return;
		if (meanIntensity <= 0.f) meanIntensity = 1.f;
	}
//	newV = min(newLY / meanIntensity, newV);
	film->AddSampleCount(1.f); // TODO: add to the correct buffer groups
	// calculate accept probability from old and new image sample
	float accProb, accProb2, factor;
	if (LY > 0.f) {
		accProb = min(1.f, newLY / LY);
		if (useVariance && V > 0.f && newV > 0.f && newLY > 0.f)
			factor = newV / (/*newLY **/ V * accProb);
		else
			factor = 1.f;
	} else {
		accProb = 1.f;
		factor = 1.f;
	}
	accProb2 = accProb * factor;
	float newWeight = (accProb + (large ? 1.f : 0.f)) / (factor * newLY / meanIntensity + pLarge);
	weight += (1.f - accProb) / (LY / (factor * meanIntensity) + pLarge);
	// try or force accepting of the new sample
	if (accProb2 == 1.f || consecRejects >= maxRejects || tspack->rng->floatValue() < accProb2) {
		// Add accumulated contribution of previous reference sample
		for(u_int i = 0; i < oldContributions.size(); ++i) {
			XYZColor color = oldContributions[i].color;
			color *= weight;
			film->AddSample(oldContributions[i].imageX, oldContributions[i].imageY,
				color, oldContributions[i].alpha,
				oldContributions[i].buffer, oldContributions[i].bufferGroup);
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
		for(u_int i = 0; i < newContributions.size(); ++i) {
			XYZColor color = newContributions[i].color;
			color *= newWeight;
			film->AddSample(newContributions[i].imageX, newContributions[i].imageY,
				color, newContributions[i].alpha,
				newContributions[i].buffer, newContributions[i].bufferGroup);
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
	float microMutationProb = params.FindOneFloat("micromutationprob", 0.f);	// probability of generating a large sample mutation
	float range = params.FindOneFloat("mutationrange", (xEnd - xStart + yEnd - yStart) / 32.);	// maximum distance in pixel for a small mutation
	int stratawidth = params.FindOneInt("stratawidth", 256);	// stratification of large mutation image samples (stratawidth*stratawidth)
	bool useVariance = params.FindOneBool("usevariance", false);
	bool useQR = params.FindOneBool("useqr", false);	// use quasi random number sequences

	return new MetropolisSampler(xStart, xEnd, yStart, yEnd, maxConsecRejects,
			largeMutationProb, microMutationProb, range, stratawidth, useVariance, useQR);
}

int MetropolisSampler::initCount, MetropolisSampler::initSamples;
float MetropolisSampler::meanIntensity;

static DynamicLoader::RegisterSampler<MetropolisSampler> r("metropolis");
