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
#include "bxdf.h"

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
		const float costhetah = fabsf(CosTheta(wh));
		const float e = (ex * wh.x * wh.x + ey * wh.y * wh.y) /
			(1.f - costhetah * costhetah);
		return sqrtf((ex + 2.f) * (ey + 2.f)) * INV_TWOPI * powf(costhetah, e);
	}
	virtual void Sample_f(const Vector &wo, Vector *wi, float u1, float u2, float *pdf) const;
	virtual float Pdf(const Vector &wo, const Vector &wi) const;
private:
	void sampleFirstQuadrant(float u1, float u2, float *phi, float *costheta) const;
	float ex, ey;
};

}//namespace lux

#endif // LUX_ANISOTROPIC_H

