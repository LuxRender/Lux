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

#ifndef LUX_FRESNELCONDUCTOR_H
#define LUX_FRESNELCONDUCTOR_H
// fresnelconductor.h*
#include "lux.h"
#include "fresnel.h"
#include "luxrays/core/color/spectrumwavelengths.h"

namespace lux
{

class  FresnelConductor : public Fresnel {
public:
	// FresnelConductor Public Methods
	FresnelConductor(const SWCSpectrum &e, const SWCSpectrum &kk)
		: eta(e), k(kk) {
	}
	virtual ~FresnelConductor() { }
	virtual void Evaluate(const SpectrumWavelengths &sw, float cosi,
		SWCSpectrum *const f) const;
	virtual float Index(const SpectrumWavelengths &sw) const {
		return eta.Filter(sw);
	}
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw) const {
		// The 4e-9*Pi comes from Beer law (4*Pi) and unit conversion
		// of w from nm to m
		return k / SWCSpectrum(sw.w) * (4e-9f * M_PI);
	}
	virtual void ComplexEvaluate(const SpectrumWavelengths &sw,
		SWCSpectrum *fr, SWCSpectrum *fi) const {
		*fr = eta;
		*fi = k;
	}
private:
	// FresnelConductor Private Data
	SWCSpectrum eta, k;
};

}//namespace lux

#endif // LUX_FRESNELCONDUCTOR_H

