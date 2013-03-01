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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/  

// gaussianspd.cpp*
#include "gaussianspd.h"

using namespace lux;

void GaussianSPD::init(float mean, float width, float refl) {
	mu = mean;
    wd = width;
	r0 = refl;

	float scale2 = float(-0.5 / (width * width));

	lambdaMin = GAUSS_CACHE_START;
	lambdaMax = GAUSS_CACHE_END;
	delta = (GAUSS_CACHE_END - GAUSS_CACHE_START) / (GAUSS_CACHE_SAMPLES-1);
    invDelta = 1.f / delta;
	nSamples = GAUSS_CACHE_SAMPLES;

	AllocateSamples(GAUSS_CACHE_SAMPLES);

	// Fill samples with Gaussian curve
	for(int i=0; i<GAUSS_CACHE_SAMPLES; i++) {
		float w = (GAUSS_CACHE_START + (delta*i));
		float x = w - mu;
		samples[i] = refl * expf(x * x * scale2);
	}

	Clamp();
}
