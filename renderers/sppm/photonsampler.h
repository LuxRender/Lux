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

class PhotonSampler {
public:
	PhotonSampler(const RandomGenerator *rng);
	virtual ~PhotonSampler() { }

	virtual PhotonSamplerType GetType() const = 0;

protected:
	const RandomGenerator *rng;
};

//------------------------------------------------------------------------------
// Halton Photon Sampler
//------------------------------------------------------------------------------

class HaltonPhotonSampler : public PhotonSampler {
public:
	HaltonPhotonSampler(const Scene &scene, const RandomGenerator *rng);
	~HaltonPhotonSampler();

	PhotonSamplerType GetType() const { return HALTON; }

	Sample *StartNewPhotonPass(const float wavelength) {
		haltonOffset = rng->floatValue();
		sample->swl.Sample(wavelength);

		return sample;
	}

	void StartNewPhotonPath() {
		// This may be required by the volume integrator
		for (u_int j = 0; j < sample->n1D.size(); ++j)
			for (u_int k = 0; k < sample->n1D[j]; ++k)
				sample->oneD[j][k] = rng->floatValue();
	}

	void GetLightData(const u_int pathCount, float *u) {
		PermutedHalton halton(7, *rng);
		halton.Sample(pathCount, u);

		// Add an offset to the samples to avoid to start with 0.f values
		for (int j = 0; j < 7; ++j) {
			float v = u[j] + haltonOffset;
			u[j] = (v >= 1.f) ? (v - 1.f) : v;
		}
	}

	void GetPathVertexData(float *u1, float *u2, float *u3) {
		*u1 = rng->floatValue();
		*u2 = rng->floatValue();
		*u3 = rng->floatValue();
	}

	float GetPathVertexRRData() {
		return rng->floatValue();
	}

private:
	Sample *sample;

	float haltonOffset;
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
				sample->oneD[j][k] = rng->floatValue();
	}

	void AcceptCandidate() { swap(currentIndex, candidateIndex); }

	void Uniform() {
		vector<float> &candidate = data[candidateIndex];

		for (size_t i = 0; i < candidate.size(); ++i)
			candidate[i] = rng->floatValue();
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
