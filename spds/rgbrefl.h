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

#ifndef LUX_RGBREFLSPD_H
#define LUX_RGBREFLSPD_H
// rgbrefl.h*
#include "lux.h"
#include "color.h"
#include "spectrum.h"
#include "spd.h"

namespace lux
{

// reflectant SPD, from RGB color, using smits conversion, reconstructed using linear interpolation
  class RGBReflSPD : public SPD {
  public:
    RGBReflSPD() : SPD() {
	  init(RGBColor(1.f));
    }

    RGBReflSPD(RGBColor s) : SPD() {
      init(s);
    }

    ~RGBReflSPD() {}

  protected:
	  void AddWeighted(float w, float *c) {
		  for(int i=0; i<nSamples; i++) {
			samples[i] += c[i] * w;
		  }
	  }

	void init(RGBColor s);

  private:   
  };

}//namespace lux

#endif // LUX_RGBREFLSPD_H
