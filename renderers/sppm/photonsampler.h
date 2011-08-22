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
#include "hitpoints.h"

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
	PhotonSampler(SPPMRenderer *sppmr):
		Sampler(0, 0, 0, 0, 0), renderer(sppmr) { }
	virtual ~PhotonSampler() { }
	virtual u_int GetTotalSamplePos() { return 0; }
	virtual u_int RoundSize(u_int size) const { return size; }
	virtual void InitSample(Sample *sample) const = 0;
	virtual void FreeSample(Sample *sample) const = 0;
	virtual bool GetNextSample(Sample *sample) = 0;
	virtual float GetOneD(const Sample &sample, u_int num, u_int pos) = 0;
	virtual void GetTwoD(const Sample &sample, u_int num, u_int pos,
		float u[2]) = 0;
	virtual float *GetLazyValues(const Sample &sample, u_int num, u_int pos) = 0;


	void AddFluxToHitPoint(const Sample *sample, const u_int lightGroup, HitPoint * const hp, const XYZColor flux);

	// TODO: remove the arguments to get a coherent Sample;:AddSample(const Sample &sample) API
	virtual void AddSample(const Sample *sample, const u_int lightGroup, HitPoint * const hp, const XYZColor flux)
	{
		AddFluxToHitPoint(sample, lightGroup, hp, flux);
	}

	virtual void TracePhotons(
		Sample *sample,
		Distribution1D *lightCDF
		);

	void TracePhoton(
		Sample *sample,
		Distribution1D *lightCDF
		);

	void IncPhoton() const;
	bool ContinueTracing() const;

private:
	SPPMRenderer *renderer;
};

//------------------------------------------------------------------------------
// Halton Photon Sampler
//------------------------------------------------------------------------------

