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

// singlebsdf.cpp*
#include "singlebsdf.h"
#include "spectrum.h"

using namespace lux;

bool SingleBSDF::SampleF(const SpectrumWavelengths &sw, const Vector &woW, Vector *wiW,
	float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
	BxDFType flags, BxDFType *sampledType, float *pdfBack,
	bool reverse) const
{
	BOOST_ASSERT(bxdf); // NOBOOK
	if (!bxdf->MatchesFlags(flags))
		return false;
	// Sample chosen _BxDF_
	if (!bxdf->SampleF(sw, WorldToLocal(woW), wiW, u1, u2, f_,
		pdf, pdfBack, reverse))
		return false;
	if (sampledType)
		*sampledType = bxdf->type;
	*wiW = LocalToWorld(*wiW);
	const float sideTest = Dot(*wiW, ng) / Dot(woW, ng);
	if (sideTest > 0.f) {
		// ignore BTDFs
		if (bxdf->type & BSDF_TRANSMISSION)
			return false;
	} else if (sideTest < 0.f) {
		// ignore BRDFs
		if (bxdf->type & BSDF_REFLECTION)
			return false;
	} else
		return false;
	if (!reverse)
		*f_ *= fabsf(sideTest);
	return true;
}
float SingleBSDF::Pdf(const SpectrumWavelengths &sw, const Vector &woW,
	const Vector &wiW, BxDFType flags) const
{
	if (!bxdf->MatchesFlags(flags))
		return 0.f;
	return bxdf->Pdf(sw, WorldToLocal(woW), WorldToLocal(wiW));
}

SWCSpectrum SingleBSDF::F(const SpectrumWavelengths &sw, const Vector &woW,
	const Vector &wiW, bool reverse, BxDFType flags) const
{
	const float sideTest = Dot(wiW, ng) / Dot(woW, ng);
	if (sideTest > 0.f)
		// ignore BTDFs
		flags = BxDFType(flags & ~BSDF_TRANSMISSION);
	else if (sideTest < 0.f)
		// ignore BRDFs
		flags = BxDFType(flags & ~BSDF_REFLECTION);
	else
		flags = static_cast<BxDFType>(0);
	if (!bxdf->MatchesFlags(flags))
		return SWCSpectrum(0.f);
	SWCSpectrum f_(0.f);
	bxdf->F(sw, WorldToLocal(woW), WorldToLocal(wiW), &f_);
	if (!reverse)
		f_ *= fabsf(sideTest);
	return f_;
}
SWCSpectrum SingleBSDF::rho(const SpectrumWavelengths &sw, BxDFType flags) const
{
	if (!bxdf->MatchesFlags(flags))
		return SWCSpectrum(0.f);
	return bxdf->rho(sw);
}
SWCSpectrum SingleBSDF::rho(const SpectrumWavelengths &sw, const Vector &woW,
	BxDFType flags) const
{
	if (!bxdf->MatchesFlags(flags))
		return SWCSpectrum(0.f);
	return bxdf->rho(sw, WorldToLocal(woW));
}
