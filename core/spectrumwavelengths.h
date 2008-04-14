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

#ifndef LUX_SPECTRUMWAVELENGTHS_H
#define LUX_SPECTRUMWAVELENGTHS_H
// spectrumwavelengths.h*
#include "lux.h"
#include "spd.h"
#include "spectrum.h"

#include "data/xyzbasis.h"

namespace lux
{

class	SpectrumWavelengths {
public:

	// SpectrumWavelengths Public Methods
	SpectrumWavelengths();

	inline void Sample(float u1, float u2) {
		single = false;
		single_w = Floor2Int(u2 * WAVELENGTH_SAMPLES);

		// Sample new stratified wavelengths and precompute RGB/XYZ data
		const float offset = float(WAVELENGTH_END - WAVELENGTH_START) * inv_WAVELENGTH_SAMPLES;
		float waveln = WAVELENGTH_START + u1 * offset;
		for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i) {
			w[i] = waveln;
			waveln += offset;
		}			
		
		for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i) {
			// Interpolate RGB Conversion SPDs
			spect_w.c[i] = (double) spd_w->sample(w[i]);
			spect_c.c[i] = (double) spd_c->sample(w[i]);
			spect_m.c[i] = (double) spd_m->sample(w[i]);
			spect_y.c[i] = (double) spd_y->sample(w[i]);
			spect_r.c[i] = (double) spd_r->sample(w[i]);
			spect_g.c[i] = (double) spd_g->sample(w[i]);
			spect_b.c[i] = (double) spd_b->sample(w[i]);
		}

		for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i) {
			// Interpolate XYZ Conversion weights
			const float w0 = w[i] - CIEstart;
			int i0 = Floor2Int(w0);
			const float b0 = w0 - float(i0), a0 = 1.0f - b0;
			cie_X[i] = (CIE_X[i0] * a0 + CIE_X[i0+1] * b0);
			cie_Y[i] = (CIE_Y[i0] * a0 + CIE_Y[i0+1] * b0);
			cie_Z[i] = (CIE_Z[i0] * a0 + CIE_Z[i0+1] * b0);
		}
	}

	inline float SampleSingle() {
		single = true;
		return w[single_w];
	}

	float w[WAVELENGTH_SAMPLES]; // Wavelengths in nm

	bool single; // Split to single
	int  single_w; // Chosen single wavelength bin

	SWCSpectrum spect_w, spect_c, spect_m;	// white, cyan, magenta
	SWCSpectrum spect_y, spect_r, spect_g;	// yellow, red, green
	SWCSpectrum spect_b;	// blue

	float *cie_X, *cie_Y, *cie_Z; // CIE XYZ weights

private:
	SPD *spd_w, *spd_c, *spd_m, *spd_y,
		*spd_r, *spd_g, *spd_b;
};


}//namespace lux

#endif // LUX_SPECTRUMWAVELENGTHS_H