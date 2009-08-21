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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_GAUSSIANSPD_H
#define LUX_GAUSSIANSPD_H
// gaussian.h*
#include "lux.h"
#include "color.h"
#include "spectrum.h"
#include "spd.h"

// Peak + fallof SPD using gaussian distribution

#define GAUSS_CACHE_START   380. // precomputed cache starts at wavelength,
#define GAUSS_CACHE_END     720. // and ends at wavelength
#define GAUSS_CACHE_SAMPLES 512  // total number of cache samples 

namespace lux
{

  class GaussianSPD : public SPD {
  public:
    GaussianSPD() : SPD() {
		init(550.0f, 50.0f, 1.f);
    }

    GaussianSPD(float mean, float width, float refl) : SPD() {
		init(mean, width, refl);
    }

    virtual ~GaussianSPD() {
	}

	void init(float mean, float width, float refl);

  protected:
    float mu, wd, r0;

  };

}//namespace lux

#endif // LUX_GAUSSIANSPD_H

