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

// schlicktranslucentbtdf.cpp*
#include "schlicktranslucentbtdf.h"
#include "spectrum.h"
#include "mc.h"
#include "microfacetdistribution.h"

using namespace lux;

SchlickTranslucentBTDF::SchlickTranslucentBTDF(const SWCSpectrum &d,
	const SWCSpectrum &t, const SWCSpectrum &s, const SWCSpectrum &s2,
	const SWCSpectrum &a, const SWCSpectrum &a2, float dep, float dep2)
	: BxDF(BxDFType(BSDF_DIFFUSE | BSDF_TRANSMISSION)),
	Rd(d), Rt(t), Rs(s), Rs_bf(s2), Alpha(a), Alpha_bf(a2),
	depth(dep), depth_bf(dep2)
{
}

void SchlickTranslucentBTDF::F(const SpectrumWavelengths &sw, const Vector &wo, 
	 const Vector &wi, SWCSpectrum *const f_) const
{
	const float cosi = fabsf(CosTheta(wi));
	const float coso = fabsf(CosTheta(wo));

	const Vector H(Normalize(Vector(wi.x + wo.x, wi.y + wo.y, wi.z - wo.z)));
	const float u = AbsDot(wi, H);
	const SWCSpectrum S1(SchlickFresnel(u, Rs));
	const SWCSpectrum S2(SchlickFresnel(u, Rs_bf));
	SWCSpectrum S(((SWCSpectrum(1.f) - S1) * (SWCSpectrum(1.f) - S2)).Sqrt());
	if (CosTheta(wi) > 0.f) {
		if (depth > 0.f || depth_bf > 0.f)
			S *= Exp(Alpha * -(depth / cosi) + Alpha_bf * -(depth_bf / coso));
	} else {
		if (depth > 0.f || depth_bf > 0.f)
			S *= Exp(Alpha * -(depth / coso) + Alpha_bf * -(depth_bf / cosi));
	}
	f_->AddWeighted(INV_PI * coso, S * Rt * (SWCSpectrum(1.f) - Rd));
}

bool SchlickTranslucentBTDF::SampleF(const SpectrumWavelengths &sw, const Vector &wo,
	Vector *wi, float u1, float u2, SWCSpectrum *const f_, float *pdf, 
	float *pdfBack, bool reverse) const
{
	// Cosine-sample the hemisphere, flipping the direction if necessary
	*wi = CosineSampleHemisphere(u1, u2);
	if (wo.z > 0.f) wi->z = -(wi->z);	// make sure wi is in opposite hemisphere
	// wi may be in the tangent plane, which will 
	// fail the SameHemisphere test in Pdf()
	if (SameHemisphere(wo, *wi))
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

float SchlickTranslucentBTDF::Pdf(const SpectrumWavelengths &sw, const Vector &wo,
	const Vector &wi) const
{
	return SameHemisphere(wo, wi) ? 0.f : fabsf(wi.z) * INV_PI;
}
