/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

#ifndef LUX_SCHLICKDISTRIBUTION_H
#define LUX_SCHLICKDISTRIBUTION_H
// schlickdistribution.h*
#include "lux.h"
#include "microfacetdistribution.h"
#include "luxrays/core/geometry/vector.h"
using luxrays::Vector;

namespace lux
{

class  SchlickDistribution : public MicrofacetDistribution {
public:
	SchlickDistribution(float r, float a) : roughness(r), anisotropy(a) { }
	virtual ~SchlickDistribution() { }
	// SchlickDistribution Public Methods
	virtual void SampleH(float u1, float u2, Vector *wh, float *d,
		float *pdf) const;
	virtual float D(const Vector &wh) const {
		const float cosTheta = fabsf(wh.z);
		return SchlickZ(cosTheta) * SchlickA(wh) * INV_PI;
	}
	virtual float Pdf(const Vector &wh) const { return D(wh); }
	virtual float G(const Vector &wo, const Vector &wi, const Vector &wh) const {
		return SchlickG(fabsf(wo.z)) * SchlickG(fabsf(wi.z));
	}
private:
	float SchlickG(float costheta) const {
		return costheta / (costheta * (1.f - roughness) + roughness);
	}
	float SchlickZ(float cosNH) const {
		if (roughness == 0.f)
			return INFINITY;

		const float cosNH2 = cosNH * cosNH;
		// expanded for increased numerical stability
		const float d = (cosNH2 * roughness + (1.f - cosNH2));
		// use double division to avoid overflow in product
		// return roughness / (d * d);
		return (roughness / d) / d;
	}
	float SchlickA(const Vector &H) const {
		const float h = sqrtf(H.x * H.x + H.y * H.y);
		if (h > 0.f) {
			const float w = (anisotropy > 0.f ? H.x : H.y) / h;
			const float p = 1.f - fabsf(anisotropy);
			return sqrtf(p / (p * p + w * w * (1.f - p * p)));
		}
		return 1.f;
	}
	float roughness, anisotropy;
};

}//namespace lux

#endif // LUX_SCHLICKDISTRIBUTION_H

