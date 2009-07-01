/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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

#ifndef LUX_BLACKBODYSPD_H
#define LUX_BLACKBODYSPD_H
// blackbody.h*
#include "lux.h"
#include "color.h"
#include "spectrum.h"
#include "spd.h"

#define BB_CACHE_START   380. // precomputed cache starts at wavelength,
#define BB_CACHE_END     720. // and ends at wavelength
#define BB_CACHE_SAMPLES 256  // total number of cache samples 

namespace lux
{

  class BlackbodySPD : public SPD {
  public:
    BlackbodySPD() : SPD() {
		init(6500.f); // default to D65 temperature
    }

    BlackbodySPD(float t) : SPD() {
		init(t);
    }

    virtual ~BlackbodySPD() {
		FreeSamples();
	}

	void init(float t);

  protected:
    float temp;

  };

}//namespace lux

#endif // LUX_BLACKBODYSPD_H

