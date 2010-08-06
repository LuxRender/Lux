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
 
// infinitesample.cpp*
#include "lux.h"
#include "light.h"
#include "scene.h"
#include "mipmap.h"
#include "rgbillum.h"

namespace lux
{
// InfiniteAreaLightIS Definitions
class InfiniteAreaLightIS : public Light {
public:
	// InfiniteAreaLightIS Public Methods
	InfiniteAreaLightIS(const Transform &light2world, const RGBColor &l,
		u_int ns, const string &texmap, EnvironmentMapping *m,
		float gain, float gamma);
	virtual ~InfiniteAreaLightIS();
	virtual float Power(const Scene *scene) const {
		Point worldCenter;
		float worldRadius;
		scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
		return SPDbase.Y() *
			radianceMap->LookupFloat(CHANNEL_WMEAN, .5f, .5f, .5f) *
			M_PI * worldRadius * worldRadius;
	}
	virtual bool IsDeltaLight() const { return false; }
	virtual bool IsEnvironmental() const { return true; }
	virtual bool Le(MemoryArena *arena, const Scene *scene,
		const Sample *sample, const Ray &r, BSDF **bsdf, float *pdf,
		float *pdfDirect, SWCSpectrum *L) const;
	virtual float Pdf(const Point &p, const Point &po,
		const Normal &ns) const;
	virtual bool Sample_L(MemoryArena *arena, const Scene *scene,
		const Sample *sample, float u1, float u2, float u3, BSDF **bsdf,
		float *pdf, SWCSpectrum *Le) const;
	virtual bool Sample_L(MemoryArena *arena, const Scene *scene,
		const Sample *sample, const Point &p, float u1, float u2,
		float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
		SWCSpectrum *Le) const;

	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet);

	MIPMap *radianceMap;
	EnvironmentMapping *mapping;
private:
	// InfiniteAreaLightIS Private Data
	RGBIllumSPD SPDbase;
	Distribution2D *uvDistrib;
};

}

