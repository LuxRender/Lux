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

#ifndef LUX_ANISOTROPIC_H
#define LUX_ANISOTROPIC_H
// anisotropic.h*
#include "lux.h"
#include "microfacetdistribution.h"
#include "luxrays/core/geometry/vector.h"
using luxrays::Vector;

namespace lux
{

class  Anisotropic : public MicrofacetDistribution {
public:
	// Anisotropic Public Methods
	Anisotropic(float x, float y) { ex = x; ey = y;
		if (ex > 100000.f || isnan(ex)) ex = 100000.f;
		if (ey > 100000.f || isnan(ey)) ey = 100000.f;
	}
	virtual ~Anisotropic() { }
	virtual float D(const Vector &wh) const {
		const float cosTheta = fabsf(wh.z);
		const float e = (ex * wh.x * wh.x + ey * wh.y * wh.y) /
			max(0.f, 1.f - cosTheta * cosTheta);
		return sqrtf((ex + 2.f) * (ey + 2.f)) * INV_TWOPI *
			powf(cosTheta, e);
	}
	virtual void SampleH(float u1, float u2, Vector *wh, float *d,
		float *pdf) const;
	virtual float Pdf(const Vector &wh) const;
	virtual float G(const Vector &wo, const Vector &wi, const Vector &wh) const {
		const float NdotWh = fabsf(CosTheta(wh));
		const float NdotWo = fabsf(CosTheta(wo));
		const float NdotWi = fabsf(CosTheta(wi));
		const float WOdotWh = AbsDot(wo, wh);
		const float WIdotWh = AbsDot(wi, wh);
		return min(1.f, min((2.f * NdotWh * NdotWo / WOdotWh),
		                (2.f * NdotWh * NdotWi / WIdotWh)));
	}
private:
	void SampleFirstQuadrant(float u1, float u2, float *phi, float *cosTheta) const;
	float ex, ey;
};

}//namespace lux

#endif // LUX_ANISOTROPIC_H

