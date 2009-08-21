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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// igi.cpp*
#include "lux.h"
#include "transport.h"
#include "geometry/point.h"
#include "geometry/normal.h"
#include "spectrumwavelengths.h"

namespace lux
{

// IGI Local Structures
struct VirtualLight {
	VirtualLight() { }
	VirtualLight(const TsPack *tspack, const Point &pp, const Normal &nn,
		const SWCSpectrum &le)
		: Le(le), p(pp), n(nn) {
		for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			w[i] = tspack->swl->w[i];
	}
	SWCSpectrum GetSWCSpectrum(const TsPack *tspack) const;
	SWCSpectrum Le;
	float w[WAVELENGTH_SAMPLES];
	Point p;
	Normal n;
};

class IGIIntegrator : public SurfaceIntegrator {
public:
	// IGIIntegrator Public Methods
	IGIIntegrator(int nl, int ns, int d, float md);
	virtual ~IGIIntegrator () {
		delete[] lightSampleOffset;
		delete[] bsdfSampleOffset;
		delete[] bsdfComponentOffset;
	}
	virtual int Li(const TsPack *tspack, const Scene *scene, const Sample *sample) const;
	virtual void RequestSamples(Sample *sample, const Scene *scene);
	virtual void Preprocess(const TsPack *tspack, const Scene *scene);
	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	// IGI Private Data
	u_int nLightPaths, nLightSets;
	vector<vector<VirtualLight> > virtualLights;
	int maxSpecularDepth;
	float minDist2;
	int vlSetOffset, bufferId;

	int *lightSampleOffset, lightNumOffset;
	int *bsdfSampleOffset, *bsdfComponentOffset;
};

}//namespace lux
