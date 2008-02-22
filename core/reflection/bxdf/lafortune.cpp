/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

// lafortune.cpp*
#include "lafortune.h"
#include "color.h"
#include "spectrum.h"
#include "mc.h"
#include "sampling.h"
#include <stdarg.h>

using namespace lux;

Lafortune::Lafortune(const SWCSpectrum &r, u_int nl,
	const SWCSpectrum *xx,
	const SWCSpectrum *yy,
	const SWCSpectrum *zz,
	const SWCSpectrum *e, BxDFType t)
	: BxDF(t), R(r) {
	nLobes = nl;
	x = xx;
	y = yy;
	z = zz;
	exponent = e;
}
SWCSpectrum Lafortune::f(const Vector &wo,
                      const Vector &wi) const {
	SWCSpectrum ret = R * INV_PI;
	for (u_int i = 0; i < nLobes; ++i) {
		// Add contribution for $i$th Phong lobe
		SWCSpectrum v = x[i] * wo.x * wi.x + y[i] * wo.y * wi.y +
			z[i] * wo.z * wi.z;
		ret += v.Pow(exponent[i]);
	}
	return ret;
}

SWCSpectrum Lafortune::Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf) const {
	u_int comp = lux::random::uintValue() % (nLobes+1);
	if (comp == nLobes) {
		// Cosine-sample the hemisphere, flipping the direction if necessary
		*wi = CosineSampleHemisphere(u1, u2);
		if (wo.z < 0.) wi->z *= -1.f;
	}
	else {
		// Sample lobe _comp_ for Lafortune BRDF
		float xlum = x[comp].y();
		float ylum = y[comp].y();
		float zlum = z[comp].y();
		float costheta = powf(u1, 1.f / (.8f * exponent[comp].y() + 1));
		float sintheta = sqrtf(max(0.f, 1.f - costheta*costheta));
		float phi = u2 * 2.f * M_PI;
		Vector lobeCenter = Normalize(Vector(xlum * wo.x, ylum * wo.y, zlum * wo.z));
		Vector lobeX, lobeY;
		CoordinateSystem(lobeCenter, &lobeX, &lobeY);
		*wi = SphericalDirection(sintheta, costheta, phi, lobeX, lobeY,
			lobeCenter);
	}
	if (!SameHemisphere(wo, *wi)) return SWCSpectrum(0.f);
	*pdf = Pdf(wo, *wi);
	return f(wo, *wi);
}
float Lafortune::Pdf(const Vector &wo, const Vector &wi) const {
	if (!SameHemisphere(wo, wi)) return 0.f;
	float pdfSum = fabsf(wi.z) * INV_PI;
	for (u_int i = 0; i < nLobes; ++i) {
		float xlum = x[i].y();
		float ylum = y[i].y();
		float zlum = z[i].y();
		Vector lobeCenter =
			Normalize(Vector(wo.x * xlum, wo.y * ylum, wo.z * zlum));
		float e = .8f * exponent[i].y();
		pdfSum += (e + 1.f) * powf(max(0.f, Dot(wi, lobeCenter)), e);
	}
	return pdfSum / (1.f + nLobes);
}