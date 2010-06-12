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

// blinn.cpp*
#include "blinn.h"

using namespace lux;

void Blinn::SampleH(float u1, float u2, Vector *wh, float *d, float *pdf) const
{
	// Compute sampled half-angle vector $\wh$ for Blinn distribution
	const float cosTheta = powf(u1, 1.f / (exponent + 1.f));
	const float sinTheta = sqrtf(max(0.f, 1.f - cosTheta * cosTheta));
	const float phi = u2 * 2.f * M_PI;
	*wh = SphericalDirection(sinTheta, cosTheta, phi);
	const float f = powf(cosTheta, exponent) * INV_TWOPI;
	*d = (exponent + 2.f) * f;
	// Compute PDF for \wi from Blinn distribution
	*pdf = (exponent + 1.f) * f;
}

float Blinn::Pdf(const Vector &wh) const
{
	const float cosTheta = fabsf(wh.z);
	// Compute PDF for \wi from Blinn distribution
	return (exponent + 1.f) * powf(cosTheta, exponent) * INV_TWOPI;
}
