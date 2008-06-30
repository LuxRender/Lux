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

// microfacet.cpp*
#include "microfacet.h"
#include "color.h"
#include "spectrum.h"
#include "mc.h"
#include "sampling.h"
#include "fresnel.h"
#include "microfacetdistribution.h"
#include <stdarg.h>

using namespace lux;

Microfacet::Microfacet(const SWCSpectrum &reflectance,
                       Fresnel *f,
					   MicrofacetDistribution *d)
	: BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
	 R(reflectance), distribution(d), fresnel(f) {
}

SWCSpectrum Microfacet::f(const Vector &wo,
                       const Vector &wi) const {
	float cosThetaO = fabsf(CosTheta(wo));
	float cosThetaI = fabsf(CosTheta(wi));
	Vector wh = Normalize(wi + wo);
	float cosThetaH = Dot(wi, wh);
	SWCSpectrum F = fresnel->Evaluate(cosThetaH);
	return R * distribution->D(wh) * G(wo, wi, wh) * F /
		 (4.f * cosThetaI * cosThetaO);
}

SWCSpectrum Microfacet::Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf, float *pdfBack, bool reverse) const {
	distribution->Sample_f(wo, wi, u1, u2, pdf);
	if (pdfBack)
		*pdfBack = Pdf(*wi, wo);
	if (!SameHemisphere(wo, *wi)) return SWCSpectrum(0.f);
	if (reverse)
		return f(*wi, wo) * (wo.z / wi->z);
	else
		return f(wo, *wi);
}
float Microfacet::Pdf(const Vector &wo,
		const Vector &wi) const {
	if (!SameHemisphere(wo, wi)) return 0.f;
	return distribution->Pdf(wo, wi);
}

