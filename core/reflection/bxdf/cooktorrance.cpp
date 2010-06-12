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

// cooktorrance.cpp*
#include "cooktorrance.h"
#include "spectrum.h"
#include "mc.h"
#include "microfacetdistribution.h"
#include "fresnel.h"

using namespace lux;

CookTorrance::CookTorrance(const SWCSpectrum &ks, MicrofacetDistribution *dist,
	Fresnel *fres) : BxDF(BxDFType(BSDF_REFLECTION | BSDF_SPECULAR)), KS(ks), distribution(dist), fresnel(fres)
{
}

void CookTorrance::f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f_) const {
	const float cosThetaO = fabsf(CosTheta(wo));
	const float cosThetaI = fabsf(CosTheta(wi));
	Vector wh(Normalize(wi + wo));
	if (wh.z < 0.f)
		wh = -wh;
	const float cosThetaH = Dot(wi, wh);
	const float cG = G(wo, wi, wh);

	SWCSpectrum F;
	fresnel->Evaluate(tspack, cosThetaH, &F);
	f_->AddWeighted(distribution->D(wh) * cG  / (M_PI * cosThetaI * cosThetaO), KS * F);
}

float CookTorrance::G(const Vector &wo, const Vector &wi, const Vector &wh) const
{
	const float NdotWh = fabsf(CosTheta(wh));
	const float NdotWo = fabsf(CosTheta(wo));
	const float NdotWi = fabsf(CosTheta(wi));
	const float WodotWh = AbsDot(wo, wh);
	return min(1.f, min((2.f * NdotWh * NdotWo / WodotWh), (2.f * NdotWh * NdotWi / WodotWh)));
}

bool CookTorrance::Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi, float u1, float u2, SWCSpectrum *const f_, float *pdf, float *pdfBack, bool reverse) const
{
	Vector wh;
	float d;
	distribution->SampleH(u1, u2, &wh, &d, pdf);
	if (wh.z < 0.f)
		wh = -wh;
	*wi = 2.f * Dot(wo, wh) * wh - wo;
	if (*pdf == 0.f) {
		if (pdfBack)
			*pdfBack = 0.f;
		return false;
	}
	const float cosThetaH = Dot(wo, wh);
	*pdf /= 4.f * fabsf(cosThetaH);
	if (pdfBack)
		*pdfBack = *pdf;

	SWCSpectrum F;
	fresnel->Evaluate(tspack, cosThetaH, &F);
	*f_ = (d * G(wo, *wi, wh)  / (M_PI * fabsf(wi->z) * fabsf(wo.z))) *
		(KS * F);
	return true;
}

float CookTorrance::Pdf(const TsPack *tspack, const Vector &wo, const Vector &wi) const
{
	if (!SameHemisphere(wo, wi))
		return 0.f;

	Vector wh(Normalize(wi + wo));
	if (wh.z < 0.f)
		wh = -wh;
	return distribution->Pdf(wh) / (4.f * AbsDot(wo, wh));
}

