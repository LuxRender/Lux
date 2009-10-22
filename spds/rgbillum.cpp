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

// rgbillum.cpp*
#include "rgbillum.h"
#include "memory.h"

using namespace lux;

#include "data/rgbD65_32.h"

void RGBIllumSPD::init(RGBColor s)
{
	lambdaMin = illumrgb2spect_start;
	lambdaMax = illumrgb2spect_end;
	u_int n = illumrgb2spect_bins;
	delta = (lambdaMax - lambdaMin) / (n - 1);
	invDelta = 1.f / delta;
	nSamples = n;

	AllocateSamples(n);

	// Zero out
	for (u_int i = 0; i < n; ++i)
		samples[i] = 0.f;

	float r = s.c[0];
	float g = s.c[1];
	float b = s.c[2];

	if (r <= g && r <= b) {
		AddWeighted(r, illumrgb2spect_white);

		if (g <= b) {
			AddWeighted(g - r, illumrgb2spect_cyan);
			AddWeighted(b - g, illumrgb2spect_blue);
		} else {
			AddWeighted(b - r, illumrgb2spect_cyan);
			AddWeighted(g - b, illumrgb2spect_green);
		}
	} else if (g <= r && g <= b) {
		AddWeighted(g, illumrgb2spect_white);

		if (r <= b) {
			AddWeighted(r - g, illumrgb2spect_magenta);
			AddWeighted(b - r, illumrgb2spect_blue);
		} else {
			AddWeighted(b - g, illumrgb2spect_magenta);
			AddWeighted(r - b, illumrgb2spect_red);
		}
	} else { // blue <= red && blue <= green
		AddWeighted(b, illumrgb2spect_white);

		if (r <= g) {
			AddWeighted(r - b, illumrgb2spect_yellow);
			AddWeighted(g - r, illumrgb2spect_green);
		} else {
			AddWeighted(g - b, illumrgb2spect_yellow);
			AddWeighted(r - g, illumrgb2spect_red);
		}
	}

	Scale(illumrgb2spect_scale);
	Clamp();
}

