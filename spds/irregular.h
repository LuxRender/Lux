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

#ifndef LUX_IRREGULARSPD_H
#define LUX_IRREGULARSPD_H
// irregular.h*
#include "lux.h"
#include "color.h"
#include "spectrum.h"
#include "regular.h"

namespace lux
{
  // only use spline for regular data
  enum SPDResamplingMethod { Linear, Spline };

  // Irregularly sampled SPD
  // Resampled to a fixed resolution (at construction)
  // using cubic spline interpolation
  class IrregularSPD : public SPD {
  public:

    IrregularSPD() : SPD() {}

    // creates an irregularly sampled SPD
    // may "truncate" the edges to fit the new resolution
    //  wavelengths   array containing the wavelength of each sample
    //  samples       array of sample values at the given wavelengths
    //  n             number of samples
    //  resolution    resampling resolution (in nm)
    IrregularSPD(const float* const wavelengths, const float* const samples,
			int n, float resolution = 5, SPDResamplingMethod resamplignMethod = Linear);

    virtual ~IrregularSPD() {}

  protected:
	  void init(float lMin, float lMax, const float* const s, int n);

  private:
    // computes data for natural spline interpolation
    // from Numerical Recipes in C
    void calc_spline_data(const float* const wavelengths,
			const float* const amplitudes, int n, float *spline_data);
  };

}//namespace lux

#endif // LUX_IRREGULARSPD_H
