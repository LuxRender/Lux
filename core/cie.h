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

#ifndef LUX_CIE_H
#define LUX_CIE_H
// cie.h*
#include "spectrum.h"
#include "spd.h"
#include <assert.h>

namespace lux
{
  // Normalized SPD of D illuminant
  class DIlluminant : public SPD {
  public:
    
    DIlluminant();

    // creates a D illuminant with color temperature T (4000-25000 Kelvin)
    DIlluminant(float T) : SPD() {
      temp = T;

      // compute color temperature to chromaticity
      float xD, yD;

      assert(T >= 4000 && T <= 25000);
      
      float T2 = T*T;
      float T3 = T2*T;

      if (T <= 7000) 
        xD = -4.6070e9 / T3 + 2.9678e6 / T2 + 0.09911e3 / T + 0.244063;
      else 
        xD = -2.0054e9 / T3 + 1.9018e6 / T2 + 0.24748e3 / T + 0.237040;

      yD = -3.0*xD*xD + 2.870*xD - 0.275;

      // calculate SPD constants from chromaticity coordinates
      float M = 0.0241 + 0.2562*xD - 0.7341*yD;

      float M1 = (-1.3515 -  1.7703*xD +  5.9114*yD) / M;
      float M2 = ( 0.0300 - 31.4424*xD + 30.0717*yD) / M;

      // precompute SPD
     
      float *spd = new float[nFS];

      for (int i = 0; i < nFS; i++) {
        int lambda = FSstart + i * (FSend - FSstart) / (nFS-1);
        
        spd[i] = S0[i] + M1*S1[i] + M2*S2[i];
      }

      SD = RegularSPD(spd, FSstart, FSend, nFS);

      delete[] spd;

      // sample SPD at 560nm for normalization
      invSD_560 = 1.f;  
      double t;
      sample(560, t);
      invSD_560 = 1.f / t;
    }

    ~DIlluminant() {
    }

    void sample(const float lambda, double& p) const {
      SD.sample(lambda, p);
      p *= invSD_560;
    }
    
  private:

    float temp;
    RegularSPD SD;
    double invSD_560;

    static const int FSstart = 300;
    static const int FSend = 830;
    static const int nFS = (FSend-FSstart) / 10 + 1;
    static const float S0[nFS];
    static const float S1[nFS];
    static const float S2[nFS];
  };

}//namespace lux

#endif // LUX_CIE_H
