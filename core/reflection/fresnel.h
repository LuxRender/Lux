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

#ifndef LUX_FRESNEL_H
#define LUX_FRESNEL_H
// fresnel.h*
#include "lux.h"
#include "spectrum.h"

namespace lux
{

class  Fresnel {
public:
	// Fresnel Interface
	virtual ~Fresnel() { }
	virtual void Evaluate(const TsPack *tspack, float cosi, SWCSpectrum *const f) const = 0;
	virtual float Index(const TsPack *tspack) const = 0;
	virtual SWCSpectrum SigmaA(const TsPack *tspack) const {
		return SWCSpectrum(0.f);
	}
	virtual void ComplexEvaluate(const TsPack *tspack,
		SWCSpectrum *fr, SWCSpectrum *fi) const = 0;
};

// The ConcreteFresnel class allows an arbitrary Fresnel derivative to be
// returned by a function
// The destructor doesn't free the referenced class since it can be allocated
// in a memory arena
class ConcreteFresnel : public Fresnel {
public:
	ConcreteFresnel(const Fresnel *fr) : fresnel(fr) { }
	virtual ~ConcreteFresnel() { }
	virtual void Evaluate(const TsPack *tspack, float cosi,
		SWCSpectrum *const f) const {
		fresnel->Evaluate(tspack, cosi, f);
	}
	virtual float Index(const TsPack *tspack) const {
		return fresnel->Index(tspack);
	}
	virtual SWCSpectrum SigmaA(const TsPack *tspack) const {
		return fresnel->SigmaA(tspack);
	}
	virtual void ComplexEvaluate(const TsPack *tspack,
		SWCSpectrum *fr, SWCSpectrum *fi) const {
		fresnel->ComplexEvaluate(tspack, fr, fi);
	}
private:
	const Fresnel *fresnel;
};

void FrDiel(float cosi, float cost,
	const SWCSpectrum &etai, const SWCSpectrum &etat, SWCSpectrum *const f);
void FrDiel2(float cosi, const SWCSpectrum &cost, const SWCSpectrum &eta, SWCSpectrum *f);
void FrCond(float cosi, const SWCSpectrum &n, const SWCSpectrum &k, SWCSpectrum *const f);
void FrFull(float cosi, const SWCSpectrum &cost, const SWCSpectrum &n, const SWCSpectrum &k, SWCSpectrum *const f);
SWCSpectrum FresnelApproxEta(const SWCSpectrum &intensity);
SWCSpectrum FresnelApproxK(const SWCSpectrum &intensity);

}//namespace lux

#endif // LUX_FRESNEL_H

