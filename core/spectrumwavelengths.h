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
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

// Force correct declaration order of SpectrumWavelengths and SWCSpectrum
#ifndef LUX_SWCSPECTRUM_H
#include "spectrum.h"
#endif

#ifndef LUX_SPECTRUMWAVELENGTHS_H
#define LUX_SPECTRUMWAVELENGTHS_H
// Spectrumwavelengths.h*
#include "lux.h"
#include "regular.h"

namespace lux
{

class	SpectrumWavelengths {
public:

	// SpectrumWavelengths Public Methods
	SpectrumWavelengths() : single_w(0), single(false) { }
	~SpectrumWavelengths() { }

	inline void Sample(float u1) {
		single = false;
		u1 *= WAVELENGTH_SAMPLES;
		single_w = luxrays::Floor2UInt(u1);
		u1 -= single_w;

		// Sample new stratified wavelengths and precompute RGB/XYZ data
		const float offset = float(WAVELENGTH_END - WAVELENGTH_START) * inv_WAVELENGTH_SAMPLES;
		float waveln = WAVELENGTH_START + u1 * offset;
		for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i) {
			w[i] = waveln;
			waveln += offset;
		}
		spd_w.Offsets(WAVELENGTH_SAMPLES, w, binsRGB, offsetsRGB);
		spd_ciex.Offsets(WAVELENGTH_SAMPLES, w, binsXYZ, offsetsXYZ);
	}

	inline float SampleSingle() const {
		single = true;
		return w[single_w];
	}

	float w[WAVELENGTH_SAMPLES]; // Wavelengths in nm

	u_int  single_w; // Chosen single wavelength bin
	mutable bool single; // Split to single

	int binsRGB[WAVELENGTH_SAMPLES], binsXYZ[WAVELENGTH_SAMPLES];
	float offsetsRGB[WAVELENGTH_SAMPLES], offsetsXYZ[WAVELENGTH_SAMPLES];

	static const RegularSPD spd_w, spd_c, spd_m, spd_y,
		spd_r, spd_g, spd_b, spd_ciex, spd_ciey, spd_ciez;
};


}//namespace lux

#endif // LUX_SPECTRUMWAVELENGTHS_H
