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

// TODO: add scaling of output image samples

// metrosampler.cpp*
#include "metrosampler.h"
#include "memory.h"
#include "scene.h"
#include "dynload.h"

using namespace lux;

#define SAMPLE_FLOATS 6
static const u_int rngN = 8191;
static const u_int rngA = 884;

MetropolisSampler::MetropolisData::MetropolisData(const Sample &sample) :
	consecRejects(0), large(true), stamp(0), weight(0.f), LY(0.f),
	alpha(0.f), totalLY(0.f), sampleCount(0.f)
{
	u_int i;
	// Compute number of non lazy samples
	normalSamples = SAMPLE_FLOATS;
	for (i = 0; i < sample.n1D.size(); ++i)
		normalSamples += sample.n1D[i];
	for (i = 0; i < sample.n2D.size(); ++i)
		normalSamples += 2 * sample.n2D[i];

	// Compute number of lazy samples and initialize management data
	totalSamples = normalSamples;
	offset = new u_int[sample.nxD.size()];
	totalTimes = 0;
	for (i = 0; i < sample.nxD.size(); ++i) {
		offset[i] = totalSamples;
		totalTimes += sample.nxD[i];
		totalSamples += sample.dxD[i] * sample.nxD[i];
	}

	// Allocate sample image to hold the current reference
	sampleImage = AllocAligned<float>(totalSamples);
	timeImage = AllocAligned<int>(totalTimes);

	// Compute best offset between sample vectors in the rng
	// TODO use the smallest gcf of totalSamples and rngN that is greater
	// than totalSamples or equal
	rngOffset = totalSamples;
	if (rngOffset >= rngN)
		rngOffset = rngOffset % (rngN - 1) + 1;
	// Current base index, minus offset to compensate for the advance done
	// in GetNextSample
	rngBase = rngN - rngOffset;
	// Allocate memory for the Cranley-Paterson rotation vector
	rngRotation = AllocAligned<float>(totalSamples);
}
MetropolisSampler::MetropolisData::~MetropolisData()
{
	FreeAligned(rngRotation);
	FreeAligned(timeImage);
	FreeAligned(sampleImage);
	delete[] offset;
}

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

static float rngDummy;
#define rngGet(__pos) (modff(rngSamples[(data->rngBase + (__pos)) % rngN] + data->rngRotation[(__pos)], &rngDummy))
#define rngGet2(__pos,__off) (modff(rngSamples[(data->rngBase + (__pos) + (__off)) % rngN] + data->rngRotation[(__pos)], &rngDummy))
// Metropolis method definitions
MetropolisSampler::MetropolisSampler(int xStart, int xEnd, int yStart, int yEnd,
		u_int maxRej, float largeProb, float rng, bool useV) :
 Sampler(xStart, xEnd, yStart, yEnd, 1),
 maxRejects(maxRej), pLarge(largeProb),
 range(rng), useVariance(useV)
{
	// Allocate and compute all values of the rng
	rngSamples = AllocAligned<float>(rngN);
	rngSamples[0] = 0.f;
	rngSamples[1] = 1.f / rngN;
	u_int rngI = 1;
	for (u_int i = 2; i < rngN; ++i) {
		rngI = (rngI * rngA) % rngN;
		rngSamples[i] = static_cast<float>(rngI) / rngN;
	}
	RandomGenerator rndg(1);
	Shuffle(rndg, rngSamples, rngN, 1);
}

MetropolisSampler::~MetropolisSampler() {
	FreeAligned(rngSamples);
}

// interface for new ray/samples from scene
bool MetropolisSampler::GetNextSample(Sample *sample)
{
	MetropolisData *data = (MetropolisData *)(sample->samplerData);

	// Advance to the next vector in the QMC sequence and stay inside the
	// array bounds
	data->rngBase += data->rngOffset;
	if (data->rngBase > rngN - 1)
		data->rngBase -= rngN;
	// If all possible combinations have been used,
	// change the Cranley-Paterson rotation vector
	// This is also responsible for first time initialization of the vector
	if (data->rngBase == 0) {
		// This is a safe point to stop without too visible patterns
		// if the render has to stop
		if (film->enoughSamplesPerPixel)
			return false;
		for (u_int i = 0; i < data->totalSamples; ++i)
			data->rngRotation[i] = sample->rng->floatValue();
	}
	if (data->large) {
		// *** large mutation ***
		// Initialize all non lazy samples
		sample->imageX = rngGet(0) * (xPixelEnd - xPixelStart) + xPixelStart;
		sample->imageY = rngGet(1) * (yPixelEnd - yPixelStart) + yPixelStart;
		sample->lensU = rngGet(2);
		sample->lensV = rngGet(3);
		sample->time = rngGet(4);
		sample->wavelengths = rngGet(5);
		for (u_int i = SAMPLE_FLOATS; i < data->normalSamples; ++i)
			sample->oneD[0][i - SAMPLE_FLOATS] = rngGet(i);
		// Reset number of mutations for lazy samples
		for (u_int i = 0; i < data->totalTimes; ++i)
			sample->timexD[0][i] = -1;
		// Reset number of mutations for whole sample
		sample->stamp = 0;
	} else {
		// *** small mutation ***
		// Mutation of non lazy samples
		sample->imageX = mutateScaled(data->sampleImage[0],
			rngGet(0), xPixelStart, xPixelEnd, range);
		sample->imageY = mutateScaled(data->sampleImage[1],
			rngGet(1), yPixelStart, yPixelEnd, range);
		sample->lensU = mutateScaled(data->sampleImage[2], rngGet(2), 0.f, 1.f, .5f);
		sample->lensV = mutateScaled(data->sampleImage[3], rngGet(3), 0.f, 1.f, .5f);
		sample->time = mutateScaled(data->sampleImage[4], rngGet(4), 0.f, 1.f, .5f);
		sample->wavelengths = mutateScaled(data->sampleImage[5],
			rngGet(5), 0.f, 1.f, .5f);
		for (u_int i = SAMPLE_FLOATS; i < data->normalSamples; ++i)
			sample->oneD[0][i - SAMPLE_FLOATS] =
				mutate(data->sampleImage[i], rngGet(i));
		// Increase reference mutation count
		++(sample->stamp);
	}
	return true;
}

