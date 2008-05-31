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

// erpt.cpp*
#include "erpt.h"
#include "dynload.h"

using namespace lux;

#define SAMPLE_FLOATS 7

// mutate a value in the range [0-1]
static float mutate(const float x)
{
	static const float s1 = 1.f / 1024.f, s2 = 1.f / 16.f;
	float randomValue = lux::random::floatValue();
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
static float mutateScaled(const float x, const float mini, const float maxi, const float range)
{
	float randomValue = lux::random::floatValue();
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
ERPTSampler::ERPTSampler(int xStart, int xEnd, int yStart, int yEnd,
		int totMutations, float rng, int sw) :
 Sampler(xStart, xEnd, yStart, yEnd, 1), LY(0.), gain(0.f),
 totalSamples(0), totalTimes(0), totalMutations(totMutations), chain(0),
 numChains(0), mutation(0), consecRejects(0), stamp(0),
 range(rng), weight(0.), alpha(0.), baseImage(NULL), sampleImage(NULL),
 timeImage(NULL), strataWidth(sw)
{
	// Allocate storage for image stratified samples
	strataSamples = (float *)AllocAligned(2 * sw * sw * sizeof(float));
	strataSqr = sw*sw;
	currentStrata = strataSqr;
}

ERPTSampler::~ERPTSampler() {
	FreeAligned(sampleImage);
	FreeAligned(baseImage);
	FreeAligned(timeImage);
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
		// Dade - we are at a valid checkpoint where we can stop the
		// rendering. Check if we have enough samples per pixel in the film.
		if (film->enoughSamplePerPixel)
			return false;
		if(currentStrata == strataSqr) {
			// Generate shuffled stratified image samples
			StratifiedSample2D(strataSamples, strataWidth, strataWidth, true);
			Shuffle(strataSamples, strataSqr, 2);
			currentStrata = 0;
		}
		// *** new seed ***
		sample->imageX = strataSamples[currentStrata*2] * (xPixelEnd - xPixelStart) + xPixelStart;
		sample->imageY = strataSamples[(currentStrata*2)+1] * (yPixelEnd - yPixelStart) + yPixelStart;
		currentStrata++;

		sample->lensU = lux::random::floatValue();
		sample->lensV = lux::random::floatValue();
		sample->time = lux::random::floatValue();
		sample->wavelengths = lux::random::floatValue();
		sample->singleWavelength = lux::random::floatValue();
		for (int i = SAMPLE_FLOATS; i < normalSamples; ++i)
			sample->oneD[0][i - SAMPLE_FLOATS] = lux::random::floatValue();
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
			sample->wavelengths = baseImage[5];
			sample->singleWavelength = lux::random::floatValue();//baseImage[6];
			for (int i = SAMPLE_FLOATS; i < totalSamples; ++i)
				sample->oneD[0][i - SAMPLE_FLOATS] = baseImage[i];
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
		sample->wavelengths = mutate(sampleImage[5]);
		sample->singleWavelength = lux::random::floatValue();//mutate(sampleImage[6]);
		for (int i = SAMPLE_FLOATS; i < normalSamples; ++i)
			sample->oneD[0][i - SAMPLE_FLOATS] = mutate(sampleImage[i]);
		++(sample->stamp);
	}

    return true;
}

float *ERPTSampler::GetLazyValues(Sample *sample, u_int num, u_int pos)
{
	float *data = sample->xD[num] + pos * sample->dxD[num];
	if (sample->timexD[num][pos] != sample->stamp) {
		if (sample->timexD[num][pos] == -1) {
			float *image = baseImage + offset[num] + pos * sample->dxD[num];
			for (u_int i = 0; i < sample->dxD[num]; ++i) {
				data[i] = lux::random::floatValue();
				image[i] = data[i];
			}
			sample->timexD[num][pos] = 0;
		} else {
			float *image = sampleImage + offset[num] + pos * sample->dxD[num];
			for (u_int i = 0; i < sample->dxD[num]; ++i){
				data[i] = image[i];
			}
		}
		for (int &time = sample->timexD[num][pos]; time < sample->stamp; ++time) {
			for (u_int i = 0; i < sample->dxD[num]; ++i)
				data[i] = mutate(data[i]);
		}
	}
	return data;
}

// interface for adding/accepting a new image sample.
void ERPTSampler::AddSample(float imageX, float imageY, const Sample &sample, const Ray &ray, const XYZColor &newL, float newAlpha, int id)
{
	if (!isSampleEnd) {
		sample.AddContribution(imageX, imageY, newL, newAlpha, id);
	} else {	// backward compatible with multiimage.cpp and path.cpp
		sample.contributions.clear();
		sample.AddContribution(imageX, imageY, newL, newAlpha, id);
	}
}

void ERPTSampler::AddSample(const Sample &sample)
{
	vector<Sample::Contribution> &newContributions(sample.contributions);
	float newLY = 0.0f;
	for(u_int i = 0; i < newContributions.size(); ++i)
		newLY += newContributions[i].color.y();
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
	if (chain == 0 && mutation == 0) {
		gain = newLY / (meanIntensity * totalSamples);
		if (gain < 1.f)
			numChains = Floor2Int(lux::random::floatValue() + gain);
		else
			numChains = Floor2Int(gain);
		if (numChains == 0)
			return;
		gain /= numChains;
		film->AddSampleCount(1.f); // TODO: add to correct buffer groups
	}
	// calculate accept probability from old and new image sample
	float accProb = min(1.0f, newLY / LY);
	if (mutation == 0)
		accProb = 1.f;
	float newWeight = accProb;
	weight += 1.f - accProb;

	// try accepting of the new sample
	if (accProb == 1.f || lux::random::floatValue() < accProb) {
		// Add accumulated contribution of previous reference sample
		weight *= gain * meanIntensity / LY;
		if (!isinf(weight) && LY > 0.f) {
			for(u_int i = 0; i < oldContributions.size(); ++i) {
				XYZColor color = oldContributions[i].color;
				color *= weight;
				film->AddSample(oldContributions[i].imageX, oldContributions[i].imageY,
					color, oldContributions[i].alpha,
					oldContributions[i].buffer, oldContributions[i].bufferGroup);
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
		if (chain == 0 && mutation == 0) {
			for (int i = 0; i < totalSamples; ++i)
				baseImage[i] = sampleImage[i];
		}
		consecRejects = 0;
	} else {
		// Add contribution of new sample before rejecting it
		newWeight *= gain * meanIntensity / newLY;
		if (!isinf(newWeight) && newLY > 0.f) {
			for(u_int i = 0; i < newContributions.size(); ++i) {
				XYZColor color = newContributions[i].color;
				color *= newWeight;
				film->AddSample(newContributions[i].imageX, newContributions[i].imageY,
					color, newContributions[i].alpha,
					newContributions[i].buffer, newContributions[i].bufferGroup);
			}
		}
		// Restart from previous reference
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
	int totMutations = params.FindOneInt("chainlength", 2000);	// number of mutations from a given seed
	float range = params.FindOneFloat("mutationrange", (xEnd - xStart + yEnd - yStart) / 50.);	// maximum distance in pixel for a small mutation
	int stratawidth = params.FindOneInt("stratawidth", 256);	// stratification of large mutation image samples (stratawidth*stratawidth)

	return new ERPTSampler(xStart, xEnd, yStart, yEnd, totMutations, range, stratawidth);
}

int ERPTSampler::initCount, ERPTSampler::initSamples;
float ERPTSampler::meanIntensity = 0.f;

