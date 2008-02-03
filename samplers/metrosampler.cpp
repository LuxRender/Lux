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

// metrosampler.cpp*
#include "metrosampler.h"
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
	static const float s1 = 64.;
	float dx = range * exp(-log(s1) * lux::random::floatValue());
	if (lux::random::floatValue() < 0.5) {
		float x1 = x + dx;
		return (x1 > maxi) ? x1 - maxi + mini : x1;
	} else {
		float x1 = x - dx;
		return (x1 < mini) ? x1 - mini + maxi : x1;
	}
}

// Metropolis method definitions
MetropolisSampler::MetropolisSampler(int xStart, int xEnd, int yStart, int yEnd, int maxRej, float largeProb, float rng) :
 Sampler(xStart, xEnd, yStart, yEnd, 0), large(true), L(0.),
 totalSamples(0), totalTimes(0), maxRejects(maxRej), consecRejects(0),
 pLarge(largeProb), range(rng), weight(0.), alpha(0.), sampleImage(NULL),
 timeImage(NULL)
{
}

// Copy
MetropolisSampler* MetropolisSampler::clone() const
{
	MetropolisSampler *newSampler = new MetropolisSampler(*this);
	newSampler->totalSamples = 0;
	newSampler->sampleImage = NULL;
	return newSampler;
}

// interface for new ray/samples from scene
bool MetropolisSampler::GetNextSample(Sample *sample, u_int *use_pos)
{
	large = (lux::random::floatValue() < pLarge) || initCount < initSamples;
	if (sampleImage == NULL) {
		u_int i;
		normalSamples = 5;
		for (i = 0; i < sample->n1D.size(); ++i)
			normalSamples += sample->n1D[i];
		for (i = 0; i < sample->n2D.size(); ++i)
			normalSamples += 2 * sample->n2D[i];
		totalSamples = normalSamples;
		offset = new int[sample->nxD.size()];
		totalTimes = 0;
		for (i = 0; i < sample->nxD.size(); ++i) {
			offset[i] = totalSamples;
			totalTimes += sample->nxD[i];
			totalSamples += sample->dxD[i] * sample->nxD[i];
		}
		sampleImage = (float *)AllocAligned(totalSamples * sizeof(float));
		timeImage = (int *)AllocAligned(totalTimes * sizeof(int));
		large = true;
	}
	if (large) {
		// *** large mutation ***
		sample->imageX = lux::random::floatValue() * (xPixelEnd - xPixelStart) + xPixelStart;
		sample->imageY = lux::random::floatValue() * (yPixelEnd - yPixelStart) + yPixelStart;
		sample->lensU = lux::random::floatValue();
		sample->lensV = lux::random::floatValue();
		sample->time = lux::random::floatValue();
		for (int i = 5; i < normalSamples; ++i)
			sample->oneD[0][i - 5] = lux::random::floatValue();
		for (u_int i = 0; i < sample->nxD.size(); ++i)
			for (u_int j = 0; j < sample->nxD[i]; ++j)
				sample->timexD[i][j] = -1;
	} else {
		// *** small mutation ***
		// mutate current sample
		sample->imageX = mutateScaled(sampleImage[0], xPixelStart, xPixelEnd, range);
		sample->imageY = mutateScaled(sampleImage[1], yPixelStart, yPixelEnd, range);
		sample->lensU = mutate(sampleImage[2]);
		sample->lensV = mutate(sampleImage[3]);
		sample->time = mutate(sampleImage[4]);
		for (int i = 5; i < normalSamples; ++i)
			sample->oneD[0][i - 5] = mutate(sampleImage[i]);
		for (int i = 0; i < totalTimes; ++i)
			sample->timexD[0][i] = timeImage[i];
	}

    return true;
}

float *MetropolisSampler::GetLazyValues(Sample *sample, u_int num, u_int pos)
{
	float *data = sample->xD[num] + pos * sample->dxD[num];
	if (sample->timexD[num][pos] != sample->stamp) {
		if (sample->timexD[num][pos] == -1) {
			for (u_int i = 0; i < sample->dxD[num]; ++i)
				data[i] = lux::random::floatValue();
			sample->timexD[num][pos] = 0;
			return data;
		}
		for (u_int i = 0; i < sample->dxD[num]; ++i)
			data[i] = sampleImage[offset[num] +
				pos * sample->dxD[num] + i];
		for (; sample->timexD[num][pos] < sample->stamp; ++(sample->timexD[num][pos])) {
			for (u_int i = 0; i < sample->dxD[num]; ++i)
				data[i] = mutate(data[i]);
		}
	}
	return data;
}

// interface for adding/accepting a new image sample.
void MetropolisSampler::AddSample(const Sample &sample, const Ray &ray,
			   const Spectrum &newL, float newAlpha, Film *film)
{
	float newLY = newL.y();
	// calculate meanIntensity
	if (initCount < initSamples) {
		meanIntensity += newLY / initSamples;
		++(initCount);
		if (initCount < initSamples)
			return;
		if (meanIntensity == 0.) meanIntensity = 1.;
	}
	// calculate accept probability from old and new image sample
	float LY = L.y();
	float accProb = min(1.0f, newLY / LY);
	float newWeight = (accProb + large ? 1. : 0.) / (newLY / meanIntensity + pLarge);
	weight += (1. - accProb) / (LY / meanIntensity + pLarge);

	// try or force accepting of the new sample
	if (consecRejects > maxRejects || lux::random::floatValue() < accProb ) {
		film->AddSample(sampleImage[0], sampleImage[1], L * weight, alpha);
		weight = newWeight;
		L = newL;
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
		if (large)
			sample.stamp = 0;
		++sample.stamp;
		consecRejects = 0;
	} else {
		film->AddSample(sample.imageX, sample.imageY, newL * newWeight, newAlpha);
		for (int i = 0; i < totalTimes; ++i)
			sample.timexD[0][i] = timeImage[i];
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
	float range = params.FindOneFloat("mutationrange", (xEnd - xStart + yEnd - yStart) / 32.);	// maximum distance in pixel for a small mutation
	return new MetropolisSampler(xStart, xEnd, yStart, yEnd, maxConsecRejects, largeMutationProb, range);
}

int MetropolisSampler::initCount, MetropolisSampler::initSamples;
float MetropolisSampler::meanIntensity;

