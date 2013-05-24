/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

#include "sampling.h"
#include "paramset.h"
#include "film.h"

namespace lux
{

class SobolSampler : public Sampler
{
public:
	class SobolData {
	public:
		SobolData(const Sampler &sampler, const Sample &sample);
		~SobolData();

		u_int SobolDimension(const SobolSampler &sampler,
			const u_int index, const u_int dimension) const;
		float GetSample(const SobolSampler &sampler, const u_int index) const;

		float rng0, rng1;
		u_int pass;

		u_int nxD;
		float **xD;

		boost::shared_array<float> samplingMap;		
		u_int noiseAwareMapVersion;
		u_int userSamplingMapVersion;
		boost::shared_ptr<Distribution2D> samplingDistribution2D;
	};

	SobolSampler(int xstart, int xend, int ystart, int yend, bool useNoise);
	virtual ~SobolSampler();

	virtual void InitSample(Sample *sample) const;
	virtual void FreeSample(Sample *sample) const {
		delete static_cast<SobolData *>(sample->samplerData);
	}
	virtual u_int GetTotalSamplePos();
	virtual bool GetNextSample(Sample *sample);
	virtual float GetOneD(const Sample &sample, u_int num, u_int pos);
	virtual void GetTwoD(const Sample &sample, u_int num, u_int pos,
		float u[2]);
	virtual float *GetLazyValues(const Sample &sample, u_int num, u_int pos);
	virtual u_int RoundSize(u_int sz) const { return sz; }
	virtual void GetBufferType(BufferType *type) {*type = BUF_TYPE_PER_PIXEL;}

	static Sampler *CreateSampler(const ParamSet &params, Film *film);

private:
	// SobolSampler Private Data
	mutable fast_mutex initDirectionsMutex;
	mutable u_int *directions;
	mutable vector<u_int> offset1D, offset2D, offsetxD;

	u_int totalPixels;
};

}//namespace lux
