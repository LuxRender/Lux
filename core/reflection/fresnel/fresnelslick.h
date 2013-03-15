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

#ifndef LUX_FRESNELSLICK_H
#define LUX_FRESNELSLICK_H
// fresnelslick.h*
#include "lux.h"
#include "fresnel.h"
#include "spectrumwavelengths.h"

namespace lux
{

class  FresnelSlick : public Fresnel {
public:
	FresnelSlick (const SWCSpectrum &r, const SWCSpectrum &a_) :
		normalIncidence(r), a(a_) { }
	virtual ~FresnelSlick() { }
	virtual void Evaluate(const SpectrumWavelengths &sw, float cosi,
		SWCSpectrum *const f) const;
	virtual float Index(const SpectrumWavelengths &sw) const {
		return ((SWCSpectrum(1.f) - Sqrt(normalIncidence)) /
			(SWCSpectrum(1.f) + Sqrt(normalIncidence))).Filter(sw);
	}
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw) const {
		return a;
	}
	virtual void ComplexEvaluate(const SpectrumWavelengths &sw,
		SWCSpectrum *fr, SWCSpectrum *fi) const {
		*fr = Index(sw);
		// The 4e9*Pi comes from Beer law (4*Pi) and unit conversion
		// of w from nm to m
		*fi = a * SWCSpectrum(sw.w) / (4e9f * M_PI);
	}
private:
	SWCSpectrum normalIncidence, a;
};

}//namespace lux

#endif // LUX_FRESNELSLICK_H

