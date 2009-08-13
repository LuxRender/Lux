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

static const u_int rngN = 8191;
static const u_int rngA = 884;
static float rngDummy;
#define rngGet(__pos) (modff(rngSamples[(rngBase + (__pos)) % rngN] + rngRotation[(__pos)], &rngDummy))
#define rngGet2(__pos,__off) (modff(rngSamples[(rngBase + (__pos) + (__off)) % rngN] + rngRotation[(__pos)], &rngDummy))
// Metropolis method definitions
MetropolisSampler::MetropolisSampler(int xStart, int xEnd, int yStart, int yEnd,
		int maxRej, float largeProb, float microProb, float rng,
		bool useV) :
 Sampler(xStart, xEnd, yStart, yEnd, 1), normalSamples(0), totalSamples(0),
 totalTimes(0), maxRejects(maxRej), consecRejects(0), pLarge(largeProb),
 pMicro(microProb), range(rng), useVariance(useV), sampleImage(NULL),
 timeImage(NULL), offset(NULL), rngRotation(NULL), rngBase(0),
 rngOffset(0), large(true), stamp(0), numMicro(-1), posMicro(-1), weight(0.f),
 LY(0.f), alpha(0.f), totalLY(0.f), sampleCount(0.f)
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
}

MetropolisSampler::~MetropolisSampler() {
	FreeAligned(sampleImage);
	FreeAligned(timeImage);
	FreeAligned(rngSamples);
	FreeAligned(rngRotation);
	delete[] offset;
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
	// Compute number of non lazy samples
	sampler->normalSamples = SAMPLE_FLOATS;
	for (i = 0; i < sample->n1D.size(); ++i)
		sampler->normalSamples += sample->n1D[i];
	for (i = 0; i < sample->n2D.size(); ++i)
		sampler->normalSamples += 2 * sample->n2D[i];

	// Compute number of lazy samples and initialize management data
	sampler->totalSamples = sampler->normalSamples;
	sampler->offset = new int[sample->nxD.size()];
	sampler->totalTimes = 0;
	for (i = 0; i < sample->nxD.size(); ++i) {
		sampler->offset[i] = sampler->totalSamples;
		sampler->totalTimes += sample->nxD[i];
		sampler->totalSamples += sample->dxD[i] * sample->nxD[i];
	}

	// Allocate sample image to hold the current reference
	sampler->sampleImage = AllocAligned<float>(sampler->totalSamples);
	sampler->timeImage = AllocAligned<int>(sampler->totalTimes);

	// Fetch first contribution buffer from pool
	sampler->contribBuffer = sampler->film->scene->contribPool->Next(NULL);

	// Compute best offset between sample vectors in the rng
	// TODO use the smallest gcf of totalSamples and rngN that is greater
	// than totalSamples or equal
	sampler->rngOffset = sampler->totalSamples;
	if (sampler->rngOffset >= rngN)
		sampler->rngOffset = sampler->rngOffset % (rngN - 1) + 1;
	// Current base index, minus offset to compensate for the advance done
	// in GetNextSample
	sampler->rngBase = rngN - sampler->rngOffset;
	// Allocate memory for the Cranley-Paterson rotation vector
	sampler->rngRotation = AllocAligned<float>(sampler->totalSamples);
}

// interface for new ray/samples from scene
bool MetropolisSampler::GetNextSample(Sample *sample, u_int *use_pos)
{
	sample->sampler = this;
	const float mutationSelector = tspack->rng->floatValue();
	large = (mutationSelector < pLarge);
	if (sampleImage == NULL) {
		initMetropolis(this, sample);
		large = true;
	}

	// Advance to the next vector in the QMC sequence and stay inside the
	// array bounds
	rngBase += rngOffset;
	if (rngBase > rngN - 1)
		rngBase -= rngN;
	// If all possible combinations have been used,
	// change the Cranley-Paterson rotation vector
	// This is also responsible for first time initialization of the vector
	if (rngBase == 0) {
		// This is a safe point to stop without too visible patterns
		// if the render has to stop
		if (film->enoughSamplePerPixel)
			return false;
		for (int i = 0; i < totalSamples; ++i)
			rngRotation[i] = tspack->rng->floatValue();
	}
	if (large) {
		// *** large mutation ***
		// Initialize all non lazy samples
		sample->imageX = rngGet(0) * (xPixelEnd - xPixelStart) + xPixelStart;
		sample->imageY = rngGet(1) * (yPixelEnd - yPixelStart) + yPixelStart;
		sample->lensU = rngGet(2);
		sample->lensV = rngGet(3);
		sample->time = rngGet(4);
		sample->wavelengths = rngGet(5);
		sample->singleWavelength = rngGet(6);
		for (int i = SAMPLE_FLOATS; i < normalSamples; ++i)
			sample->oneD[0][i - SAMPLE_FLOATS] = rngGet(i);
		// Reset number of mutations for lazy samples
		for (int i = 0; i < totalTimes; ++i)
			sample->timexD[0][i] = -1;
		// Reset number of mutations for whole sample
		sample->stamp = 0;
		numMicro = -1;
	} else {
		// *** small mutation ***
		// Select between full mutation or mutation of one node
		if (1.f - mutationSelector < pMicro)
			numMicro = min<int>(sample->nxD.size(), Float2Int((1.f - mutationSelector) / pMicro * (sample->nxD.size() + 1)));
		else
			numMicro = -1;
		if (numMicro > 0) {
			// Only one node will be mutated,
			// select it if it's not the non lazy samples
			u_int maxPos = 0;
			for (; maxPos < sample->nxD[numMicro - 1]; ++maxPos)
				if (sample->timexD[numMicro - 1][maxPos] < sample->stamp)
					break;
			posMicro = min<int>(sample->nxD[numMicro - 1] - 1, Float2Int(tspack->rng->floatValue() * maxPos));
		} else {
			// Mutation of non lazy samples
			// (also for whole sample mutation)
			posMicro = -1;
			sample->imageX = mutateScaled(sampleImage[0],
				rngGet(0), xPixelStart, xPixelEnd, range);
			sample->imageY = mutateScaled(sampleImage[1],
				rngGet(1), yPixelStart, yPixelEnd, range);
			sample->lensU = mutate(sampleImage[2], rngGet(2));
			sample->lensV = mutate(sampleImage[3], rngGet(3));
			sample->time = mutate(sampleImage[4], rngGet(4));
			sample->wavelengths = mutate(sampleImage[5], rngGet(5));
			sample->singleWavelength = mutateScaled(sampleImage[6],
				rngGet(6), 0.f, 1.f, 1.f);
			for (int i = SAMPLE_FLOATS; i < normalSamples; ++i)
				sample->oneD[0][i - SAMPLE_FLOATS] =
					mutate(sampleImage[i], rngGet(i));
		}
		// Increase reference mutation count
		++(sample->stamp);
	}
    return true;
}

