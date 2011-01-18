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
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

// RGBColor.cpp*
#include "spectrum.h"
#include "spectrumwavelengths.h"
#include "color.h"
#include "regular.h"
#include "memory.h"

using namespace lux;

Scalar SWCSpectrum::Y(const SpectrumWavelengths &sw) const {
	Scalar y = 0.f;

	if (sw.single) {
		const u_int j = sw.single_w;
		SpectrumWavelengths::spd_ciey.Sample(1,
			sw.binsXYZ + j, sw.offsetsXYZ + j, &y);
		y *= c[j] * WAVELENGTH_SAMPLES;
	} else {
		SWCSpectrum ciey;
		SpectrumWavelengths::spd_ciey.Sample(WAVELENGTH_SAMPLES,
			sw.binsXYZ, sw.offsetsXYZ, ciey.c);
		for (u_int j = 0; j < WAVELENGTH_SAMPLES; ++j) {
			y += ciey.c[j] * c[j];
		}
	}

	return y;
}

SWCSpectrum::SWCSpectrum(const SpectrumWavelengths &sw, const SPD &s) {
	s.Sample(WAVELENGTH_SAMPLES, sw.w, c);
}

SWCSpectrum::SWCSpectrum(const SpectrumWavelengths &sw, const RGBColor &s) {
	const float r = s.c[0];
	const float g = s.c[1];
	const float b = s.c[2];

	*this = sw.sampled_w;
	if (r <= g && r <= b) {
		*this *= r;

		if (g <= b) {
			*this += sw.sampled_c * (g - r) + sw.sampled_b * (b - g);
		} else {
			*this += sw.sampled_c * (b - r) + sw.sampled_g * (g - b);
		}
	} else if (g <= r && g <= b) {
		*this *= g;

		if (r <= b) {
			*this += sw.sampled_m * (r - g) + sw.sampled_b * (b - r);
		} else {
			*this += sw.sampled_m * (b - g) + sw.sampled_r * (r - b);
		}
	} else {	// blue <= red && blue <= green
		*this *= b;

		if (r <= g) {
			*this += sw.sampled_y * (r - b) + sw.sampled_g * (g - r);
		} else {
			*this += sw.sampled_y * (g - b) + sw.sampled_r * (r - g);
		}
	}
}
