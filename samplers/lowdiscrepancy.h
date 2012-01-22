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
 
// lowdiscrepancy.cpp*
#include "sampling.h"
#include "paramset.h"
#include "film.h"

namespace lux
{

// LDSampler Declarations
class LDSampler : public Sampler {
public:
	class LDData {
	public:
		LDData(const Sample &sample, int xPixelStart, int yPixelStart,
			u_int pixelSamples);
		~LDData();
		int xPos, yPos;
		u_int samplePos;
		float *imageSamples, *lensSamples, *timeSamples,
			*wavelengthsSamples, *singleWavelengthSamples;
		float **xD, **oneDSamples, **twoDSamples, **xDSamples;
		u_int n1D, n2D, nxD;
	};
	// LDSampler Public Methods
	LDSampler(int xstart, int xend,
	          int ystart, int yend,
			  u_int nsamp, string pixelsampler,
			  string *smplFileName);
	virtual ~LDSampler();

	virtual void InitSample(Sample *sample) const {
		sample->sampler = const_cast<LDSampler *>(this);
		sample->samplerData = new LDData(*sample, xPixelStart,
			yPixelStart, pixelSamples);
	}
	virtual void FreeSample(Sample *sample) const {
		delete static_cast<LDData *>(sample->samplerData);
	}
	virtual u_int RoundSize(u_int size) const {
		return RoundUpPow2(size);
	}
	virtual void GetBufferType(BufferType *type) {*type = BUF_TYPE_PER_PIXEL;}
	virtual u_int GetTotalSamplePos();
	virtual bool GetNextSample(Sample *sample);
	virtual float GetOneD(const Sample &sample, u_int num, u_int pos);
	virtual void GetTwoD(const Sample &sample, u_int num, u_int pos,
		float u[2]);
	virtual float *GetLazyValues(const Sample &sample, u_int num, u_int pos);

	static Sampler *CreateSampler(const ParamSet &params, const Film *film);

protected:
	virtual void WriteSampleInformationHeader(const Sample &sample);
	virtual void WriteSampleInformation(const Sample &sample);

private:
	// LDSampler Private Data
	u_int pixelSamples, totalPixels;
	PixelSampler* pixelSampler;

	fast_mutex sampPixelPosMutex;
	u_int sampPixelPos;
};

}//namespace lux

