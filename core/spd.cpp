/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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

// spd.cpp*
#include "spd.h"
#include "memory.h"

using namespace lux;

void SPD::AllocateSamples(int n) {
	 // Allocate memory for samples
	samples = (float *)
		AllocAligned(n * sizeof(float));
}

void SPD::FreeSamples() {
	 // Free Allocate memory for samples
	FreeAligned(samples);
}

void SPD::Normalize() {
	float max = 0.f;

	for(int i=0; i<nSamples; i++)
		if(samples[i] > max)
			max = samples[i];

	float scale = 1.f/max;

	for(int i=0; i<nSamples; i++)
		samples[i] *= scale;
}

void SPD::Clamp() {
	for(int i=0; i<nSamples; i++) {
		if(samples[i] < 0.f) samples[i] = 0.f;
		if(samples[i] > INFINITY) samples[i] = INFINITY;
	}
}

void SPD::Scale(float s) {
	for(int i=0; i<nSamples; i++)
		samples[i] *= s;
}

