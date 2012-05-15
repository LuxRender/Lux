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

// irregular.cpp*
#include "irregular.h"

using namespace lux;

// creates an irregularly sampled SPD
// may "truncate" the edges to fit the new resolution
//  wavelengths   array containing the wavelength of each sample
//  samples       array of sample values at the given wavelengths
//  n             number of samples
//  resolution    resampling resolution (in nm)
IrregularSPD::IrregularSPD(const float* const wavelengths, const float* const samples,
	u_int n, float resolution, SPDResamplingMethod resamplingMethod) 
	: SPD()
{
	float lambdaMin = wavelengths[0];
	float lambdaMax = wavelengths[n - 1];

	u_int sn = Ceil2UInt((lambdaMax - lambdaMin) / resolution) + 1;

	vector<float> sam(sn);

	if (resamplingMethod == Linear) {
		u_int k = 0;
		for (u_int i = 0; i < sn; i++) {
			float lambda = lambdaMin + i * resolution;

			if (lambda < wavelengths[0] || lambda > wavelengths[n-1]) {
				sam[i] = 0.f;
				continue;
			}

			for (; k < n; ++k) {
				if (wavelengths[k] >= lambda)
					break;
			}

			if (wavelengths[k] == lambda)
				sam[i] = samples[k];
			else { 
				float intervalWidth = wavelengths[k] - wavelengths[k - 1];
				float u = (lambda - wavelengths[k - 1]) / intervalWidth;
				sam[i] = Lerp(u, samples[k - 1], samples[k]);
			}
		}
	} else {
		vector<float> sd(n);

		calc_spline_data(wavelengths, samples, n, &sd[0]);

		u_int k = 0;
		for (u_int i = 0; i < sn; i++) {
			float lambda = lambdaMin + i * resolution;

			if (lambda < wavelengths[0] || lambda > wavelengths[n-1]) {
				sam[i] = 0.f;
				continue;
			}

			while (lambda > wavelengths[k+1])
				k++;

			float h = wavelengths[k+1] - wavelengths[k];
			float a = (wavelengths[k+1] - lambda) / h;
			float b = (lambda - wavelengths[k]) / h;

			sam[i] = max(a*samples[k] + b*samples[k+1]+
				((a*a*a-a)*sd[k] + (b*b*b-b)*sd[k+1])*(h*h)/6.f, 0.f);
		}
	}

	init(lambdaMin, lambdaMax, &sam[0], sn);
}

void IrregularSPD::init(float lMin, float lMax, const float* const s, u_int n) {
	lambdaMin = lMin;
	lambdaMax = lMax;
	delta = (lambdaMax - lambdaMin) / (n-1);
	invDelta = 1.f / delta;
	nSamples = n;

	AllocateSamples(n);

	// Copy samples
	for (u_int i = 0; i < n; ++i)
		samples[i] = s[i];

}

// computes data for natural spline interpolation
// from Numerical Recipes in C
void IrregularSPD::calc_spline_data(const float* const wavelengths,
	const float* const amplitudes, u_int n, float *spline_data)
{
	vector<float> u(n - 1);

	// natural spline
	spline_data[0] = u[0] = 0.f;

	for (u_int i = 1; i <= n - 2; i++) {
		float sig = (wavelengths[i] - wavelengths[i - 1]) /
			(wavelengths[i + 1] - wavelengths[i - 1]);
		float p = sig * spline_data[i - 1] + 2.f;
		spline_data[i] = (sig - 1.f) / p;
		u[i] = (amplitudes[i + 1] - amplitudes[i]) /
			(wavelengths[i + 1] - wavelengths[i]) - 
			(amplitudes[i] - amplitudes[i - 1]) /
			(wavelengths[i] - wavelengths[i - 1]);
		u[i] = (6.f * u[i] /
			(wavelengths[i + 1] - wavelengths[i - 1]) -
			sig * u[i - 1]) / p;
	}

	float qn, un;
  
	// natural spline
	qn = un = 0.f;
	spline_data[n - 1] = (un - qn * u[n-2]) /
		(qn * spline_data[n - 2] + 1.f);
	for (u_int k = 0; k < n - 1; ++k)
		spline_data[n - k - 1] = spline_data[n - k - 1] *
			spline_data[n - k] + u[k];
}

