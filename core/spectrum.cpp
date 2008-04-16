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

// spectrum.cpp*
#include "spectrum.h"
#include "spectrumwavelengths.h"
#include "regular.h"
#include "memory.h"

#include <boost/thread/tss.hpp>

using namespace lux;

// Spectrum Method Definitions
ostream &operator<<(ostream &os, const Spectrum &s) {
	for (int i = 0; i < COLOR_SAMPLES; ++i) {
		os << s.c[i];
		if (i != COLOR_SAMPLES-1)
			os << ", ";
	}
	return os;
}
float Spectrum::XWeight[COLOR_SAMPLES] = {
	0.412453f, 0.357580f, 0.180423f
};
float Spectrum::YWeight[COLOR_SAMPLES] = {
	0.212671f, 0.715160f, 0.072169f
};
float Spectrum::ZWeight[COLOR_SAMPLES] = {
	0.019334f, 0.119193f, 0.950227f
};

Spectrum lux::FromXYZ(float x, float y, float z) {
	float c[3];
	c[0] =  3.240479f * x + -1.537150f * y + -0.498535f * z;
	c[1] = -0.969256f * x +  1.875991f * y +  0.041556f * z;
	c[2] =  0.055648f * x + -0.204043f * y +  1.057311f * z;
	return Spectrum(c);
}

// thread specific wavelengths
boost::thread_specific_ptr<SpectrumWavelengths> thread_wavelengths;

XYZColor SWCSpectrum::ToXYZ() const {
	SpectrumWavelengths *sw = thread_wavelengths.get();
	float xyz[3];
	xyz[0] = xyz[1] = xyz[2] = 0.;
	if (sw->single) {
		int j = sw->single_w;
		xyz[0] = sw->cie_X[j] * c[j];
		xyz[1] = sw->cie_Y[j] * c[j];
		xyz[2] = sw->cie_Z[j] * c[j];
	} else {
		for (unsigned int j = 0; j < WAVELENGTH_SAMPLES; ++j) {
			xyz[0] += sw->cie_X[j] * c[j];
			xyz[1] += sw->cie_Y[j] * c[j];
			xyz[2] += sw->cie_Z[j] * c[j];
		}
		xyz[0] *= inv_WAVELENGTH_SAMPLES;
		xyz[1] *= inv_WAVELENGTH_SAMPLES;
		xyz[2] *= inv_WAVELENGTH_SAMPLES;
	} 

	return XYZColor(xyz);
}

double SWCSpectrum::y() const {
	SpectrumWavelengths *sw = thread_wavelengths.get();
	double y = 0.;

	if (sw->single) {
		int j = sw->single_w;
		y = sw->cie_Y[j] * c[j];
	} else {
		for (unsigned int j = 0; j < WAVELENGTH_SAMPLES; ++j) {
			y += sw->cie_Y[j] * c[j];
		}
		y *= inv_WAVELENGTH_SAMPLES;
	}

	return y;
}

SWCSpectrum::SWCSpectrum(const SPD *s) {
	SpectrumWavelengths *sw = thread_wavelengths.get();
	for (unsigned int j = 0; j < WAVELENGTH_SAMPLES; ++j) {
		c[j] = (double) s->sample(sw->w[j]);
	}
}

SWCSpectrum::SWCSpectrum(Spectrum s) {
	SpectrumWavelengths *sw = thread_wavelengths.get();
	const float r = s.c[0];
	const float g = s.c[1];
	const float b = s.c[2];

	for (unsigned int j = 0; j < WAVELENGTH_SAMPLES; ++j)
		c[j] = 0.;

	if (r <= g && r <= b)
	{
		AddWeighted(r, sw->spect_w);

		if (g <= b)
		{
			AddWeighted(g - r, sw->spect_c);
			AddWeighted(b - g, sw->spect_b);
		}
		else
		{
			AddWeighted(b - r, sw->spect_c);
			AddWeighted(g - b, sw->spect_g);
		}
	}
	else if (g <= r && g <= b)
	{
		AddWeighted(g, sw->spect_w);

		if (r <= b)
		{
			AddWeighted(r - g, sw->spect_m);
			AddWeighted(b - r, sw->spect_b);
		}
		else
		{
			AddWeighted(b - g, sw->spect_m);
			AddWeighted(r - b, sw->spect_r);
		}
	}
	else // blue <= red && blue <= green
	{
		AddWeighted(b, sw->spect_w);

		if (r <= g)
		{
			AddWeighted(r - b, sw->spect_y);
			AddWeighted(g - r, sw->spect_g);
		}
		else
		{
			AddWeighted(g - b, sw->spect_y);
			AddWeighted(r - g, sw->spect_r);
		}
	}
}




