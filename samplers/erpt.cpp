/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// initial metropolis light transport sample integrator
// by radiance

// TODO: add scaling of output image samples

// erpt.cpp*
#include "erpt.h"
#include "dynload.h"

using namespace lux;

// mutate a value in the range [0-1]
static float mutate(const float x)
{
	static const float s1 = 1/1024., s2 = 1/16.;
	float dx = s2 * exp(-log(s2/s1) * lux::random::floatValue());
	if (lux::random::floatValue() < 0.5) {
		float x1 = x + dx;
		return (x1 > 1) ? x1 - 1 : x1;
	} else {
		float x1 = x - dx;
		return (x1 < 0) ? x1 + 1 : x1;
	}
}

// mutate a value in the range [min-max]
static float mutateScaled(const float x, const float mini, const float maxi, const float range)
{
//	static const float s1 = 16.;
	float dx = range * exp(-log(2. * range/*s1*/) * lux::random::floatValue());
	if (lux::random::floatValue() < 0.5) {
		float x1 = x + dx;
		return (x1 > maxi) ? x1 - maxi + mini : x1;
	} else {
		float x1 = x - dx;
		return (x1 < mini) ? x1 - mini + maxi : x1;
	}
}

// Metropolis method definitions
ERPTSampler::ERPTSampler(int xStart, int xEnd, int yStart, int yEnd, int totMutations, float rng) :
 Sampler(xStart, xEnd, yStart, yEnd, 0), L(0.),
 totalSamples(0), totalTimes(0), totalMutations(totMutations), chain(0),
 numChains(0), mutation(0), consecRejects(0), stamp(0),
 range(rng), weight(0.), alpha(0.), baseImage(NULL), sampleImage(NULL),
 timeImage(NULL)
{
}

// Copy
ERPTSampler* ERPTSampler::clone() const
{
	ERPTSampler *newSampler = new ERPTSampler(*this);
	newSampler->totalSamples = 0;
	newSampler->sampleImage = NULL;
	return newSampler;
}

static void initERPT(ERPTSampler *sampler, const Sample *sample)
{
	u_int i;
	sampler->normalSamples = 5;
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
	sampler->baseImage = (float *)AllocAligned(sampler->totalSamples * sizeof(float));
	sampler->timeImage = (int *)AllocAligned(sampler->totalTimes * sizeof(int));
}

// interface for new ray/samples from scene
bool ERPTSampler::GetNextSample(Sample *sample, u_int *use_pos)
{
	sample->sampler = this;
	if (sampleImage == NULL) {
		initERPT(this, sample);
	}
	if ((chain == 0 && mutation == 0) || initCount < initSamples) {
		// *** new seed ***
		sample->imageX = lux::random::floatValue() * (xPixelEnd - xPixelStart) + xPixelStart;
		sample->imageY = lux::random::floatValue() * (yPixelEnd - yPixelStart) + yPixelStart;
		sample->lensU = lux::random::floatValue();
		sample->lensV = lux::random::floatValue();
		sample->time = lux::random::floatValue();
		for (int i = 5; i < normalSamples; ++i)
			sample->oneD[0][i - 5] = lux::random::floatValue();
		for (int i = 0; i < totalTimes; ++i)
			sample->timexD[0][i] = -1;
		sample->stamp = 0;
	} else {
		if (mutation == 0) {
			// *** new chain ***
			sample->imageX = baseImage[0];
			sample->imageY = baseImage[1];
			sample->lensU = baseImage[2];
			sample->lensV = baseImage[3];
			sample->time = baseImage[4];
			for (int i = 5; i < totalSamples; ++i)
				sample->oneD[0][i - 5] = baseImage[i];
			for (int i = 0; i < totalTimes; ++i) {
				if (sample->timexD[0][i] != -1)
					sample->timexD[0][i] = 0;
			}
			sample->stamp = 0;
		}
		// *** small mutation ***
		// mutate current sample
		sample->imageX = mutateScaled(sampleImage[0], xPixelStart, xPixelEnd, range);
		sample->imageY = mutateScaled(sampleImage[1], yPixelStart, yPixelEnd, range);
		sample->lensU = mutate(sampleImage[2]);
		sample->lensV = mutate(sampleImage[3]);
		sample->time = mutate(sampleImage[4]);
		for (int i = 5; i < normalSamples; ++i)
			sample->oneD[0][i - 5] = mutate(sampleImage[i]);
		++(sample->stamp);
	}

    return true;
}

