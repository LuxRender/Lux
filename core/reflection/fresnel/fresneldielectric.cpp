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

// fresneldielectric.cpp*
#include "fresneldielectric.h"
#include "spectrum.h"
#include "spectrumwavelengths.h"

using namespace lux;

void FresnelDielectric::Evaluate(const TsPack *tspack, float cosi, SWCSpectrum *const f) const {
	if (cb != 0.f && !tspack->swl->single) {
		SWCSpectrum eta = SWCSpectrum(tspack->swl->w);
		eta *= eta;
		eta = SWCSpectrum(eta_t) + SWCSpectrum(cb) / eta;
		SWCSpectrum cost(sqrtf(max(0.f, 1.f - cosi * cosi)));
		if (cosi > 0.f)
			cost /= eta;
		else
			cost *= eta;
		cost.Clamp(0.f, 1.f);
		cost = (SWCSpectrum(1.f) - cost * cost).Sqrt();
		FrDiel2(fabsf(cosi), cost, eta, f);
	} else {
		// Compute Fresnel reflectance for dielectric
		cosi = Clamp(cosi, -1.f, 1.f);
		// Compute indices of refraction for dielectric
		bool entering = cosi > 0.;
		float et = eta_t;

		// Handle dispersion using cauchy formula
		if(cb != 0.f) { // We are already in single mode
			const float w = tspack->swl->w[tspack->swl->single_w];
			et += cb / (w * w);
		}

		// Compute _sint_ using Snell's law
		const float sint = (entering ? 1.f / et : et) *
			sqrtf(max(0.f, 1.f - cosi * cosi));
		// Handle total internal reflection
		if (sint > 1.f)
			*f = SWCSpectrum(1.f);
		else
			FrDiel(fabsf(cosi), sqrtf(max(0.f, 1.f - sint * sint)),
				1.f, et, f);
	}
}

float FresnelDielectric::Index(const TsPack *tspack) const
{
	const float *w = tspack->swl->w;
	if (tspack->swl->single)
		return (eta_t + cb / (w[tspack->swl->single_w] * w[tspack->swl->single_w]));
	const float i[4] = {eta_t + cb / (w[0] * w[0]),
		eta_t + cb / (w[1] * w[1]),
		eta_t + cb / (w[2] * w[2]),
		eta_t + cb / (w[3] * w[3])};
	return SWCSpectrum(i).Filter(tspack);
}