float *MetropolisSampler::GetLazyValues(Sample *sample, u_int num, u_int pos)
{
	// Get size and position of current lazy values node
	const u_int size = sample->dxD[num];
	float *data = sample->xD[num] + pos * size;
	// Get the reference number of mutations
	int stampLimit = sample->stamp;
	// If the current node doesn't need mutation, use previous value
	if (numMicro >= 0 && num != numMicro - 1 && pos != posMicro)
		--stampLimit;
	// If we are at the target, don't do anything
	if (sample->timexD[num][pos] != stampLimit) {
		// If the node has not yet been initialized, do it now
		// otherwise get the last known value from the sample image
		if (sample->timexD[num][pos] == -1) {
			for (u_int i = 0; i < size; ++i)
				data[i] = rngGet(normalSamples + pos * size + i);
			sample->timexD[num][pos] = 0;
		} else {
			float *image = sampleImage + offset[num] + pos * size;
			for (u_int i = 0; i < size; ++i)
				data[i] = image[i];
		}
		// Mutate as needed
		for (; sample->timexD[num][pos] < stampLimit; ++(sample->timexD[num][pos])) {
			for (u_int i = 0; i < size; ++i)
				data[i] = mutate(data[i], rngGet2(normalSamples + pos * size + i, rngOffset * (stampLimit - sample->timexD[num][pos] + 1)));
		}
	}
	return data;
}

void MetropolisSampler::AddSample(const Sample &sample)
{
	vector<Contribution> &newContributions(sample.contributions);
	float newLY = 0.f;
	for(u_int i = 0; i < newContributions.size(); ++i) {
		float ly = newContributions[i].color.y();
		if (ly > 0.f) {
			if (useVariance && newContributions[i].variance > 0.f)
				newLY += ly * newContributions[i].variance;
			else
				newLY += ly;
		}
	}
	// calculate meanIntensity
	if (large) {
		totalLY += newLY;
		++sampleCount;
	}
	const float meanIntensity = totalLY > 0.f ? totalLY / sampleCount : 1.f;

	contribBuffer->AddSampleCount(1.f);

	// calculate accept probability from old and new image sample
	float accProb;
	if (LY > 0.f)
		accProb = min(1.f, newLY / LY);
	else
		accProb = 1.f;
	float newWeight = accProb + (large ? 1.f : 0.f);
	weight += 1.f - accProb;
	// try or force accepting of the new sample
	if (accProb == 1.f || consecRejects >= maxRejects || tspack->rng->floatValue() < accProb) {
		// Add accumulated contribution of previous reference sample
		const float norm = weight * meanIntensity / (LY + pLarge * meanIntensity);
		for(u_int i = 0; i < oldContributions.size(); ++i) {
			// Radiance - added new use of contributionpool/buffers
			if(&oldContributions && !contribBuffer->Add(&oldContributions[i], norm)) {
				contribBuffer = film->scene->contribPool->Next(contribBuffer);
				contribBuffer->Add(&oldContributions[i], norm);
			}
		}
		// Save new contributions for reference
		weight = newWeight;
		LY = newLY;
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
		const float norm = newWeight * meanIntensity / (newLY + pLarge * meanIntensity);
		for(u_int i = 0; i < newContributions.size(); ++i) {
			// Radiance - added new use of contributionpool/buffers
			if(!contribBuffer->Add(&newContributions[i], norm)) {
				contribBuffer = film->scene->contribPool->Next(contribBuffer);
				contribBuffer->Add(&newContributions[i], norm);
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
	int maxConsecRejects = params.FindOneInt("maxconsecrejects", 512);	// number of consecutive rejects before a next mutation is forced
	float largeMutationProb = params.FindOneFloat("largemutationprob", 0.4f);	// probability of generating a large sample mutation
	float microMutationProb = params.FindOneFloat("micromutationprob", 0.f);	// probability of generating a micro sample mutation
	float range = params.FindOneFloat("mutationrange", (xEnd - xStart + yEnd - yStart) / 32.);	// maximum distance in pixel for a small mutation
	bool useVariance = params.FindOneBool("usevariance", false);

	return new MetropolisSampler(xStart, xEnd, yStart, yEnd, maxConsecRejects,
		largeMutationProb, microMutationProb, range, useVariance);
}

static DynamicLoader::RegisterSampler<MetropolisSampler> r("metropolis");
