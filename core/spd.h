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

#ifndef LUX_SPD_H
#define LUX_SPD_H
// spd.h*
#include "lux.h"

namespace lux
{

  // regularly sampled SPD, reconstructed using linear interpolation
  class RegularSPD : public SPD {
  public:
    RegularSPD() : nSamples(0), SPD() {
    }

    // creates a regularly sampled SPD
    //  samples    array of sample values
    //  lambdaMin  wavelength (nm) of first sample
    //  lambdaMax  wavelength (nm) of last sample
    //  n          number of samples
    RegularSPD(const float* const samples, float lambdaMin, float lambdaMax, int n) : SPD() {
      init(lambdaMin, lambdaMax, samples, n);
    }  

    ~RegularSPD() {
    }

    // samples the SPD by performing a linear interpolation on the data
    virtual void sample(const float lambda, double& p) const {

      if (nSamples < 1 || lambda < lambdaMin || lambda > lambdaMax) {
        p = 0.0;
        return;
      }

      // interpolate the two closest samples linearly
      float x = (lambda - lambdaMin) * invDelta;
      int b0 = max((int)x, 0);
      int b1 = min(b0+1, nSamples-1);
      float dx = x - b0;
      p = ((1. - dx) * samples[b0]) + (dx * samples[b1]);
    }

  protected:

    void init(float lMin, float lMax, const float* const s, int n) {
      lambdaMin = lMin;
      lambdaMax = lMax;
      delta = (lambdaMax - lambdaMin) / (n-1);
      invDelta = 1.f / delta;
      nSamples = n;

      samples.reserve(n);
      for (int i = 0; i < n; i++)
        samples.push_back(s[i]);
    }

    int nSamples;
    float lambdaMin;
    float lambdaMax;
    float delta, invDelta;
    vector<float> samples;

  private:   
  };

  // only use spline for regular data
  enum SPDResamplingMethod { Linear, Spline };

  // Irregularly sampled SPD
  // Resampled to a fixed resolution (at construction)
  // using cubic spline interpolation
  class IrregularSPD : public RegularSPD {
  public:

    IrregularSPD() : RegularSPD() {
    }

    // creates an irregularly sampled SPD
    // may "truncate" the edges to fit the new resolution
    //  wavelengths   array containing the wavelength of each sample
    //  samples       array of sample values at the given wavelengths
    //  n             number of samples
    //  resolution    resampling resolution (in nm)
    IrregularSPD(const float* const wavelengths, const float* const samples, int n, int resolution = 5, SPDResamplingMethod resamplignMethod = Linear) 
      : RegularSPD() {

      int lambdaMin = Ceil2Int(wavelengths[0] / resolution) * resolution;
      int lambdaMax = Floor2Int(wavelengths[n-1] / resolution) * resolution;

      int sn = (lambdaMax - lambdaMin) / resolution + 1;

      float *sam = new float[sn];
      float *sd  = NULL;

      if (resamplignMethod == Linear) {
        int k = 0;
        for (int i = 0; i < sn; i++) {

          float lambda = lambdaMin + i * resolution;

          if (lambda < wavelengths[0] || lambda > wavelengths[n-1]) {
            sam[i] = 0.0;
            continue;
          }

		  for (; k < n; ++k)
			  if (wavelengths[k] >= lambda)
				  break;
   
		  if (wavelengths[k] == lambda)
			  sam[i] = samples[k];
          else { 
		    float intervalWidth = wavelengths[k] - wavelengths[k - 1];
		    float u = (lambda - wavelengths[k - 1]) / intervalWidth;
		    sam[i] = ((1. - u) * samples[k - 1]) + (u * samples[k]);
          }
        }
      }
      else {
        sd  = new float[sn];

        calc_spline_data(wavelengths, samples, n, sd);

        int k = 0;
        for (int i = 0; i < sn; i++) {

          float lambda = lambdaMin + i * resolution;

          if (lambda < wavelengths[0] || lambda > wavelengths[n-1]) {
            sam[i] = 0.0;
            continue;
          }

          while (lambda > wavelengths[k+1])
            k++;

          float h = wavelengths[k+1] - wavelengths[k];
          float a = (wavelengths[k+1] - lambda) / h;
          float b = (lambda - wavelengths[k]) / h;

          sam[i] = max(a*samples[k] + b*samples[k+1]+
            ((a*a*a-a)*sd[k] + (b*b*b-b)*sd[k+1])*(h*h)/6.0, 0);
        }
      }

      init(lambdaMin, lambdaMax, sam, sn);

      delete[] sam;
      delete[] sd;
    }

    ~IrregularSPD() {
    }

  private:

    // computes data for natural spline interpolation
    // from Numerical Recipes in C
    void calc_spline_data(const float* const wavelengths, const float* const amplitudes, int n, float *spline_data) {
      
      float *u = new float[n-1];

      // natural spline
      spline_data[0] = u[0] = 0.f;

      for (int i = 1; i <= n-2; i++) {
        float sig = (wavelengths[i] - wavelengths[i-1]) / (wavelengths[i+1] - wavelengths[i-1]);
        float p = sig * spline_data[i-1] + 2.f;
        spline_data[i] = (sig-1.0)/p;
        u[i] = (amplitudes[i+1] - amplitudes[i]) / (wavelengths[i+1] - wavelengths[i]) - 
          (amplitudes[i] - amplitudes[i-1]) / (wavelengths[i] - wavelengths[i-1]);
        u[i] = (6.0*u[i] / (wavelengths[i+1] - wavelengths[i-1]) - sig*u[i-1]) / p;
      }

      float qn, un;
      
      // natural spline
      qn = un = 0.0;
      spline_data[n-1] = (un - qn * u[n-2]) / (qn * spline_data[n-2] + 1.0);
      for (int k = n-2; k >= 0; k--) {
        spline_data[k] = spline_data[k] * spline_data[k+1] + u[k];
      }

      delete[] u;
    }
  };

}//namespace lux

#endif // LUX_SPD_H
