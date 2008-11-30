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

// point.cpp*
#include "lux.h"
#include "light.h"

namespace lux
{

// PointLight Classes
class PointLight : public Light {
public:
	// PointLight Public Methods
	PointLight(const Transform &light2world, const RGBColor &le, float gain);
	~PointLight();
	SWCSpectrum Power(const TsPack *tspack, const Scene *) const {
		return SWCSpectrum(tspack, LSPD) * 4.f * M_PI;
	}
	bool IsDeltaLight() const { return true; }
	SWCSpectrum Sample_L(const TsPack *tspack, const Point &P, float u1, float u2, float u3,
			Vector *wo, float *pdf, VisibilityTester *visibility) const;
	SWCSpectrum Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2,
			float u3, float u4, Ray *ray, float *pdf) const;
	float Pdf(const Point &, const Vector &) const;
	bool Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const;
	bool Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
		VisibilityTester *visibility, SWCSpectrum *Le) const;
	SWCSpectrum Le(const TsPack *tspack, const Scene *scene, const Ray &r,
		const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const;
	
	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet);
private:
	// PointLight Private Data
	Point lightPos;
	SPD *LSPD;
};

}//namespace lux