float *ERPTSampler::GetLazyValues(Sample *sample, u_int num, u_int pos)
{
	float *data = sample->xD[num] + pos * sample->dxD[num];
	if (sample->timexD[num][pos] != sample->stamp) {
		if (sample->timexD[num][pos] == -1) {
			for (u_int i = 0; i < sample->dxD[num]; ++i) {
				data[i] = lux::random::floatValue();
				baseImage[offset[num] + pos * sample->dxD[num] + i] = data[i];
			}
			sample->timexD[num][pos] = 0;
		} else {
			for (u_int i = 0; i < sample->dxD[num]; ++i){
				data[i] = sampleImage[offset[num] +
					pos * sample->dxD[num] + i];
			}
		}
		for (; sample->timexD[num][pos] < sample->stamp; ++(sample->timexD[num][pos])) {
			for (u_int i = 0; i < sample->dxD[num]; ++i)
				data[i] = mutate(data[i]);
		}
	}
	return data;
}

// interface for adding/accepting a new image sample.
void ERPTSampler::AddSample(const Sample &sample, const Ray &ray,
			   const SWCSpectrum &newL, float newAlpha, Film *film)
{
	float newLY = newL.y();
	// calculate meanIntensity
	if (initCount < initSamples) {
		meanIntensity += newLY;
		++(initCount);
		if (initCount < initSamples)
			return;
		if (meanIntensity == 0.) meanIntensity = 1.;
		meanIntensity /= initSamples * totalMutations;
	}
	// calculate the number of chains on a new seed
	if (chain == 0 && mutation == 0)
		numChains = Floor2Int(lux::random::floatValue() + newLY / (meanIntensity * totalSamples));
	// calculate accept probability from old and new image sample
	float LY = L.y();
	float accProb = min(1.0f, newLY / LY);
	float newWeight = accProb * meanIntensity;
	weight += (1. - accProb) * meanIntensity;
	if (mutation == 0)
		accProb = 1.;

	// try accepting of the new sample
	if (accProb == 1. || lux::random::floatValue() < accProb /*|| consecRejects > totalMutations / 10*/) {
		L *= weight / LY;
		film->AddSample(sampleImage[0], sampleImage[1], L, alpha);
		weight = newWeight;
		L = newL.ToXYZ();
		alpha = newAlpha;
		sampleImage[0] = sample.imageX;
		sampleImage[1] = sample.imageY;
		sampleImage[2] = sample.lensU;
		sampleImage[3] = sample.lensV;
		sampleImage[4] = sample.time;
		for (int i = 5; i < totalSamples; ++i)
			sampleImage[i] = sample.oneD[0][i - 5];
		for (int i = 0 ; i < totalTimes; ++i)
			timeImage[i] = sample.timexD[0][i];
		stamp = sample.stamp;
		if (chain == 0 && mutation == 0) {
			for (int i = 0; i < totalSamples; ++i)
				baseImage[i] = sampleImage[i];
		}
		consecRejects = 0;
	} else {
		film->AddSample(sample.imageX, sample.imageY, newL * (newWeight / newLY), newAlpha);
		for (int i = 0; i < totalTimes; ++i)
			sample.timexD[0][i] = timeImage[i];
		sample.stamp = stamp;
		++consecRejects;
	}
	if (++mutation >= totalMutations) {
		mutation = 0;
		if (++chain >= numChains)
			chain = 0;
	}
}

Sampler* ERPTSampler::CreateSampler(const ParamSet &params, const Film *film)
{
	int xStart, xEnd, yStart, yEnd;
	film->GetSampleExtent(&xStart, &xEnd, &yStart, &yEnd);
	initSamples = params.FindOneInt("initsamples", 100000);
	initCount = 0;
	meanIntensity = 0.;
	int totMutations = params.FindOneInt("chainlength", 512);	// number of mutations from a given seed
	float range = params.FindOneFloat("mutationrange", (xEnd - xStart + yEnd - yStart) / 32.);	// maximum distance in pixel for a small mutation
	return new ERPTSampler(xStart, xEnd, yStart, yEnd, totMutations, range);
}

int ERPTSampler::initCount, ERPTSampler::initSamples;
float ERPTSampler::meanIntensity;

