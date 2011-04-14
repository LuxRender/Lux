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

// random.cpp*
#include "sampling.h"
#include "paramset.h"
#include "film.h"

namespace lux
{

class RandomSampler : public Sampler
{
public:
	class RandomData {
	public:
		RandomData(int xPixelStart, int yPixelStart, u_int pixelSamples);
		~RandomData();
		int xPos, yPos;
		u_int samplePos;
	};
	RandomSampler(int xstart, int xend, int ystart, int yend,
		u_int ps, string pixelsampler);
	virtual ~RandomSampler();

	virtual void *InitSampleData(const Sample &sample) const {
		return new RandomData(xPixelStart, yPixelStart,
			pixelSamples);
	}
	virtual u_int GetTotalSamplePos();
	virtual bool GetNextSample(Sample *sample, void *samplerData);
	virtual float *GetLazyValues(const Sample &sample, void *samplerData, u_int num, u_int pos);
	virtual u_int RoundSize(u_int sz) const { return sz; }
	virtual void GetBufferType(BufferType *type) {*type = BUF_TYPE_PER_PIXEL;}

	static Sampler *CreateSampler(const ParamSet &params, const Film *film);
private:
	// RandomSampler Private Data
	bool jitterSamples;
	u_int pixelSamples;
	u_int totalPixels;
	PixelSampler* pixelSampler;

	fast_mutex sampPixelPosMutex;
	u_int sampPixelPos;
};

}//namespace lux
