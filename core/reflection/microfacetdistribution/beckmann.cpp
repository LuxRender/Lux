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

// beckmann.cpp*
#include "beckmann.h"
#include "luxrays/core/geometry/vector.h"
using luxrays::Vector;
using luxrays::SphericalDirection;

using namespace lux;

Beckmann::Beckmann(float rms) : r(rms)
{
}

float Beckmann::D(const Vector &wh) const
{
	float cosTheta = wh.z;
	float theta = acosf(cosTheta);
	float tanTheta = tanf(theta);

	float dfac = tanTheta / r;

	return expf(-(dfac * dfac)) / (r * r * powf(cosTheta, 4.f));
}

void Beckmann::SampleH(float u1, float u2, Vector *wh, float *d,
	float *pdf) const
{
	// Compute sampled half-angle vector $\wh$ for Beckmann distribution
	// Adapted from B. Walter et al, Microfacet Models for Refraction,
	// Eurographics Symposium on Rendering, 2007, page 7

	float theta = atanf(sqrtf(max(0.f, -(r * r) * logf(1.f - u1))));
	float cosTheta = cosf(theta);
	float sinTheta = sqrtf(max(0.f, 1.f - cosTheta * cosTheta));
	float phi = u2 * 2.f * M_PI;

	*wh = SphericalDirection(sinTheta, cosTheta, phi);

	// Compute PDF for \wi from Beckmann distribution
	// note that the inverse of the integral over
	// the Beckmann distribution is not available in closed form,
	// so this is not really correct
	// (see Kelemen and Szirmay-Kalos / Microfacet Based BRDF Model,
	// Eurographics 2001)
	const float dfac = sinTheta / (cosTheta * r);
	*d = expf(-(dfac * dfac)) / (r * r * powf(cosTheta, 4.f));
	*pdf = *d;
}

// NB: See note above!
float Beckmann::Pdf(const Vector &wh) const
{
	return D(wh);
}

float Beckmann::HalfG(const Vector &w, const Vector &h) const
{
	if (!SameHemisphere(w, h))
		return 0.f;
	const float cosTheta = w.z;
	const float theta = acosf(cosTheta);
	const float tanTheta = tanf(theta);
	const float a = 1.f / (r * tanTheta);
	if (a >= 1.6f)
		return 1.f;
	// The true formula is:
	// 2/(1+erf(a)+exp(-a*a)/(a*sqrt(pi)))
	// This approximation has a relative error less than 0.35%
	return a * (3.535f + a * 2.181) / (1 + a * (2.276f + a * 2.577f));
}
