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

#ifndef LUX_FRESNEL_H
#define LUX_FRESNEL_H
// fresnel.h*
#include "lux.h"
#include "color.h"
#include "spectrum.h"

namespace lux
{

class  Fresnel {
public:
	// Fresnel Interface
	virtual ~Fresnel();
	virtual SWCSpectrum Evaluate(const TsPack *tspack, float cosi) const = 0;
};

  SWCSpectrum FrDiel(float cosi, float cost,
	const SWCSpectrum &etai, const SWCSpectrum &etat);
  SWCSpectrum FrCond(float cosi, const SWCSpectrum &n, const SWCSpectrum &k);
  SWCSpectrum FresnelApproxEta(const SWCSpectrum &intensity);
  SWCSpectrum FresnelApproxK(const SWCSpectrum &intensity);

}//namespace lux

#endif // LUX_FRESNEL_H

