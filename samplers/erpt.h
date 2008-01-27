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
	ERPTSampler(Sampler *baseSampler, int totMut, int maxRej, float rng);
	virtual ERPTSampler* clone() const;
	u_int GetTotalSamplePos() { return sampler->GetTotalSamplePos(); }
	int RoundSize(int size) const { return sampler->RoundSize(size); }
	bool GetNextSample(Sample *sample, u_int *use_pos);
	void AddSample(const Sample &sample, const Ray &ray,
		const Spectrum &L, float alpha, Film *film);
	~ERPTSampler() { delete sampler; delete[] sampleImage; }
	static Sampler *CreateSampler(const ParamSet &params, const Film *film);

	Sampler *sampler;
	Spectrum L, Ld;
	int totalSamples, totalMutations, maxRejects, mutations, rejects, count;
	float range, sampleImageX, sampleImageY;
	float *sampleImage;
};

}//namespace lux

#endif // LUX_ERPT_H

