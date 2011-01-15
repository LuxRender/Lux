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

// rgbrefl.cpp*
#include "rgbrefl.h"
#include "memory.h"

using namespace lux;

#include "data/rgbE_32.h"

void RGBReflSPD::init(const RGBColor &s)
{
	lambdaMin = refrgb2spect_start;
	lambdaMax = refrgb2spect_end;
	u_int n = refrgb2spect_bins;
	delta = (lambdaMax - lambdaMin) / (n - 1);
	invDelta = 1.f / delta;
	nSamples = n;

	AllocateSamples(n);

	// Zero out
	for (u_int i = 0; i < n; ++i)
		samples[i] = 0.f;

	const float r = s.c[0];
	const float g = s.c[1];
	const float b = s.c[2];

	bool clamp = (r >= 0.f) && (g >= 0.f) && (b >= 0.f);

	if (r <= g && r <= b) {
		AddWeighted(r, refrgb2spect_white);

		if (g <= b) {
			AddWeighted(g - r, refrgb2spect_cyan);
			AddWeighted(b - g, refrgb2spect_blue);
		} else {
			AddWeighted(b - r, refrgb2spect_cyan);
			AddWeighted(g - b, refrgb2spect_green);
		}
	} else if (g <= r && g <= b) {
		AddWeighted(g, refrgb2spect_white);

		if (r <= b) {
			AddWeighted(r - g, refrgb2spect_magenta);
			AddWeighted(b - r, refrgb2spect_blue);
		} else {
			AddWeighted(b - g, refrgb2spect_magenta);
			AddWeighted(r - b, refrgb2spect_red);
		}
	} else { // blue <= red && blue <= green
		AddWeighted(b, refrgb2spect_white);

		if (r <= g) {
			AddWeighted(r - b, refrgb2spect_yellow);
			AddWeighted(g - r, refrgb2spect_green);
		} else {
			AddWeighted(g - b, refrgb2spect_yellow);
			AddWeighted(r - g, refrgb2spect_red);
		}
	}

	Scale(refrgb2spect_scale);
	if (clamp)
		Clamp();
}

