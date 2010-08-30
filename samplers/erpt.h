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

// erpt.h*

#ifndef LUX_ERPT_H
#define LUX_ERPT_H

#include "sampling.h"
#include "paramset.h"
#include "film.h"

namespace lux
{

class ERPTSampler : public Sampler {
public:
	class ERPTData {
	public:
		ERPTData(const Sample &sample);
		~ERPTData();
		u_int normalSamples, totalSamples, totalTimes;
		float *baseImage, *sampleImage, *currentImage;
		int *baseTimeImage, *timeImage, *currentTimeImage;
		u_int *offset;
		u_int numChains, chain, mutation;
		int stamp;
		float baseLY, quantum, weight, LY, alpha;
		vector<Contribution> oldContributions, baseContributions;
		double totalLY, sampleCount;
	};
	ERPTSampler(u_int totMutations, float rng, Sampler *sampler);
	virtual ~ERPTSampler();

	virtual void InitSample(Sample *sample) const {
		sample->samplerData = new ERPTData(*sample);
	}
	virtual void SetFilm(Film* f) { film = f; baseSampler->SetFilm(f); }
	virtual void GetBufferType(BufferType *type) {*type = BUF_TYPE_PER_SCREEN;}
	virtual u_int GetTotalSamplePos() { return baseSampler->GetTotalSamplePos(); }
	virtual u_int RoundSize(u_int size) const { return baseSampler->RoundSize(size); }
	virtual bool GetNextSample(Sample *sample, u_int *use_pos);
	virtual float *GetLazyValues(const Sample &sample, u_int num, u_int pos);
	//void AddSample(float imageX, float imageY, const Sample &sample, const Ray &ray, const XYZColor &L, float alpha, int id=0);
	virtual void AddSample(const Sample &sample, Film &film);
	static Sampler *CreateSampler(const ParamSet &params, const Film *film);

	u_int totalMutations;
	float pMicro, range;
	Sampler *baseSampler;
};

}//namespace lux

#endif // LUX_METROSAMPLER_H

