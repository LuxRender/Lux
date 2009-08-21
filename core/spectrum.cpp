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
#include "color.h"
#include "spectrum.h"
#include "spectrumwavelengths.h"
#include "regular.h"
#include "memory.h"

using namespace lux;

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

SWCSpectrum::SWCSpectrum(const TsPack *tspack, RGBColor s) {
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
