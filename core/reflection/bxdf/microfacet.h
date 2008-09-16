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

#ifndef LUX_MICROFACET_H
#define LUX_MICROFACET_H
// microfacet.h*
#include "lux.h"
#include "bxdf.h"
#include "color.h"
#include "spectrum.h"

namespace lux
{

class  Microfacet : public BxDF {
public:
	// Microfacet Public Methods
	Microfacet(const SWCSpectrum &reflectance, Fresnel *f,
		MicrofacetDistribution *d);
	void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const;
	float G(const Vector &wo, const Vector &wi,
			const Vector &wh) const {
		float NdotWh = fabsf(CosTheta(wh));
		float NdotWo = fabsf(CosTheta(wo));
		float NdotWi = fabsf(CosTheta(wi));
		float WOdotWh = AbsDot(wo, wh);
		return min(1.f, min((2.f * NdotWh * NdotWo / WOdotWh),
		                (2.f * NdotWh * NdotWi / WOdotWh)));
	}
	bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi,
		float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack = NULL,
		bool reverse = false) const;
	float Pdf(const TsPack *tspack, const Vector &wo, const Vector &wi) const;
private:
	// Microfacet Private Data
	SWCSpectrum R;
	MicrofacetDistribution *distribution;
	Fresnel *fresnel;
};


}//namespace lux

#endif // LUX_MICROFACET_H

