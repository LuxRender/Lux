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

// schlickscatter.cpp*
#include "schlickscatter.h"
#include "spectrum.h"

using namespace lux;

SchlickScatter::SchlickScatter(const DifferentialGeometry &dgs,
	const Normal &ngeom, const Volume *exterior, const Volume *interior,
	const SWCSpectrum &r, const SWCSpectrum &g) :
	BSDF(dgs, ngeom, exterior, interior), R(r),
	k(g * (SWCSpectrum(1.55f) - 0.55f * g * g))
{
}
SWCSpectrum SchlickScatter::F(const SpectrumWavelengths &sw, const Vector &wo,
	const Vector &wi, bool reverse, BxDFType flags) const
{
	if (!(flags & BSDF_DIFFUSE))
		return SWCSpectrum(0.f);
	// 1+k*cos instead of 1-k*cos because wo is reversed compared to the
	// standard phase function definition
	const SWCSpectrum compcost = SWCSpectrum(1.f) + k * Dot(wo, wi);
	return R * (SWCSpectrum(1.f) - k * k) /
		(compcost * compcost * (4.f * M_PI));
}

bool SchlickScatter::SampleF(const SpectrumWavelengths &sw, const Vector &wo,
	Vector *wi, float u1, float u2, float u3, SWCSpectrum *const f_,
	float *pdf, BxDFType flags, BxDFType *sampledType, float *pdfBack,
	bool reverse) const
{
	if (!(flags & BSDF_DIFFUSE))
		return false;
	const float g = k.Filter(sw);
	// Add a - because wo is reversed compared to the standard phase
	// function definition
	const float cost = -(2.f * u1 + g - 1.f) / (2.f * g * u1 - g + 1.f);
	Vector x, y;
	CoordinateSystem(wo, &x, &y);
	*wi = SphericalDirection(sqrtf(max(0.f, 1.f - cost * cost)), cost,
		2.f * M_PI * u2, x, y, wo);
	// The - becomes a + because cost has been reversed above
	const float compcost = 1.f + g * cost;
	*pdf = (1.f - g * g) / (compcost * compcost * (4.f * M_PI));
	if (!(*pdf > 0.f))
		return false;
	if (pdfBack)
		*pdfBack = *pdf;
	if (sampledType)
		*sampledType = BSDF_DIFFUSE;

	*f_ = R;
	return true;
}
float SchlickScatter::Pdf(const SpectrumWavelengths &sw, const Vector &wo,
	const Vector &wi, BxDFType flags) const
{
	if (!(flags & BSDF_DIFFUSE))
		return 0.f;
	const float g = k.Filter(sw);
	// 1+k*cos instead of 1-k*cos because wo is reversed compared to the
	// standard phase function definition
	const float compcost = 1.f + g * Dot(wo, wi);
	 return (1.f - g * g) / (compcost * compcost * (4.f * M_PI));
}

