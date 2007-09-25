/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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

// spot.cpp*
#include "lux.h"
#include "light.h"
#include "shape.h"
// SpotLight Declarations
class SpotLight : public Light {
public:
	// SpotLight Public Methods
	SpotLight(const Transform &light2world, const Spectrum &, float width, float fall);
	Spectrum Sample_L(const Point &p, Vector *wi, VisibilityTester *vis) const;
	bool IsDeltaLight() const { return true; }
	float Falloff(const Vector &w) const;
	Spectrum Power(const Scene *) const {
		return Intensity * 2.f * M_PI *
			(1.f - .5f * (cosFalloffStart + cosTotalWidth));
	}
	Spectrum Sample_L(const Point &P, float u1, float u2,
			Vector *wo, float *pdf, VisibilityTester *visibility) const;
	Spectrum Sample_L(const Scene *scene, float u1, float u2,
			float u3, float u4, Ray *ray, float *pdf) const;
	float Pdf(const Point &, const Vector &) const;
	
	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet);
private:
	// SpotLight Private Data
	float cosTotalWidth, cosFalloffStart;
	Point lightPos;
	Spectrum Intensity;
};
