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
static float mutateScaled(const float x, const float min, const float max)
{
	return mutate((x - min) / (max - min)) * (max - min) + min;
}

// Metropolis method definitions
MetropolisSampler::MetropolisSampler(Sampler *baseSampler, int maxRej, float largeProb) :
 Sampler(baseSampler->xPixelStart, baseSampler->xPixelEnd,
 baseSampler->yPixelStart, baseSampler->yPixelEnd,
 baseSampler->samplesPerPixel), sampler(baseSampler), L(0.), totalSamples(0),
 maxRejects(maxRej), consecRejects(0), pLarge(largeProb), sampleImage(NULL)
{
}

// Copy
MetropolisSampler* MetropolisSampler::clone() const
{
	MetropolisSampler *newSampler = new MetropolisSampler(*this);
	newSampler->sampler = sampler->clone();
	if (totalSamples > 0)
		newSampler->sampleImage = (float *)AllocAligned(totalSamples * sizeof(float));
	return newSampler;
}

// interface for new ray/samples from scene
bool MetropolisSampler::GetNextSample(Sample *sample, u_int *use_pos)
{
	bool large = (lux::random::floatValue() < pLarge);
	if (sampleImage == NULL) {
		int i;
		totalSamples = 5;
		for (i = 0; i < sample->n1D.size(); ++i)
			totalSamples += sample->n1D[i];
		for (i = 0; i < sample->n2D.size(); ++i)
			totalSamples += 2 * sample->n2D[i];
		sampleImage = (float *)AllocAligned(totalSamples * sizeof(float));
		large = true;
	}
	if (large) {
		// *** large mutation ***
		// fetch samples from sampler
		if(!sampler->GetNextSample(sample, use_pos))
			return false;
	} else {
		// *** small mutation ***
		// mutate current sample
		sample->imageX = mutateScaled(sampleImage[0], xPixelStart, xPixelEnd);
		sample->imageY = mutateScaled(sampleImage[1], yPixelStart, yPixelEnd);
		sample->lensU = mutate(sampleImage[2]);
		sample->lensV = mutate(sampleImage[3]);
		sample->time = mutate(sampleImage[4]);
		for (int i = 5; i < totalSamples; ++i)
			sample->oneD[0][i - 5] = mutate(sampleImage[i]);
	}

    return true;
}

// interface for adding/accepting a new image sample.
void MetropolisSampler::AddSample(const Sample &sample, const Ray &ray,
			   const Spectrum &newL, float alpha, Film *film)
{
	// calculate accept probability from old and new image sample
	float newLY = newL.y(), LY = L.y();
	float accprob = min(1.0f, newLY / LY);

	// add old sample
	if (LY > 0.f)
		film->AddSample(sampleImage[0], sampleImage[1],
		        L * (1. / LY) * (1. - accprob), alpha);

	// add new sample
	if (newLY > 0.f)
		film->AddSample(sample.imageX, sample.imageY,
			newL * (1. / newLY) * accprob, alpha);

	// try or force accepting of the new sample
	if (lux::random::floatValue() < accprob || consecRejects > maxRejects) {
		sampleImage[0] = sample.imageX;
		sampleImage[1] = sample.imageY;
		sampleImage[2] = sample.lensU;
		sampleImage[3] = sample.lensV;
		sampleImage[4] = sample.time;
		for (int i = 5; i < totalSamples; ++i)
			sampleImage[i] = sample.oneD[0][i - 5];
		L = newL;
		consecRejects = 0;
	} else {
		consecRejects++;
	}
}

Sampler* MetropolisSampler::CreateSampler(const ParamSet &params, const Film *film)
{
	int MaxConsecRejects = params.FindOneInt("maxconsecrejects", 512);	// number of consecutive rejects before a nex mutation is forced
	float LargeMutationProb = params.FindOneFloat("largemutationprob", 0.4f);	// probability of generating a large sample mutation
	string samplername = params.FindOneString("basesampler", "random");
	Sampler *baseSampler = MakeSampler(samplername, params, film);
	if (baseSampler == NULL)
		return NULL;
	return new MetropolisSampler(baseSampler, MaxConsecRejects, LargeMutationProb);
}

