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
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

// lafortune.cpp*
#include "lafortune.h"
#include "luxrays/core/color/swcspectrum.h"

#include "luxrays/utils/mc.h"

using namespace lux;

Lafortune::Lafortune(const SWCSpectrum &xx, const SWCSpectrum &yy,
	const SWCSpectrum &zz, const SWCSpectrum &e)
	: BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)), x(xx), y(yy), z(zz),
	exponent(e)
{
}
void Lafortune::F(const SpectrumWavelengths &sw, const Vector &wo,
	const Vector &wi, SWCSpectrum *const f_) const
{
	SWCSpectrum v(x * (wo.x * wi.x) + y * (wo.y * wi.y) + z * (wo.z * wi.z));
	*f_ += Pow(v, exponent) * fabsf(wo.z);
}

bool Lafortune::SampleF(const SpectrumWavelengths &sw, const Vector &wo,
	Vector *wi, float u1, float u2, SWCSpectrum *const f_, float *pdf,
	float *pdfBack, bool reverse) const
{
	const float xlum = x.Filter(sw);
	const float ylum = y.Filter(sw);
	const float zlum = z.Filter(sw);
	const float costheta = powf(u1, 1.f / (.8f * exponent.Filter(sw) + 1.f));
	const float sintheta = sqrtf(max(0.f, 1.f - costheta * costheta));
	const float phi = u2 * 2.f * M_PI;
	const Vector lobeCenter(Normalize(Vector(xlum * wo.x, ylum * wo.y, zlum * wo.z)));
	Vector lobeX, lobeY;
	CoordinateSystem(lobeCenter, &lobeX, &lobeY);
	*wi = SphericalDirection(sintheta, costheta, phi, lobeX, lobeY,
		lobeCenter);
	if (!SameHemisphere(wo, *wi))
		return false;
	*pdf = Pdf(sw, wo, *wi);
	if (pdfBack)
		*pdfBack = Pdf(sw, *wi, wo);
	*f_ = SWCSpectrum(0.f);
	if (reverse)
		F(sw, *wi, wo, f_);
	else
		F(sw, wo, *wi, f_);
	*f_ /= *pdf;
	return true;
}


float Lafortune::Pdf(const SpectrumWavelengths &sw, const Vector &wo,
	const Vector &wi) const
{
	if (!SameHemisphere(wo, wi))
		return 0.f;
	const float xlum = x.Filter(sw);
	const float ylum = y.Filter(sw);
	const float zlum = z.Filter(sw);
	const Vector lobeCenter(Normalize(Vector(wo.x * xlum, wo.y * ylum, wo.z * zlum)));
	const float e = .8f * exponent.Filter(sw);
	return (e + 1.f) * powf(max(0.f, Dot(wi, lobeCenter)), e);
}