class HaltonPhotonSampler : public PhotonSampler {
public:
	class HaltonPhotonSamplerData {
	public:
		HaltonPhotonSamplerData(const Sample &sample, u_int sz) :
			halton(sz, *(sample.rng)), size(sz),
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
			u_int offset = 0;
			for (u_int i = 0; i < sample.n1D.size(); ++i) {
				values[offset + i] = buffer;
				buffer += sample.n1D[i];
			}
			offset += sample.n1D.size();
			for (u_int i = 0; i < sample.n2D.size(); ++i) {
				values[offset + i] = buffer;
				buffer += 2 * sample.n2D[i];
			}
			offset += sample.n2D.size();
			for (u_int i = 0; i < sample.nxD.size(); ++i) {
				values[offset + i] = buffer;
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
	HaltonPhotonSampler(SPPMRenderer *renderer) : PhotonSampler(renderer) { }
	virtual ~HaltonPhotonSampler() { }

	virtual void InitSample(Sample *sample) const {
		sample->sampler = const_cast<HaltonPhotonSampler *>(this);
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
};

//------------------------------------------------------------------------------
// Uniform Sampler
//------------------------------------------------------------------------------

class UniformPhotonSampler : public PhotonSampler {
public:
	class UniformPhotonSamplerData {
	public:
		UniformPhotonSamplerData(const Sample &sample) {
			if (sample.n1D.size() + sample.n2D.size() +
				sample.nxD.size() == 0) {
				values = NULL;
				return;
			}
			values = new float *[sample.n1D.size() +
				sample.n2D.size() + sample.nxD.size()];

			n = 0;

			for (u_int i = 0; i < sample.n1D.size(); ++i)
				n += sample.n1D[i];
			for (u_int i = 0; i < sample.n2D.size(); ++i)
				n += 2 * sample.n2D[i];
			for (u_int i = 0; i < sample.nxD.size(); ++i)
				n += sample.dxD[i] * sample.nxD[i];
			if (n == 0) {
				values[0] = NULL;
				return;
			}

			float *buffer = new float[n];
			u_int offset = 0;
			for (u_int i = 0; i < sample.n1D.size(); ++i) {
				values[offset + i] = buffer;
				buffer += sample.n1D[i];
			}
			offset += sample.n1D.size();
			for (u_int i = 0; i < sample.n2D.size(); ++i) {
				values[offset + i] = buffer;
				buffer += 2 * sample.n2D[i];
			}
			offset += sample.n2D.size();
			for (u_int i = 0; i < sample.nxD.size(); ++i) {
				values[offset + i] = buffer;
				buffer += sample.dxD[i];
			}
		}

		void UniformSample(Sample &sample)
		{
			for(u_int i = 0; i < n; ++i)
				values[0][i] = sample.rng->floatValue();
		}

		~UniformPhotonSamplerData() {
			delete[] values[0];
			delete[] values;
		}
		float **values;
		u_int n;
	};
	UniformPhotonSampler(SPPMRenderer *renderer) : PhotonSampler(renderer) {}
	virtual ~UniformPhotonSampler() { }

	virtual void InitSample(Sample *sample) const {
		sample->sampler = const_cast<UniformPhotonSampler *>(this);
		sample->samplerData = new UniformPhotonSamplerData(*sample);
	}
	virtual void FreeSample(Sample *sample) const {
		delete static_cast<UniformPhotonSamplerData *>(sample->samplerData);
	}
	virtual bool GetNextSample(Sample *sample) {
		UniformPhotonSamplerData *data = static_cast<UniformPhotonSamplerData *>(sample->samplerData);
		data->UniformSample(*sample);

		/*
		 * TODO: Meaning of that ?
		// Add an offset to the samples to avoid to start with 0.f values
		for (u_int i = 0; i < data->size; ++i) {
			const float v = data->values[0][i] + data->haltonOffset;
			data->values[0][i] = (v >= 1.f) ? (v - 1.f) : v;
		}
		*/
		return true;
	}
	virtual float GetOneD(const Sample &sample, u_int num, u_int pos) {
		UniformPhotonSamplerData *data = static_cast<UniformPhotonSamplerData *>(sample.samplerData);
		return data->values[num][pos];
	}
	virtual void GetTwoD(const Sample &sample, u_int num, u_int pos,
		float u[2]) {
		UniformPhotonSamplerData *data = static_cast<UniformPhotonSamplerData *>(sample.samplerData);
		u[0] = data->values[sample.n1D.size() + num][2 * pos];
		u[1] = data->values[sample.n1D.size() + num][2 * pos + 1];
	}
	virtual float *GetLazyValues(const Sample &sample, u_int num, u_int pos) {
		UniformPhotonSamplerData *data = static_cast<UniformPhotonSamplerData *>(sample.samplerData);
		return &data->values[sample.n1D.size() + sample.n2D.size() + num][pos * sample.dxD[num]];
	}
};

//------------------------------------------------------------------------------
// Adaptive Markov Chain Sampler
//------------------------------------------------------------------------------
//
// TODO: try to remove the Uniform sampler and use the Halton as uniform sampler for AMCMC

class AMCMCPhotonSampler : public UniformPhotonSampler
{
	public:
		class AMCMCPhotonSamplerData : public UniformPhotonSamplerData {
			public:
				AMCMCPhotonSamplerData(Sample &sample): UniformPhotonSamplerData(sample) {}

				void Mutate(const RandomGenerator * const rng, AMCMCPhotonSamplerData &source, const float mutationSize) const;

				static float MutateSingle(const RandomGenerator *rng, const float u, const float mutationSize);
		};

		struct SplatNode {
			SplatNode(const u_int lg, const XYZColor f, HitPoint * const hp) {
				lightGroup = lg;
				flux = f;
				hitPoint = hp;
			}

			u_int lightGroup;
			XYZColor flux;
			HitPoint *hitPoint;
		};

		struct AMCMCPath : public std::vector <SplatNode> {
			AMCMCPhotonSamplerData* data;

			bool isVisible() const
			{
				return size() > 0;
			}

			// TODO: experiment and add a splatCount like Dade previously did
			void Splat(const Sample *sample, PhotonSampler *sampler) const
			{
				for (AMCMCPath::const_iterator iter = begin(); iter != end(); ++iter)
					sampler->AddFluxToHitPoint(sample, iter->lightGroup, iter->hitPoint, iter->flux);
			}
		};

		AMCMCPhotonSampler(SPPMRenderer *renderer) : UniformPhotonSampler(renderer)
		{
			mutationSize = 1.f;
			accepted = 1;
			mutated = 0;
			uniformCount = 1;
		}
		virtual ~AMCMCPhotonSampler() { }

		virtual void InitSample(Sample *sample) const {
			sample->sampler = const_cast<AMCMCPhotonSampler *>(this);

			for(u_int i = 0; i < 2; ++i)
				paths[i].data = new AMCMCPhotonSamplerData(*sample);

			pathCandidate = paths + 1;
			pathCurrent = paths;
		}
		virtual void FreeSample(Sample *sample) const {
			for(u_int i = 0; i < 2; ++i)
				delete paths[i].data;
		}

		void GetNextSample(Sample *sample, bool uniform)
		{
			sample->samplerData = pathCandidate->data;
			pathCandidate->clear();

			if(uniform)
				pathCandidate->data->UniformSample(*sample);
			else
				pathCandidate->data->Mutate(sample->rng, *pathCurrent->data, mutationSize);
		}

		void swap()
		{
			std::swap(pathCurrent, pathCandidate);
		}

		virtual void AddSample(const Sample *sample, const u_int lightGroup, HitPoint * const hp, const XYZColor flux)
		{
			pathCandidate->push_back(SplatNode(lightGroup, flux, hp));
		}

	virtual void TracePhotons(
		Sample *sample,
		Distribution1D *lightCDF
		);

	private:
		// AMC data
		float mutationSize;
		u_int accepted;
		u_int mutated;

		mutable AMCMCPath *pathCurrent, *pathCandidate;
		mutable AMCMCPath paths[2];

	public:
		u_int uniformCount;
};

}//namespace lux

#endif	/* LUX_PHOTON_SAMPLER_H */