float *MetropolisSampler::GetLazyValues(const Sample &sample, u_int num, u_int pos)
{
	MetropolisData *data = (MetropolisData *)(sample.samplerData);
	// Get size and position of current lazy values node
	const u_int size = sample.dxD[num];
	float *sd = sample.xD[num] + pos * size;
	// Get the reference number of mutations
	const int stampLimit = sample.stamp;
	// If we are at the target, don't do anything
	if (sample.timexD[num][pos] != stampLimit) {
		// If the node has not yet been initialized, do it now
		// otherwise get the last known value from the sample image
		if (sample.timexD[num][pos] == -1) {
			for (u_int i = 0; i < size; ++i)
				sd[i] = rngGet(data->normalSamples + pos * size + i);
			sample.timexD[num][pos] = 0;
		} else {
			float *image = data->sampleImage + data->offset[num] + pos * size;
			for (u_int i = 0; i < size; ++i)
				sd[i] = image[i];
		}
		// Mutate as needed
		for (; sample.timexD[num][pos] < stampLimit; ++(sample.timexD[num][pos])) {
			for (u_int i = 0; i < size; ++i)
				sd[i] = mutate(sd[i], rngGet2(data->normalSamples + pos * size + i, data->rngOffset * static_cast<u_int>(stampLimit - sample.timexD[num][pos] + 1)));
		}
	}
	return sd;
}

void MetropolisSampler::AddSample(const Sample &sample)
{
	MetropolisData *data = (MetropolisData *)(sample.samplerData);
	vector<Contribution> &newContributions(sample.contributions);
	float newLY = 0.f;
	for(u_int i = 0; i < newContributions.size(); ++i) {
		const float ly = newContributions[i].color.Y();
		if (ly > 0.f && !isinf(ly)) {
			if (useVariance && newContributions[i].variance > 0.f)
				newLY += ly * newContributions[i].variance;
			else
				newLY += ly;
		} else
			newContributions[i].color = XYZColor(0.f);
	}
	// calculate meanIntensity
	if (data->large) {
		data->totalLY += newLY;
		++(data->sampleCount);
	}
	const float meanIntensity = data->totalLY > 0. ? static_cast<float>(data->totalLY / data->sampleCount) : 1.f;

	sample.contribBuffer->AddSampleCount(1.f);

	// calculate accept probability from old and new image sample
	float accProb;
	if (data->LY > 0.f && data->consecRejects < maxRejects)
		accProb = min(1.f, newLY / data->LY);
	else
		accProb = 1.f;
	const float newWeight = accProb + (data->large ? 1.f : 0.f);
	data->weight += 1.f - accProb;
	// try or force accepting of the new sample
	if (accProb == 1.f || sample.rng->floatValue() < accProb) {
		// Add accumulated contribution of previous reference sample
		const float norm = data->weight / (data->LY / meanIntensity + pLarge);
		if (norm > 0.f) {
			for(u_int i = 0; i < data->oldContributions.size(); ++i)
				sample.contribBuffer->Add(data->oldContributions[i], norm);
		}
		// Save new contributions for reference
		data->weight = newWeight;
		data->LY = newLY;
		data->oldContributions = newContributions;
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

		data->consecRejects = 0;
	} else {
		// Add contribution of new sample before rejecting it
		const float norm = newWeight / (newLY / meanIntensity + pLarge);
		if (norm > 0.f) {
			for(u_int i = 0; i < newContributions.size(); ++i)
				sample.contribBuffer->Add(newContributions[i], norm);
		}
		// Restart from previous reference
		for (u_int i = 0; i < data->totalTimes; ++i)
			sample.timexD[0][i] = data->timeImage[i];
		sample.stamp = data->stamp;

		++(data->consecRejects);
	}
	newContributions.clear();
	const float mutationSelector = sample.rng->floatValue();
	data->large = (mutationSelector < pLarge);
}

Sampler* MetropolisSampler::CreateSampler(const ParamSet &params, const Film *film)
{
	int xStart, xEnd, yStart, yEnd;
	film->GetSampleExtent(&xStart, &xEnd, &yStart, &yEnd);
	int maxConsecRejects = params.FindOneInt("maxconsecrejects", 512);	// number of consecutive rejects before a next mutation is forced
	float largeMutationProb = params.FindOneFloat("largemutationprob", 0.4f);	// probability of generating a large sample mutation
	float range = params.FindOneFloat("mutationrange", (xEnd - xStart + yEnd - yStart) / 32.f);	// maximum distance in pixel for a small mutation
	bool useVariance = params.FindOneBool("usevariance", false);

	return new MetropolisSampler(xStart, xEnd, yStart, yEnd, max(maxConsecRejects, 0),
		largeMutationProb, range, useVariance);
}

static DynamicLoader::RegisterSampler<MetropolisSampler> r("metropolis");
