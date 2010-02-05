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
	FresnelSlick (const SWCSpectrum &r, const SWCSpectrum &k_) :
		normalIncidence(r), k(k_) { }
	virtual ~FresnelSlick() { }
	virtual void Evaluate(const TsPack *tspack, float cosi,
		SWCSpectrum *const f) const;
	virtual float Index(const TsPack *tspack) const {
		return ((SWCSpectrum(1.f) - normalIncidence.Sqrt()) /
			(SWCSpectrum(1.f) + normalIncidence.Sqrt())).Filter(tspack);
	}
	virtual SWCSpectrum SigmaA(const TsPack *tspack) const {
		return k / SWCSpectrum(tspack->swl->w) * (4.f * M_PI);
	}
	virtual void ComplexEvaluate(const TsPack *tspack,
		SWCSpectrum *fr, SWCSpectrum *fi) const {
		*fr = Index(tspack);
		*fi = k;
	}
private:
  SWCSpectrum normalIncidence, k;
};

}//namespace lux

#endif // LUX_FRESNELSLICK_H

