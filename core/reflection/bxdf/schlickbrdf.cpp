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

// schlickbrdf.cpp*
#include "schlickbrdf.h"
#include "spectrum.h"
#include "mc.h"
#include "microfacetdistribution.h"

using namespace lux;

SchlickBRDF::SchlickBRDF(const SWCSpectrum &d, const SWCSpectrum &s,
	const SWCSpectrum &a, float dep, float r, float p)
	: BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
	  Rd(d), Rs(s), Alpha(a), depth(dep), roughness(r), anisotropy(p)
{
}
void SchlickBRDF::f(const TsPack *tspack, const Vector &wo, 
	 const Vector &wi, SWCSpectrum *const f_) const
{
	const float cosi = fabsf(CosTheta(wi));
	const float coso = fabsf(CosTheta(wo));
	// absorption
	SWCSpectrum a(1.f);
	
	if (depth > 0.f) {
		// 1/cosi+1/coso=(cosi+coso)/(cosi*coso)
		float depthfactor = depth * (cosi + coso) / (cosi * coso);
		a = Exp(Alpha * -depthfactor);
	}
	const Vector H(Normalize(wo + wi));
	const float u = AbsDot(wi, H);
	const SWCSpectrum S(SchlickFresnel(u));

	// diffuse part
	f_->AddWeighted(INV_PI, a * Rd * (SWCSpectrum(1.f) - S));

	// specular part
	f_->AddWeighted(SchlickD(cosi, coso, H), S);	
}

static float GetPhi(float a, float b)
{
	return M_PI * .5f * sqrtf(a * b / (1.f - a * (1.f - b)));
}

bool SchlickBRDF::Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi,
	float u1, float u2, SWCSpectrum *const f_, float *pdf, 
	float *pdfBack, bool reverse) const
{
	Vector H;
	float cosWH;
	u1 *= 2.f;
	if (u1 < 1.f) {
		// Cosine-sample the hemisphere, flipping the direction if necessary
		*wi = CosineSampleHemisphere(u1, u2);
		if (wo.z < 0.f)
			wi->z *= -1.f;
		H = Normalize(wo + *wi);
		cosWH = AbsDot(wo, H);
	} else {
		u1 -= 1.f;
		u2 *= 4.f;
		const float cos2theta = u1 / (roughness * (1 - u1) + u1);
		const float costheta = sqrtf(cos2theta);
		const float sintheta = sqrtf(1.f - cos2theta);
		const float p = 1.f - fabsf(anisotropy);
		float phi;
		if (u2 < 1.f) {
			phi = GetPhi(u2 * u2, p * p);
		} else if (u2 < 2.f) {
			u2 = 2.f - u2;
			phi = M_PI - GetPhi(u2 * u2, p * p);
		} else if (u2 < 3.f) {
			u2 -= 2.f;
			phi = M_PI + GetPhi(u2 * u2, p * p);
		} else {
			u2 = 4.f - u2;
			phi = M_PI * 2.f - GetPhi(u2 * u2, p * p);
		}
		if (anisotropy > 0.f)
			phi += M_PI * .5f;
		H = Vector(sintheta * cosf(phi), sintheta * sinf(phi), costheta);
		cosWH = Dot(wo, H);
		*wi = 2.f * cosWH * H - wo;
	}
	const float specPdf = SchlickZ(H.z) * SchlickA(H) /
		(8.f * M_PI * fabsf(cosWH));
	*pdf = fabsf(wi->z) * INV_TWOPI + specPdf;
	if (!(*pdf > 0.f))
		return false;
	if (pdfBack)
		*pdfBack = fabsf(wo.z) * INV_TWOPI + specPdf;

	*f_ = SWCSpectrum(0.f);
	// No need to check for the reverse flag as the BRDF is identical in
	// both cases
	f(tspack, *wi, wo, f_);
	return true;
}
float SchlickBRDF::Pdf(const TsPack *tspack, const Vector &wo,
	const Vector &wi) const
{
	if (!SameHemisphere(wo, wi))
		return 0.f;
	const Vector H(Normalize(wo + wi));
	return fabsf(wi.z) * INV_TWOPI + SchlickZ(fabsf(H.z)) * SchlickA(H) /
		(8.f * M_PI * AbsDot(wi, H));
}

