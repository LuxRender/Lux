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
	// Compute Fresnel reflectance for dielectric
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
		// Compute indices of refraction for dielectric
		bool entering = cosi > 0.f;
		float eta = eta_t;

		// Handle dispersion using cauchy formula
		if(cb != 0.f) { // We are already in single mode
			const float w = tspack->swl->w[tspack->swl->single_w];
			eta += cb / (w * w);
		}

		// Compute _sint_ using Snell's law
		const float sint = (entering ? 1.f / eta : eta) *
			sqrtf(max(0.f, 1.f - cosi * cosi));
		// Handle total internal reflection
		if (sint >= 1.f)
			*f = SWCSpectrum(1.f);
		else
			FrDiel2(fabsf(cosi),
				SWCSpectrum(sqrtf(max(0.f, 1.f - sint * sint))),
				SWCSpectrum(eta), f);
	}
}

float FresnelDielectric::Index(const TsPack *tspack) const
{
	const SpectrumWavelengths *swl = tspack->swl;
	if (swl->single)
		return (eta_t + cb / (swl->w[swl->single_w] * swl->w[swl->single_w]));
	return eta_t + cb / (WAVELENGTH_END * WAVELENGTH_START);
}

void FresnelDielectric::ComplexEvaluate(const TsPack *tspack,
	SWCSpectrum *fr, SWCSpectrum *fi) const
{
	const float *w = tspack->swl->w;
	for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i)
		fr->c[i] = eta_t + cb / (w[i] * w[i]);
	*fi = SWCSpectrum(0.f);
}
