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

// fresneldielectric.cpp*
#include "fresneldielectric.h"
#include "luxrays/core/color/swcspectrum.h"
#include "luxrays/core/color/spectrumwavelengths.h"

using namespace lux;

void FresnelDielectric::Evaluate(const SpectrumWavelengths &sw, float cosi,
	SWCSpectrum *const f) const
{
	// Compute Fresnel reflectance for dielectric
	SWCSpectrum cost(max(0.f, 1.f - cosi * cosi));
	if (cosi > 0.f)
		cost /= eta_t * eta_t;
	else
		cost *= eta_t * eta_t;
	cost = cost.Clamp(0.f, 1.f);
	cost = Sqrt(SWCSpectrum(1.f) - cost);
	FrDiel2(fabsf(cosi), cost, cosi > 0.f ? eta_t : SWCSpectrum(1.f) / eta_t, f);
}

float FresnelDielectric::Index(const SpectrumWavelengths &sw) const
{
	if (sw.single)
		return eta_t.c[sw.single_w];
	else
		return index;
}

void FresnelDielectric::ComplexEvaluate(const SpectrumWavelengths &sw,
	SWCSpectrum *fr, SWCSpectrum *fi) const
{
	*fr = eta_t;
	// The 4e9*Pi comes from beers law (4*Pi) and unit conversion of w
	// from nm to m
	*fi = a * SWCSpectrum(sw.w) / (4e9f * M_PI);
}
