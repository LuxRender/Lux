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

// microfacet.cpp*
#include "microfacet.h"
#include "spectrum.h"
#include "mc.h"
#include "fresnel.h"
#include "microfacetdistribution.h"

using namespace lux;

Microfacet::Microfacet(const SWCSpectrum &reflectance,
                       Fresnel *f,
					   MicrofacetDistribution *d)
	: BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
	 R(reflectance), distribution(d), fresnel(f) {
}

void Microfacet::f(const TsPack *tspack, const Vector &wo, 
				   const Vector &wi, SWCSpectrum *const f) const {
	float cosThetaO = fabsf(CosTheta(wo));
	float cosThetaI = fabsf(CosTheta(wi));
	Vector wh = Normalize(wi + wo);
	float cosThetaH = Dot(wi, wh);
	SWCSpectrum F;
	fresnel->Evaluate(tspack, cosThetaH, &F);
	f->AddWeighted(distribution->D(wh) * G(wo, wi, wh) /
		(4.f * cosThetaI * cosThetaO), R * F);
}

bool Microfacet::Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi,
						  float u1, float u2, SWCSpectrum *const f, float *pdf, 
						  float *pdfBack,	bool reverse) const {
	distribution->Sample_f(wo, wi, u1, u2, pdf);
	if (pdfBack)
		*pdfBack = Pdf(tspack, *wi, wo);
	if (!SameHemisphere(wo, *wi)) 
		return false;

	*f = SWCSpectrum(0.f);
	if (reverse) {
		this->f(tspack, *wi, wo, f);
	}
	else
		this->f(tspack, wo, *wi, f);
	return true;
}
float Microfacet::Pdf(const TsPack *tspack, const Vector &wo,
		const Vector &wi) const {
	if (!SameHemisphere(wo, wi)) return 0.f;
	return distribution->Pdf(wo, wi);
}

