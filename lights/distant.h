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

// distant.cpp*
#include "lux.h"
#include "light.h"
#include "scene.h"
#include "texture.h"

namespace lux
{

// DistantLight Declarations
class DistantLight : public Light {
public:
	// DistantLight Public Methods
	DistantLight(const Transform &light2world, 
		const boost::shared_ptr<Texture<SWCSpectrum> > &L, float gain, 
		float theta, const Vector &dir);
	virtual ~DistantLight();
	virtual bool IsDeltaLight() const { return false; }
	virtual bool IsEnvironmental() const { return true; }
	virtual float Power(const Scene &scene) const {
		Point worldCenter;
		float worldRadius;
		scene.WorldBound().BoundingSphere(&worldCenter, &worldRadius);
		return Lbase->Y() * gain * M_PI * worldRadius * worldRadius;
	}
	virtual bool Le(const Scene &scene, const Sample &sample, const Ray &r,
		BSDF **bsdf, float *pdf, float *pdfDirect,
		SWCSpectrum *L) const;
	virtual float Pdf(const Point &p, const PartialDifferentialGeometry &dg) const;

	virtual bool SampleL(const Scene &scene, const Sample &sample,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf,
		SWCSpectrum *Le) const;
	virtual bool SampleL(const Scene &scene, const Sample &sample,
		const Point &p, float u1, float u2, float u3, BSDF **bsdf,
		float *pdf, float *pdfDirect, SWCSpectrum *Le) const;
	
	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet);
private:
	// DistantLight Private Data
	Vector x, y, lightDir;
	boost::shared_ptr<Texture<SWCSpectrum> > Lbase;
	float gain, sin2ThetaMax, cosThetaMax;
	BxDF *bxdf;
};

}//namespace lux
