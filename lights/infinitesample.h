/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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
 
// infinitesample.cpp*
#include "lux.h"
#include "light.h"
#include "texture.h"
#include "shape.h"
#include "scene.h"
#include "mipmap.h"

namespace lux
{

// Utility Classes and Functions
struct Distribution1D {
	// Distribution1D Methods
	Distribution1D(float *f, int n) {
		func = new float[n];
		cdf = new float[n+1];
		count = n;
		memcpy(func, f, n*sizeof(float));
		ComputeStep1dCDF(func, n, &funcInt, cdf);
		invFuncInt = 1.f / funcInt;
		invCount = 1.f / count;
	}
	float Sample(float u, float *pdf) {
		// Find surrounding cdf segments
		float *ptr = std::lower_bound(cdf, cdf+count+1, u);
		int offset = (int) (ptr-cdf-1);
		// Return offset along current cdf segment
		u = (u - cdf[offset]) / (cdf[offset+1] - cdf[offset]);
		*pdf = func[offset] * invFuncInt;
		return offset + u;
	}
	// Distribution1D Data
	float *func, *cdf;
	float funcInt, invFuncInt, invCount;
	int count;
};
// InfiniteAreaLightIS Definitions
class InfiniteAreaLightIS : public Light {
public:
	// InfiniteAreaLightIS Public Methods
	InfiniteAreaLightIS(const Transform &light2world,	const Spectrum &power, int ns,
			  const string &texmap);
	~InfiniteAreaLightIS();
	SWCSpectrum Power(const Scene *scene) const {
		Point worldCenter;
		float worldRadius;
		scene->WorldBound().BoundingSphere(&worldCenter,
			&worldRadius);
		return Lbase * radianceMap->Lookup(.5f, .5f, .5f) *
			M_PI * worldRadius * worldRadius;
	}
	bool IsDeltaLight() const { return false; }
	SWCSpectrum Le(const RayDifferential &r) const;
	SWCSpectrum Sample_L(const Point &p, float u1, float u2, Vector *wi, float *pdf,
		VisibilityTester *visibility) const;
	SWCSpectrum Sample_L(const Scene *scene, float u1, float u2,
			float u3, float u4, Ray *ray, float *pdf) const;
	float Pdf(const Point &, const Vector &) const;
	SWCSpectrum Sample_L(const Point &P, Vector *w, VisibilityTester *visibility) const;

	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet);

private:
	// InfiniteAreaLightIS Private Data
	Spectrum Lbase;
	MIPMap<Spectrum> *radianceMap;
	Distribution1D *uDistrib, **vDistribs;
};

}

