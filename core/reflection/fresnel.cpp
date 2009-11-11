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

// fresnel.cpp*
#include "fresnel.h"
#include "spectrum.h"

using namespace lux;

namespace lux
{

// Utility Functions
void FrDiel(float cosi, float cost,
	const SWCSpectrum &etai, const SWCSpectrum &etat, SWCSpectrum *const f)
{
	FrDiel2(cosi, SWCSpectrum(cost), etat / etai, f);
}
void FrDiel2(float cosi, const SWCSpectrum &cost, const SWCSpectrum &eta, SWCSpectrum *f)
{
	SWCSpectrum Rparl(eta * cosi);
	Rparl = (cost - Rparl) / (cost + Rparl);
	SWCSpectrum Rperp(eta * cost);
	Rperp = (SWCSpectrum(cosi) - Rperp) / (SWCSpectrum(cosi) + Rperp);
	*f = (Rparl * Rparl + Rperp * Rperp) * .5f;
}
void FrCond(float cosi,
			const SWCSpectrum &eta,
			const SWCSpectrum &k,
			SWCSpectrum *const f) {
	SWCSpectrum tmp = (eta*eta + k*k) * (cosi*cosi) + 1;
	SWCSpectrum Rparl2 = 
		(tmp - (2.f * eta * cosi)) /
		(tmp + (2.f * eta * cosi));
	SWCSpectrum tmp_f = eta*eta + k*k + (cosi*cosi);
	SWCSpectrum Rperp2 =
		(tmp_f - (2.f * eta * cosi)) /
		(tmp_f + (2.f * eta * cosi));
	*f = (Rparl2 + Rperp2) * 0.5f;
}
void FrFull(float cosi, const SWCSpectrum &cost, const SWCSpectrum &eta, const SWCSpectrum &k, SWCSpectrum *f)
{
	SWCSpectrum tmp = (eta * eta + k * k) * (cosi * cosi) + (cost * cost);
	SWCSpectrum Rparl2 = (tmp - (2.f * cosi * cost) * eta) /
		(tmp + (2.f * cosi * cost) * eta);
	SWCSpectrum tmp_f = (eta * eta + k * k) * (cost * cost) + (cosi * cosi);
	SWCSpectrum Rperp2 = (tmp_f - (2.f * cosi * cost) * eta) /
		(tmp_f + (2.f * cosi * cost) * eta);
	*f = (Rparl2 + Rperp2) * 0.5f;
}
SWCSpectrum FresnelApproxEta(const SWCSpectrum &Fr) {
	SWCSpectrum sqrtReflectance = Fr.Clamp(0.f, .999f).Sqrt();
	return (SWCSpectrum(1.f) + sqrtReflectance) /
		(SWCSpectrum(1.f) - sqrtReflectance);
}
 SWCSpectrum FresnelApproxK(const SWCSpectrum &Fr) {
	SWCSpectrum reflectance = Fr.Clamp(0.f, .999f);
	return 2.f * (reflectance /
		(SWCSpectrum(1.f) - reflectance)).Sqrt();
}

}//namespace lux

