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

XYZColor SWCSpectrum::ToXYZ(const TsPack *tspack) const {
	SpectrumWavelengths *sw = tspack->swl;
	float xyz[3];
	xyz[0] = xyz[1] = xyz[2] = 0.;
	if (sw->single) {
		const int j = sw->single_w;
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

Scalar SWCSpectrum::y(const TsPack *tspack) const {
	SpectrumWavelengths *sw = tspack->swl;
	Scalar y = 0.f;

	if (sw->single) {
		const int j = sw->single_w;
		y = sw->cie_Y[j] * c[j];
	} else {
		for (unsigned int j = 0; j < WAVELENGTH_SAMPLES; ++j) {
			y += sw->cie_Y[j] * c[j];
		}
		y *= inv_WAVELENGTH_SAMPLES;
	}

	return y;
}
Scalar SWCSpectrum::filter(const TsPack *tspack) const
{
	SpectrumWavelengths *sw = tspack->swl;
	Scalar result = 0.f;
	if (sw->single) {
		result = c[sw->single_w];
	} else {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			result += c[i];
		result *= inv_WAVELENGTH_SAMPLES;
	}
	return result;
}

SWCSpectrum::SWCSpectrum(const TsPack *tspack, const SPD *s) {
	SpectrumWavelengths *sw = tspack->swl;
	for (unsigned int j = 0; j < WAVELENGTH_SAMPLES; ++j) {
		c[j] = s->sample(sw->w[j]);
	}
}

SWCSpectrum::SWCSpectrum(const TsPack *tspack, Spectrum s) {
	SpectrumWavelengths *sw = tspack->swl;
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
