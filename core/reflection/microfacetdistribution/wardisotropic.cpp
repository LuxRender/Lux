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

// wardisotropic.cpp*
#include "wardisotropic.h"
#include "geometry/vector.h"

using namespace lux;

// Ward isotropic distribution, adapted from Kelemen and Szirmay-Kalos / Microfacet Based BRDF Model, Eurographics 2001
WardIsotropic::WardIsotropic(float rms) : r(rms)
{
}

float WardIsotropic::D(const Vector &wh) const
{
	const float cosTheta = fabsf(wh.z);
	const float theta = acosf(cosTheta);
	const float tanTheta = tanf(theta);

	const float dfac = tanTheta / r;

	return expf(-(dfac * dfac)) / (M_PI * r * r * powf(cosTheta, 3.f));
}

void WardIsotropic::SampleH(float u1, float u2, Vector *wh, float *d,
	float *pdf) const
{
	// Compute sampled half-angle vector $\wh$ for Ward distribution

	const float theta = atanf(r * sqrtf(max(0.f, -logf(1.f - u1))));
	const float cosTheta = cosf(theta);
	const float sinTheta = sqrtf(max(0.f, 1.f - cosTheta * cosTheta));
	const float phi = u2 * 2.f * M_PI;

	*wh = SphericalDirection(sinTheta, cosTheta, phi);
	*d = D(*wh);
	*pdf = *d;
}

float WardIsotropic::Pdf(const Vector &wh) const
{
	return D(wh);
}

float WardIsotropic::G(const Vector &wo, const Vector &wi, const Vector &wh) const
{
	const float NdotWh = fabsf(CosTheta(wh));
	const float NdotWo = fabsf(CosTheta(wo));
	const float NdotWi = fabsf(CosTheta(wi));
	const float WOdotWh = AbsDot(wo, wh);
	const float WIdotWh = AbsDot(wi, wh);
	return min(1.f, min((2.f * NdotWh * NdotWo / WOdotWh),
	                (2.f * NdotWh * NdotWi / WIdotWh)));
}
