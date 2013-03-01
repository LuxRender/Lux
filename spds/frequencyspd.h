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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_FREQUENCYSPD_H
#define LUX_FREQUENCYSPD_H
// frequencyspd.h*
#include "lux.h"
#include "spd.h"

// Sin frequency/phase distribution

#define FREQ_CACHE_START   380.f // precomputed cache starts at wavelength,
#define FREQ_CACHE_END     720.f // and ends at wavelength
#define FREQ_CACHE_SAMPLES 2048  // total number of cache samples 

namespace lux
{

  class FrequencySPD : public SPD {
  public:
    FrequencySPD() : SPD() {
		init(0.03f, 0.5f, 1.f);
    }

    FrequencySPD(float freq, float phase, float refl) : SPD() {
		init(freq, phase, refl);
    }

    virtual ~FrequencySPD() {
	}

	void init(float freq, float phase, float refl);

  protected:
    float fq, ph, r0;

  };

}//namespace lux

#endif // LUX_FREQUENCYSPD_H

