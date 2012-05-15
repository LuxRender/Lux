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

// frequencyspd.cpp*
#include "frequencyspd.h"

using namespace lux;

void FrequencySPD::init(float freq, float phase, float refl) {
	fq = freq;
    ph = phase;
	r0 = refl;

	lambdaMin = FREQ_CACHE_START;
	lambdaMax = FREQ_CACHE_END;
	delta = (FREQ_CACHE_END - FREQ_CACHE_START) / (FREQ_CACHE_SAMPLES-1);
    invDelta = 1.f / delta;
	nSamples = FREQ_CACHE_SAMPLES;

	AllocateSamples(FREQ_CACHE_SAMPLES);

	// Fill samples with Frequency curve
	for(int i=0; i<FREQ_CACHE_SAMPLES; i++) {
		float w = (FREQ_CACHE_START + (delta*i));
		samples[i] = (sinf(w * freq + phase) + 1.0f) * 0.5f * refl;
	}

	Clamp();
}
