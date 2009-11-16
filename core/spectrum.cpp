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

Scalar SWCSpectrum::Y(const TsPack *tspack) const {
	SpectrumWavelengths *sw = tspack->swl;
	Scalar y = 0.f;

	if (sw->single) {
		const u_int j = sw->single_w;
		y = sw->cie_Y[j] * c[j];
	} else {
		for (u_int j = 0; j < WAVELENGTH_SAMPLES; ++j) {
			y += sw->cie_Y[j] * c[j];
		}
	}

	return y;
}
Scalar SWCSpectrum::Filter(const TsPack *tspack) const
{
	SpectrumWavelengths *sw = tspack->swl;
	Scalar result = 0.f;
	if (sw->single) {
		result = c[sw->single_w];
	} else {
		for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			result += c[i];
		result *= inv_WAVELENGTH_SAMPLES;
	}
	return result;
}

SWCSpectrum::SWCSpectrum(const TsPack *tspack, const SPD *s) {
	SpectrumWavelengths *sw = tspack->swl;
	for (u_int j = 0; j < WAVELENGTH_SAMPLES; ++j) {
		c[j] = s->sample(sw->w[j]);
	}
}

SWCSpectrum::SWCSpectrum(const TsPack *tspack, const RGBColor &s) {
	SpectrumWavelengths *sw = tspack->swl;
	const float r = s.c[0];
	const float g = s.c[1];
	const float b = s.c[2];

	for (u_int j = 0; j < WAVELENGTH_SAMPLES; ++j)
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
