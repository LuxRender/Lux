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

// infinite.cpp*
#include "lux.h"
#include "light.h"
#include "texture.h"
#include "shape.h"
#include "scene.h"
#include "mipmap.h"
#include "rgbillum.h"

namespace lux
{

// InfiniteAreaLight Declarations
class InfiniteAreaLight : public Light {
public:
	// InfiniteAreaLight Public Methods
	InfiniteAreaLight(const Transform &light2world, const RGBColor &l,
		u_int ns, const string &texmap, EnvironmentMapping *m,
		float gain, float gamma);
	virtual ~InfiniteAreaLight();
	virtual float Power(const Scene *scene) const;
	virtual bool IsDeltaLight() const { return false; }
	virtual bool IsEnvironmental() const { return true; }
	virtual SWCSpectrum Le(const TsPack *tspack,
		const RayDifferential &r) const;
	virtual SWCSpectrum Le(const TsPack *tspack, const Scene *scene,
		const Ray &r, const Normal &n, BSDF **bsdf, float *pdf,
		float *pdfDirect) const;
	virtual SWCSpectrum Sample_L(const TsPack *tspack, const Point &p,
		const Normal &n, float u1, float u2, float u3, Vector *wi,
		float *pdf, VisibilityTester *visibility) const;
	virtual SWCSpectrum Sample_L(const TsPack *tspack, const Point &p,
		float u1, float u2, float u3, Vector *wi, float *pdf,
		VisibilityTester *visibility) const;
	virtual SWCSpectrum Sample_L(const TsPack *tspack, const Scene *scene,
		float u1, float u2, float u3, float u4, Ray *ray,
		float *pdf) const;
	virtual float Pdf(const TsPack *, const Point &, const Normal &,
		const Vector &) const;
	virtual float Pdf(const TsPack *, const Point &, const Vector &) const;
	virtual float Pdf(const TsPack *tspack, const Point &p, const Normal &n,
		const Point &po, const Normal &ns) const;
	virtual bool Sample_L(const TsPack *tspack, const Scene *scene,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf,
		SWCSpectrum *Le) const;
	virtual bool Sample_L(const TsPack *tspack, const Scene *scene,
		const Point &p, const Normal &n, float u1, float u2, float u3,
		BSDF **bsdf, float *pdf, float *pdfDirect,
		VisibilityTester *visibility, SWCSpectrum *Le) const;
	
	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet, const TextureParams &tp);

	MIPMap<RGBColor> *radianceMap;
	EnvironmentMapping *mapping;
private:
	// InfiniteAreaLight Private Data
	RGBIllumSPD SPDbase;
};

}//namespace lux


