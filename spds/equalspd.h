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

#ifndef LUX_EQUALSPD_H
#define LUX_EQUALSPD_H
// equalspd.h*
#include "lux.h"
#include "color.h"
#include "spectrum.h"
#include "spd.h"

// Equal energy SPD

#define EQ_CACHE_START   380. // precomputed cache starts at wavelength,
#define EQ_CACHE_END     720. // and ends at wavelength
#define EQ_CACHE_SAMPLES 2  // total number of cache samples 

namespace lux
{

  class EqualSPD : public SPD {
  public:
    EqualSPD() : SPD() {
		init(1.f);
    }

    EqualSPD(float p) : SPD() {
		init(p);
    }

    virtual ~EqualSPD() {
	}

	void init(float p);

  protected:
    float power;

  };

}//namespace lux

#endif // LUX_EQUALSPD_H

