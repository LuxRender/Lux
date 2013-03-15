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

// blackbody.cpp*
#include "blackbodyspd.h"

using namespace lux;

void BlackbodySPD::init(float t) {
	temp = t;

	lambdaMin = BB_CACHE_START;
	lambdaMax = BB_CACHE_END;
	delta = (BB_CACHE_END - BB_CACHE_START) / (BB_CACHE_SAMPLES-1);
	invDelta = 1.f / delta;
	nSamples = BB_CACHE_SAMPLES;

	AllocateSamples(BB_CACHE_SAMPLES);

	// Fill samples with BB curve
	for(int i=0; i<BB_CACHE_SAMPLES; i++) {
		float w = 1e-9f * (BB_CACHE_START + (delta*i));
		// Compute blackbody power for wavelength w and temperature temp
		samples[i] = 0.4e-9f * (3.74183e-16f * powf(w, -5.f))
				/ (expf(1.4388e-2f / (w * temp)) - 1.f);
	}

	Normalize();
	Clamp();
}
