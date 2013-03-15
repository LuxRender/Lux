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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// igi.cpp*
#include "lux.h"
#include "transport.h"
#include "luxrays/core/geometry/point.h"
using luxrays::Point;
#include "luxrays/core/geometry/normal.h"
using luxrays::Normal;
#include "spectrumwavelengths.h"

namespace lux
{

// IGI Local Structures
struct VirtualLight {
	VirtualLight() { }
	VirtualLight(const SpectrumWavelengths &sw, const Point &pp,
		const Normal &nn, const SWCSpectrum &le)
		: Le(le), p(pp), n(nn) {
		for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			w[i] = sw.w[i];
	}
	SWCSpectrum GetSWCSpectrum(const SpectrumWavelengths &sw) const;
	SWCSpectrum Le;
	float w[WAVELENGTH_SAMPLES];
	Point p;
	Normal n;
};

class IGIIntegrator : public SurfaceIntegrator {
public:
	// IGIIntegrator Public Methods
	IGIIntegrator(u_int nl, u_int ns, u_int d, float md);
	virtual ~IGIIntegrator () {
		delete[] lightSampleOffset;
		delete[] bsdfSampleOffset;
		delete[] bsdfComponentOffset;
	}
	virtual u_int Li(const Scene &scene, const Sample &sample) const;
	virtual void RequestSamples(Sampler *sampler, const Scene &scene);
	virtual void Preprocess(const RandomGenerator &rng, const Scene &scene);
	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	// IGI Private Data
	u_int nLightPaths, nLightSets;
	vector<vector<VirtualLight> > virtualLights;
	u_int maxSpecularDepth;
	float gLimit;
	u_int vlSetOffset, bufferId, sampleOffset;

	u_int *lightSampleOffset, *lightSampleNumber;
	u_int *bsdfSampleOffset, *bsdfComponentOffset;
};

}//namespace lux
