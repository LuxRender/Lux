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

// spd.cpp*
#include "lux.h"
#include "color.h"
#include "spd.h"
#include "memory.h"
#include "data/xyzbasis.h"

using namespace lux;
using namespace luxrays;

void SPD::AllocateSamples(u_int n) {
	 // Allocate memory for samples
	samples = AllocAligned<float>(n);
}

void SPD::FreeSamples() {
	 // Free Allocate memory for samples
	if (samples)
		FreeAligned(samples);
}

void SPD::Normalize() {
	float max = 0.f;

	for (u_int i = 0; i < nSamples; ++i)
		if(samples[i] > max)
			max = samples[i];

	const float scale = 1.f / max;

	for (u_int i = 0; i < nSamples; ++i)
		samples[i] *= scale;
}

void SPD::Clamp() {
	for (u_int i = 0; i < nSamples; ++i) {
		if (!(samples[i] > 0.f))
			samples[i] = 0.f;
	}
}

void SPD::Scale(float s) {
	for (u_int i = 0; i < nSamples; ++i)
		samples[i] *= s;
}

void SPD::Whitepoint(float temp) {
	vector<float> bbvals;

	// Fill bbvals with BB curve
	float w = lambdaMin * 1e-9f;
	for (u_int i = 0; i < nSamples; ++i) {
		// Compute blackbody power for wavelength w and temperature temp
		bbvals.push_back(4e-9f * (3.74183e-16f * powf(w, -5.f))
				/ (expf(1.4388e-2f / (w * temp)) - 1.f));
		w += 1e-9f * delta;
	}

	// Normalize
	float max = 0.f;
	for (u_int i = 0; i < nSamples; ++i)
		if (bbvals[i] > max)
			max = bbvals[i];
	const float scale = 1.f / max;
	// Multiply
	for (u_int i = 0; i < nSamples; ++i)
		samples[i] *= bbvals[i] * scale;
}

float SPD::Y() const
{
	float y = 0.f;
	for (u_int i = 0; i < nCIE; ++i)
		y += Sample(i + CIEstart) * CIE_Y[i];
	return y * 683.f;
}
float SPD::Filter() const
{
	float y = 0.f;
	for (u_int i = 0; i < nSamples; ++i)
		y += samples[i];
	return y / nSamples;
}

XYZColor SPD::ToXYZ() const {
	XYZColor c(0.f);
	for (u_int i = 0; i < nCIE; ++i) {
		const float s = Sample(i + CIEstart);
		c.c[0] += s * CIE_X[i];
		c.c[1] += s * CIE_Y[i];
		c.c[2] += s * CIE_Z[i];
	}
	return c * 683.f;
}

XYZColor SPD::ToNormalizedXYZ() const {
	XYZColor c(0.f);
	float yint  = 0.f;
	for (u_int i = 0; i < nCIE; ++i) {
		yint += CIE_Y[i];

		const float s = Sample(i + CIEstart);
		c.c[0] += s * CIE_X[i];
		c.c[1] += s * CIE_Y[i];
		c.c[2] += s * CIE_Z[i];
	}
	return c / yint;
}
