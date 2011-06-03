/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

#ifndef LUX_PHOTON_SAMPLER_H
#define	LUX_PHOTON_SAMPLER_H

#include "lux.h"
#include "scene.h"
#include "sampling.h"

namespace lux
{

//------------------------------------------------------------------------------
// Samplers for photon phase
//------------------------------------------------------------------------------

enum PhotonSamplerType {
	HALTON, AMC
};

class PhotonSampler : public Sampler {
public:
	PhotonSampler() : Sampler(0, 0, 0, 0, 0) { }
	virtual ~PhotonSampler() { }
	virtual u_int GetTotalSamplePos() { return 0; }
	virtual u_int RoundSize(u_int size) const { return size; }
/*	virtual void InitSample(Sample *sample) const = 0;
	virtual void FreeSample(Sample *sample) const = 0;
	virtual bool GetNextSample(Sample *sample) = 0;
	virtual float GetOneD(const Sample &sample, u_int num, u_int pos) = 0;
	virtual void GetTwoD(const Sample &sample, u_int num, u_int pos,
		float u[2]) = 0;
	virtual float *GetLazyValues(const Sample &sample, u_int num, u_int pos) = 0;
	virtual void AddSample(const Sample &sample);*/
};

//------------------------------------------------------------------------------
// Halton Photon Sampler
//------------------------------------------------------------------------------

class HaltonPhotonSampler : public PhotonSampler {
public:
	class HaltonPhotonSamplerData {
	public:
		HaltonPhotonSamplerData(const Sample &sample, u_int sz) :
			halton(size, *(sample.rng)), size(sz),
			haltonOffset(sample.rng->floatValue()), pathCount(0) {
			if (sample.n1D.size() + sample.n2D.size() +
				sample.nxD.size() == 0) {
				values = NULL;
				return;
			}
			values = new float *[sample.n1D.size() +
				sample.n2D.size() + sample.nxD.size()];
			u_int n = 0;
			for (u_int i = 0; i < sample.n1D.size(); ++i)
				n += sample.n1D[i];
			for (u_int i = 0; i < sample.n2D.size(); ++i)
				n += 2 * sample.n2D[i];
			for (u_int i = 0; i < sample.nxD.size(); ++i)
				n += sample.dxD[i];
			if (n == 0) {
				values[0] = NULL;
				return;
			}
			float *buffer = new float[n];
			for (u_int i = 0; i < sample.n1D.size(); ++i) {
				values[i] = buffer;
				buffer += sample.n1D[i];
			}
			for (u_int i = 0; i < sample.n2D.size(); ++i) {
				values[i] = buffer;
				buffer += 2 * sample.n1D[i];
			}
			for (u_int i = 0; i < sample.nxD.size(); ++i) {
				values[i] = buffer;
				buffer += sample.dxD[i];
			}
		}
		~HaltonPhotonSamplerData() {
			delete[] values[0];
			delete[] values;
		}
		PermutedHalton halton;
		u_int size;
		float haltonOffset;
		u_int pathCount;
		float **values;
	};
	HaltonPhotonSampler() : PhotonSampler() { }
	virtual ~HaltonPhotonSampler() { }

	virtual void InitSample(Sample *sample) const {
		u_int size = 0;
		for (u_int i = 0; i < sample->n1D.size(); ++i)
			size += sample->n1D[i];
		for (u_int i = 0; i < sample->n2D.size(); ++i)
			size += 2 * sample->n2D[i];
		sample->samplerData = new HaltonPhotonSamplerData(*sample, size);
	}
	virtual void FreeSample(Sample *sample) const {
		delete static_cast<HaltonPhotonSamplerData *>(sample->samplerData);
	}
	virtual bool GetNextSample(Sample *sample) {
		HaltonPhotonSamplerData *data = static_cast<HaltonPhotonSamplerData *>(sample->samplerData);
		data->halton.Sample(data->pathCount++, data->values[0]);

		// Add an offset to the samples to avoid to start with 0.f values
		for (u_int i = 0; i < data->size; ++i) {
			const float v = data->values[0][i] + data->haltonOffset;
			data->values[0][i] = (v >= 1.f) ? (v - 1.f) : v;
		}
		return true;
	}
	virtual float GetOneD(const Sample &sample, u_int num, u_int pos) {
		HaltonPhotonSamplerData *data = static_cast<HaltonPhotonSamplerData *>(sample.samplerData);
		return data->values[num][pos];
	}
	virtual void GetTwoD(const Sample &sample, u_int num, u_int pos,
		float u[2]) {
		HaltonPhotonSamplerData *data = static_cast<HaltonPhotonSamplerData *>(sample.samplerData);
		u[0] = data->values[sample.n1D.size() + num][2 * pos];
		u[1] = data->values[sample.n1D.size() + num][2 * pos + 1];
	}
	virtual float *GetLazyValues(const Sample &sample, u_int num, u_int pos) {
		HaltonPhotonSamplerData *data = static_cast<HaltonPhotonSamplerData *>(sample.samplerData);
		float *result = data->values[sample.n1D.size() + sample.n2D.size() + num];
		for (u_int i = 0; i < sample.dxD[num]; ++i)
			result[i] = sample.rng->floatValue();
		return result;
	}
//	virtual void AddSample(const Sample &sample);
};

//------------------------------------------------------------------------------
// Adaptive Markov Chain Sampler
//------------------------------------------------------------------------------

class AMCMCPhotonSampler : public PhotonSampler {
public:
	AMCMCPhotonSampler(const u_int maxPhotonDepth, const Scene &scene,
			const RandomGenerator *rng);
	~AMCMCPhotonSampler();

	PhotonSamplerType GetType() const { return AMC; }

	void StartNewPhotonPass(const float wavelength) {
		sample->swl.Sample(wavelength);

		// TODO: handle samples stored in Sample class too
		for (u_int j = 0; j < sample->n1D.size(); ++j)
			for (u_int k = 0; k < sample->n1D[j]; ++k)
/*FIXME				sample->oneD[j][k] = rng->floatValue()*/;
	}

	void AcceptCandidate() { swap(currentIndex, candidateIndex); }

	void Uniform() {
		vector<float> &candidate = data[candidateIndex];

		for (size_t i = 0; i < candidate.size(); ++i)
/*FIXME			candidate[i] = rng->floatValue()*/;
	}

	void Mutate(const float mutationSize);

	Sample *GetSample() { return sample; }

	float *GetCurrentData() { return (float *)&data[currentIndex][0]; }
	float *GetCandidateData() { return (float *)&data[candidateIndex][0]; }

private:
	float MutateSingle(const float u, const float mutationSize);

	Sample *sample;

	size_t currentIndex, candidateIndex;
	vector<float> data[2];
};

}//namespace lux

#endif	/* LUX_PHOTON_SAMPLER_H */
