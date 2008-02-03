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
static float mutateImage(const float x, const float mini, const float maxi, const float range)
{
//	return x + (2. * lux::random::floatValue() - 1.) * range;
	static const float s1 = 1/1024., s2 = 1/16.;
	float dx = min(maxi - mini, range) * s2 * exp(-log(s2/s1) * lux::random::floatValue());
	if (lux::random::floatValue() < 0.5) {
		float x1 = x + dx;
		return (x1 > maxi) ? 2 * maxi - x1 : x1;
	} else {
		float x1 = x - dx;
		return (x1 < mini) ? 2 * mini - x1 : x1;
	}
}

// Metropolis method definitions
ERPTSampler::ERPTSampler(Sampler *baseSampler, int totMut, int maxRej, float rng) :
 Sampler(baseSampler->xPixelStart, baseSampler->xPixelEnd,
 baseSampler->yPixelStart, baseSampler->yPixelEnd,
 baseSampler->samplesPerPixel), sampler(baseSampler), L(0.), Ld(0.),
 totalSamples(0), totalMutations(totMut), maxRejects(maxRej), mutations(0), rejects(0), count(0), range(rng),
 offset(NULL), sampleImage(NULL)
{
}

// Copy
ERPTSampler* ERPTSampler::clone() const
{
	ERPTSampler *newSampler = new ERPTSampler(*this);
	newSampler->sampler = sampler->clone();
	if (totalSamples > 0)
		newSampler->sampleImage = (float *)AllocAligned(totalSamples * sizeof(float));
	return newSampler;
}

// interface for new ray/samples from scene
bool ERPTSampler::GetNextSample(Sample *sample, u_int *use_pos)
{
	if (sampleImage == NULL) {
		unsigned int i;
		normalSamples = 5;
		for (i = 0; i < sample->n1D.size(); ++i)
			normalSamples += sample->n1D[i];
		for (i = 0; i < sample->n2D.size(); ++i)
			normalSamples += 2 * sample->n2D[i];
		totalSamples = normalSamples;
		offset = new u_int[sample->nxD.size()];
		for (i = 0; i < sample->nxD.size(); ++i) {
			offset[i] = totalSamples;
			totalSamples += sample->dxD[i] * sample->nxD[i];
		}
		sampleImage = (float *)AllocAligned(totalSamples * sizeof(float));
	}
	if (mutations == 0) {
		// *** large mutation ***
		// fetch samples from sampler
		if(!sampler->GetNextSample(sample, use_pos))
			return false;
	} else {
		// *** small mutation ***
		// mutate current sample
		sample->imageX = mutateImage(sampleImageX, xPixelStart, xPixelEnd, range);
		sample->imageY = mutateImage(sampleImageY, yPixelStart, yPixelEnd, range);
		sample->lensU = mutate(sampleImage[2]);
		sample->lensV = mutate(sampleImage[3]);
		sample->time = mutate(sampleImage[4]);
		for (int i = 5; i < normalSamples; ++i)
			sample->oneD[0][i - 5] = mutate(sampleImage[i]);
	}

    return true;
}

float *ERPTSampler::GetLazyValues(Sample *sample, u_int num, u_int pos)
{
	float *data = sample->xD[num] + pos * sample->dxD[num];
	if (sample->timexD[num][pos] == -1) {
		sampler->GetLazyValues(sample, num, pos);
		sample->timexD[num][pos] = 0;
	}
	float *imageData = sampleImage + offset[num] + pos * sample->dxD[num];
	for (;sample->timexD[num][pos] < sample->stamp; ++(sample->timexD[num][pos]))
		for (u_int i = 0; i < sample->dxD[i]; ++i)
			data[i] = mutate(imageData[i]);
	return data;
}

// interface for adding/accepting a new image sample.
void ERPTSampler::AddSample(const Sample &sample, const Ray &ray,
			   const Spectrum &newL, float alpha, Film *film)
{
	// calculate accept probability from old and new image sample
	// TODO take path density into account
	float newLY = newL.y(), LY = L.y();
	float accprob = min(1.0f, newLY / LY);

	if (count == 0) {
		Ld = Spectrum(0.);
		sampleImageX = sampleImageY = 0.;
	}

	if (mutations == 0) {
		Ld += newL / (sampler->samplesPerPixel * totalMutations);
		sampleImage[0] = sampleImageX += sample.imageX / sampler->samplesPerPixel;
		sampleImage[1] = sampleImageY += sample.imageY / sampler->samplesPerPixel;
		++count;
	}

	if (count == sampler->samplesPerPixel)
		++mutations;
	if (mutations == 0)
		return;

	// add new sample
	if (newLY > 0.f)
		film->AddSample(sample.imageX, sample.imageY,
		        newL * Ld * (accprob / newLY), alpha);

	// try or force accepting of the new sample
	if (lux::random::floatValue() < accprob || mutations == 1 || rejects == maxRejects) {
		sampleImage[0] = sampleImageX = sample.imageX;
		sampleImage[1] = sampleImageY = sample.imageY;
		sampleImage[2] = sample.lensU;
		sampleImage[3] = sample.lensV;
		sampleImage[4] = sample.time;
		for (int i = 5; i < totalSamples; ++i)
			sampleImage[i] = sample.oneD[0][i - 5];
		L = newL;
		rejects = 0;
	} else {
		++rejects;
	}
	LY = L.y();
	if (LY > 0)
		film->AddSample(sampleImage[0], sampleImage[1],
			L * Ld * ((1. - accprob) / LY), alpha);
	if (mutations == totalMutations) {
		mutations = 0;
		count = 0;
	}
}

Sampler* ERPTSampler::CreateSampler(const ParamSet &params, const Film *film)
{
	int totalMutations = params.FindOneInt("totalmutations", 128);	// number of mutations before a next sample is taken
	int maxRejects = params.FindOneInt("maxconsecutiverejects", 10);	// number of consecutive rejects before a next mutation is accepted
	float range = params.FindOneFloat("range", 80.f);	// probability of generating a large sample mutation
	string samplername = params.FindOneString("basesampler", "random");
	Sampler *baseSampler = MakeSampler(samplername, params, film);
	if (baseSampler == NULL)
		return NULL;
	return new ERPTSampler(baseSampler, totalMutations, maxRejects, range);
}

