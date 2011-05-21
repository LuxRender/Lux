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
	HALTON
};

class PhotonSampler {
public:
	PhotonSampler(const Scene &scene, const RandomGenerator *rng);
	virtual ~PhotonSampler();

	virtual PhotonSamplerType GetType() const = 0;

	virtual Sample *StartNewPhotonPass(const float wavelength) {
		sample->swl.Sample(wavelength);
		return sample;
	}

protected:
	Sample *sample;
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

		return PhotonSampler::StartNewPhotonPass(wavelength);
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
	const RandomGenerator *rng;

	float haltonOffset;
};

}//namespace lux

#endif	/* LUX_PHOTON_SAMPLER_H */
