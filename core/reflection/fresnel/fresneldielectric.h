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

#ifndef LUX_FRESNELDIELECTRIC_H
#define LUX_FRESNELDIELECTRIC_H
// fresneldielectric.h*
#include "lux.h"
#include "fresnel.h"

namespace lux
{

class  FresnelDielectric : public Fresnel {
public:
	// FresnelDielectric Public Methods
	virtual SWCSpectrum Evaluate(float cosi) const;
	FresnelDielectric(float ei, float et, float cB = 0.f) {
		eta_i = ei;
		eta_t = et;
		cb = cB;
	}
private:
	// FresnelDielectric Private Data
	float eta_i, eta_t, cb;
};

class FresnelDielectricComplement : public FresnelDielectric {
public:
	SWCSpectrum Evaluate(float cosi) const {
		return SWCSpectrum(1.f) - FresnelDielectric::Evaluate(cosi);
	}
	FresnelDielectricComplement(float ei, float et, float cB = 0.f) :
		FresnelDielectric(ei, et, cB) {}
};

}//namespace lux

#endif // LUX_FRESNELDIELECTRIC_H

