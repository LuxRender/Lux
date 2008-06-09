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

// infinite.cpp*
#include "lux.h"
#include "light.h"
#include "texture.h"
#include "shape.h"
#include "scene.h"
#include "mipmap.h"

namespace lux
{

// InfiniteAreaLight Declarations
class InfiniteAreaLight : public Light {
public:
	// InfiniteAreaLight Public Methods
	InfiniteAreaLight(const Transform &light2world,	int ns, const string &texmap,
		float gain, float gamma);
	~InfiniteAreaLight();
	SWCSpectrum Power(const Scene *scene) const {
		Point worldCenter;
		float worldRadius;
		scene->WorldBound().BoundingSphere(&worldCenter,
		                                    &worldRadius);
		// NOTE - lordcrc - Bugfix, pbrt tracker id 0000081: crash in infinite light
		Spectrum L = 1.f;
		if (radianceMap != NULL)
			L *= radianceMap->Lookup(.5f, .5f, .5f);

		return SWCSpectrum(SPDbase) * SWCSpectrum(L) * (M_PI * worldRadius * worldRadius);
	}
	bool IsDeltaLight() const { return false; }
	SWCSpectrum Le(const RayDifferential &r) const;
	SWCSpectrum Sample_L(const Point &p, const Normal &n,
		float u1, float u2, float u3, Vector *wi, float *pdf,
		VisibilityTester *visibility) const;
	SWCSpectrum Sample_L(const Point &p, float u1, float u2, float u3,
		Vector *wi, float *pdf, VisibilityTester *visibility) const;
	SWCSpectrum Sample_L(const Scene *scene, float u1, float u2,
			float u3, float u4, Ray *ray, float *pdf) const;
	float Pdf(const Point &, const Normal &, const Vector &) const;
	float Pdf(const Point &, const Vector &) const;
	SWCSpectrum Sample_L(const Point &P, Vector *w, VisibilityTester *visibility) const;
	
	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet);
private:
	// InfiniteAreaLight Private Data
	SPD *SPDbase;
	MIPMap<Spectrum> *radianceMap;
};

}//namespace lux


