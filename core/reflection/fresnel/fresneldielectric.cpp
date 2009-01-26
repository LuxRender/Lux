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

// fresneldielectric.cpp*
#include "fresneldielectric.h"
#include "spectrum.h"
#include "spectrumwavelengths.h"

using namespace lux;

void FresnelDielectric::Evaluate(const TsPack *tspack, float cosi, SWCSpectrum *const f) const {
	if (cb != 0.f && !tspack->swl->single) {
		SWCSpectrum eta = SWCSpectrum(tspack->swl->w);
		eta *= eta;
		eta = SWCSpectrum(eta_t) + SWCSpectrum(cb * 1000000) / eta;
		eta /= SWCSpectrum(eta_i);
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
		float ei = eta_i, et = eta_t;

		if(cb != 0.) {
			// Handle dispersion using cauchy formula
			float w = tspack->swl->SampleSingle();
			et = eta_t + (cb * 1000000) / (w*w);
		}

		if (!entering)
			swap(ei, et);
		// Compute _sint_ using Snell's law
		const float sint = ei/et * sqrtf(max(0.f, 1.f - cosi*cosi));
		if (sint > 1.) {
			// Handle total internal reflection
			*f = SWCSpectrum(1.);
		} else {
			float cost = sqrtf(max(0.f, 1.f - sint*sint));
			FrDiel(fabsf(cosi), cost, ei, et, f);
		}
	}
}

