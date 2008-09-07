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

// goniometric.cpp*
#include "lux.h"
#include "light.h"
#include "shape.h"
#include "scene.h"
#include "mipmap.h"

namespace lux
{

// GonioPhotometricLight Declarations
class GonioPhotometricLight : public Light {
public:
	// GonioPhotometricLight Public Methods
	GonioPhotometricLight(const Transform &light2world, const RGBColor &, const
	string &texname);
	~GonioPhotometricLight() { delete mipmap; }
	bool IsDeltaLight() const { return true; }
	RGBColor Scale(const Vector &w) const {
		Vector wp = Normalize(WorldToLight(w));
		swap(wp.y, wp.z);
		float theta = SphericalTheta(wp);
		float phi   = SphericalPhi(wp);
		float s = phi * INV_TWOPI, t = theta * INV_PI;
		return mipmap ? mipmap->Lookup(s, t) : 1.f;
	}
	SWCSpectrum Power(const TsPack *tspack, const Scene *) const {
		return SWCSpectrum(tspack, 4.f * M_PI * Intensity *
			mipmap->Lookup(.5f, .5f, .5f));
	}
	SWCSpectrum Sample_L(const TsPack *tspack, const Point &P, float u1, float u2, float u3,
		Vector *wo, float *pdf, VisibilityTester *visibility) const;
	SWCSpectrum Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2,
			float u3, float u4, Ray *ray, float *pdf) const;
	float Pdf(const Point &, const Vector &) const;
	
	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet);
private:
	// GonioPhotometricLight Private Data
	Point lightPos;
	RGBColor Intensity;
	MIPMap<RGBColor> *mipmap;

};

}//namespace lux

