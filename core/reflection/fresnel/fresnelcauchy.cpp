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

// fresnelcauchy.cpp*
#include "fresnelcauchy.h"
#include "spectrum.h"
#include "spectrumwavelengths.h"

using namespace lux;

void FresnelCauchy::Evaluate(const SpectrumWavelengths &sw, float cosi,
	SWCSpectrum *const f) const
{
	// Compute Fresnel reflectance for dielectric
	if (cb != 0.f && !sw.single) {
		SWCSpectrum eta(sw.w);
		eta *= eta;
		eta = SWCSpectrum(eta_t) + SWCSpectrum(cb) / eta;
		SWCSpectrum cost(max(0.f, 1.f - cosi * cosi));
		if (cosi > 0.f)
			cost /= eta * eta;
		else
			cost *= eta * eta;
		cost = cost.Clamp(0.f, 1.f);
		cost = Sqrt(SWCSpectrum(1.f) - cost);
		FrDiel2(fabsf(cosi), cost, cosi > 0.f ? eta : SWCSpectrum(1.f) / eta, f);
	} else {
		// Compute indices of refraction for dielectric
		bool entering = cosi > 0.f;
		float eta = eta_t;

		// Handle dispersion using cauchy formula
		if (cb != 0.f) { // We are already in single mode
			const float w = sw.w[sw.single_w];
			eta += cb / (w * w);
		}

		// Compute _sint_ using Snell's law
		const float sint2 = (entering ? 1.f / (eta * eta) : eta * eta) *
			max(0.f, 1.f - cosi * cosi);
		// Handle total internal reflection
		if (sint2 >= 1.f)
			*f = SWCSpectrum(1.f);
		else
			FrDiel2(fabsf(cosi),
				SWCSpectrum(sqrtf(max(0.f, 1.f - sint2))),
				entering ? eta : SWCSpectrum(1.f) / eta, f);
	}
}

float FresnelCauchy::Index(const SpectrumWavelengths &sw) const
{
	if (sw.single)
		return (eta_t + cb / (sw.w[sw.single_w] * sw.w[sw.single_w]));
	return eta_t + cb / (WAVELENGTH_END * WAVELENGTH_START);
}

void FresnelCauchy::ComplexEvaluate(const SpectrumWavelengths &sw,
	SWCSpectrum *fr, SWCSpectrum *fi) const
{
	const float *w = sw.w;
	for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i)
		fr->c[i] = eta_t + cb / (w[i] * w[i]);
	// The 4e9*Pi comes from Beers law (4*Pi) and unit conversion of w
	// from nm to m
	*fi = a * SWCSpectrum(w) / (4e9f * M_PI);
}
