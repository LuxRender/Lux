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

// spot.cpp*
#include "lux.h"
#include "light.h"
#include "texture.h"

namespace lux
{

// SpotLight Declarations
class SpotLight : public Light {
public:
	// SpotLight Public Methods
	SpotLight(const Transform &light2world, const boost::shared_ptr< Texture<SWCSpectrum> > L, 
		float gain, float width, float fall);
	virtual ~SpotLight();
	virtual bool IsDeltaLight() const { return true; }
	virtual bool IsEnvironmental() const { return false; }
	float Falloff(const Vector &w) const;
	virtual SWCSpectrum Power(const TsPack *tspack, const Scene *) const {
		return Lbase->Evaluate(tspack, dummydg) * gain * 2.f * M_PI *
			(1.f - .5f * (cosFalloffStart + cosTotalWidth));
	}
	virtual SWCSpectrum Sample_L(const TsPack *tspack, const Point &P, float u1, float u2, float u3,
			Vector *wo, float *pdf, VisibilityTester *visibility) const;
	virtual SWCSpectrum Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2,
			float u3, float u4, Ray *ray, float *pdf) const;
	virtual float Pdf(const TsPack *tspack, const Point &, const Vector &) const;
	virtual float Pdf(const TsPack *tspack, const Point &p, const Normal &n,
		const Point &po, const Normal &ns) const;
	virtual bool Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const;
	virtual bool Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
		VisibilityTester *visibility, SWCSpectrum *Le) const;
	virtual SWCSpectrum Le(const TsPack *tspack, const Scene *scene, const Ray &r,
		const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const;
	
	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet, const TextureParams &tp);
private:
	// SpotLight Private Data
	float cosTotalWidth, cosFalloffStart;
	Point lightPos;
	boost::shared_ptr< Texture<SWCSpectrum> > Lbase;
	DifferentialGeometry dummydg;
	float gain;
};

}//namespace lux

