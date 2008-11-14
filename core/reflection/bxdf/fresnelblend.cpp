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

// fresnelblend.cpp*
#include "fresnelblend.h"
#include "spectrum.h"
#include "mc.h"
#include "microfacetdistribution.h"

using namespace lux;

FresnelBlend::FresnelBlend(const SWCSpectrum &d,
                           const SWCSpectrum &s,
						   const SWCSpectrum &a,
						   float dep,
						   MicrofacetDistribution *dist)
	: BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
	  Rd(d), Rs(s), Alpha(a), depth(dep) {
	distribution = dist;
}
void FresnelBlend::f(const TsPack *tspack, const Vector &wo, 
					 const Vector &wi, SWCSpectrum *const f) const {
	// absorption
	SWCSpectrum a(1.);
	
	if (depth > 0) {
		float depthfactor = depth * (1 / fabsf(CosTheta(wi)) + 1 / fabsf(CosTheta(wo)));
		a = Exp(Alpha * -depthfactor);
	}

	// diffuse part
	f->AddWeighted((1 - powf(1 - .5f * fabsf(CosTheta(wi)), 5)) *
		(1 - powf(1 - .5f * fabsf(CosTheta(wo)), 5)) *
		(28.f/(23.f*M_PI)), a * Rd * (SWCSpectrum(1.) - Rs));

	Vector H = Normalize(wi + wo);
	// specular part
	f->AddWeighted(distribution->D(H) /
		(4.f * AbsDot(wi, H) *
		max(fabsf(CosTheta(wi)), fabsf(CosTheta(wo)))),
		SchlickFresnel(Dot(wi, H)));	
}

bool FresnelBlend::Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi,
							float u1, float u2, SWCSpectrum *const f, float *pdf, 
							float *pdfBack, bool reverse) const {
	u1 *= 2.f;
	if (u1 < 1.f) {
		// Cosine-sample the hemisphere, flipping the direction if necessary
		*wi = CosineSampleHemisphere(u1, u2);
		if (wo.z < 0.) wi->z *= -1.f;
	}
	else {
		u1 -= 1.f;
		distribution->Sample_f(wo, wi, u1, u2, pdf);
	}
	*pdf = Pdf(tspack, wo, *wi);
	if (*pdf == 0.f) {
		if (pdfBack)
			*pdfBack = 0.f;
		return false;
	}
	if (pdfBack)
		*pdfBack = Pdf(tspack, *wi, wo);

	*f = SWCSpectrum(0.f);
	if (reverse) {
		this->f(tspack, *wi, wo, f);
//		*f *= (wo.z / wi->z);	
	}
	else
		this->f(tspack, wo, *wi, f);
	return true;
}
float FresnelBlend::Pdf(const TsPack *tspack, const Vector &wo,
		const Vector &wi) const {
	if (!SameHemisphere(wo, wi)) return 0.f;
	return .5f * (fabsf(wi.z) * INV_PI +
		distribution->Pdf(wo, wi));
}

