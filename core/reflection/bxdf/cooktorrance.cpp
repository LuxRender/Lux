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

// cooktorrance.cpp*
#include "cooktorrance.h"
#include "spectrum.h"
#include "microfacetdistribution.h"
#include "fresnel.h"

#include "luxrays/utils/mc.h"

using namespace lux;

CookTorrance::CookTorrance(const SWCSpectrum &ks, MicrofacetDistribution *dist,
	Fresnel *fres) : BxDF(BxDFType(BSDF_REFLECTION | BSDF_SPECULAR)),
	KS(ks), distribution(dist), fresnel(fres)
{
}

void CookTorrance::F(const SpectrumWavelengths &sw, const Vector &wo,
	const Vector &wi, SWCSpectrum *const f_) const
{
	const float cosThetaI = fabsf(CosTheta(wi));
	Vector wh(Normalize(wi + wo));
	if (wh.z < 0.f)
		wh = -wh;
	const float cosThetaH = Dot(wi, wh);
	const float cG = distribution->G(wo, wi, wh);

	SWCSpectrum F;
	fresnel->Evaluate(sw, cosThetaH, &F);
	f_->AddWeighted(distribution->D(wh) * cG  / (M_PI * cosThetaI), KS * F);
}

bool CookTorrance::SampleF(const SpectrumWavelengths &sw, const Vector &wo,
	Vector *wi, float u1, float u2, SWCSpectrum *const f_, float *pdf,
	float *pdfBack, bool reverse) const
{
	Vector wh;
	float d;
	distribution->SampleH(u1, u2, &wh, &d, pdf);
	if (wh.z < 0.f)
		wh = -wh;
	const float cosThetaH = Dot(wo, wh);
	*wi = 2.f * cosThetaH * wh - wo;
	if (*pdf == 0.f)
		return false;
	SWCSpectrum F;
	fresnel->Evaluate(sw, cosThetaH, &F);
	const float factor = d * fabsf(cosThetaH) / *pdf *
		distribution->G(wo, *wi, wh) * 4.f * INV_PI;
	if (reverse)
		*f_ = (factor / fabsf(wo.z)) *	(KS * F);
	else
		*f_ = (factor / fabsf(wi->z)) * (KS * F);
	*pdf /= 4.f * fabsf(cosThetaH);
	if (pdfBack)
		*pdfBack = *pdf;

	return true;
}

float CookTorrance::Pdf(const SpectrumWavelengths &sw, const Vector &wo,
	const Vector &wi) const
{
	if (!SameHemisphere(wo, wi))
		return 0.f;

	Vector wh(Normalize(wi + wo));
	if (wh.z < 0.f)
		wh = -wh;
	return distribution->Pdf(wh) / (4.f * AbsDot(wo, wh));
}

