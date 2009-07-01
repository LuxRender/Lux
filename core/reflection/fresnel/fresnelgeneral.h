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

#ifndef LUX_FRESNELGENERAL_H
#define LUX_FRESNELGENERAL_H
// fresnelgeneral.h*
#include "lux.h"
#include "fresnel.h"

namespace lux
{

class  FresnelGeneral : public Fresnel {
public:
	// FresnelGeneral Public Methods
	FresnelGeneral(const SWCSpectrum &e, const SWCSpectrum &kk)
		: eta(e), k(kk) {
	}
	FresnelGeneral(const SWCSpectrum &ei, const SWCSpectrum &ki, const SWCSpectrum &et, const SWCSpectrum &kt) {
		SWCSpectrum norm(ei * ei + ki * ki);
		eta = (ei * et + ki * kt) / norm;
		k = (ei * kt - et * ki) / norm;
	}
	virtual ~FresnelGeneral() { }
	virtual void Evaluate(const TsPack *tspack, float cosi, SWCSpectrum *const f) const;
private:
	// FresnelGeneral Private Data
	SWCSpectrum eta, k;
};

}//namespace lux

#endif // LUX_FRESNELCONDUCTOR_H

