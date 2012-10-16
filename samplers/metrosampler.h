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

// metrosampler.h*

#ifndef LUX_METROSAMPLER_H
#define LUX_METROSAMPLER_H

#include <boost/shared_array.hpp>

#include "sampling.h"
#include "paramset.h"
#include "film.h"
#include "timer.h"

namespace lux
{
	
class MetropolisSampler : public Sampler {
public:
	class MetropolisData {
	public:
		MetropolisData(const MetropolisSampler &sampler);
		~MetropolisData();

		u_int normalSamples, totalSamples, totalTimes, consecRejects;
		float *sampleImage, *currentImage;
		int *timeImage, *currentTimeImage;
		u_int *offset, *timeOffset;
		float *rngRotation;
		u_int rngBase, rngOffset;
		int stamp, currentStamp;
		float weight, LY, alpha;
		vector <Contribution> oldContributions;
		double totalLY, sampleCount;

		boost::shared_array<float> samplingMap;
		u_int noiseAwareMapVersion;
		u_int userSamplingMapVersion;
		boost::shared_ptr<Distribution2D> samplingDistribution2D;

		bool large, cooldown;
	};

	MetropolisSampler(int xStart, int xEnd, int yStart, int yEnd,
		u_int maxRej, float largeProb, float rng,
		bool useV, bool useC, bool useNoise);
	virtual ~MetropolisSampler();

	virtual void InitSample(Sample *sample) const {
		sample->sampler = const_cast<MetropolisSampler *>(this);
		sample->samplerData = new MetropolisData(*this);
	}
	virtual void FreeSample(Sample *sample) const {
		delete static_cast<MetropolisData *>(sample->samplerData);
	}
	virtual u_int GetTotalSamplePos() { return 0; }
	virtual u_int RoundSize(u_int size) const { return size; }
	virtual bool GetNextSample(Sample *sample);
	virtual float GetOneD(const Sample &sample, u_int num, u_int pos);
	virtual void GetTwoD(const Sample &sample, u_int num, u_int pos,
		float u[2]);
	virtual float *GetLazyValues(const Sample &sample, u_int num, u_int pos);
	virtual void AddSample(const Sample &sample);
	static Sampler *CreateSampler(const ParamSet &params, Film *film);
		
	u_int maxRejects;
	float pLarge, range;
	u_int cooldownTime;
	float *rngSamples;
	bool useVariance, useNoiseAware;
};

}//namespace lux

#endif // LUX_METROSAMPLER_H

