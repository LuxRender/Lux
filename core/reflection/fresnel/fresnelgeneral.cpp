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

// fresnelgeneral.cpp*
#include "fresnelgeneral.h"
#include "spectrum.h"

using namespace lux;

void FresnelGeneral::Evaluate(const SpectrumWavelengths &sw, float cosi,
	SWCSpectrum *const f) const
{
	if (model == CONDUCTOR_FRESNEL) {
		if (cosi > 0.f)
			FrCond(cosi, eta, k, f);
		else
			*f = SWCSpectrum(0.f);
		return;
	}
	SWCSpectrum sint2(max(0.f, 1.f - cosi * cosi));
	if (cosi > 0.f)
		sint2 /= eta * eta;
	else
		sint2 *= eta * eta;
	sint2 = sint2.Clamp(0.f, 1.f);
	const SWCSpectrum cost2 = (SWCSpectrum(1.f) - sint2);
	if (cosi > 0.f) {
		if (model == DIELECTRIC_FRESNEL)
			FrDiel2(cosi, cost2.Sqrt(), eta, f);
		else {
			const SWCSpectrum a(2.f * k * k * sint2);
			FrFull(cosi, (cost2 + (cost2 * cost2 + a * a).Sqrt()).Sqrt(), eta, k, f);
		}
	} else {
		if (model == DIELECTRIC_FRESNEL)
			FrDiel2(-cosi, cost2.Sqrt(), SWCSpectrum(1.f) / eta, f);
		else {
			const SWCSpectrum a(2.f * k * k * sint2);
			const SWCSpectrum d2 = eta * eta + k * k;
			FrFull(-cosi, (cost2 + (cost2 * cost2 + a * a).Sqrt()).Sqrt(), eta / d2, -k / d2, f);
		}
	}
}
