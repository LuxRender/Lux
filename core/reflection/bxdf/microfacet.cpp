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

MicrofacetReflection::MicrofacetReflection(const SWCSpectrum &reflectance,
	const Fresnel *fr, MicrofacetDistribution *d, bool oneS)
	: BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
	  R(reflectance), distribution(d), fresnel(fr), oneSided(oneS)
{
}

void MicrofacetReflection::f(const TsPack *tspack, const Vector &wo, 
	const Vector &wi, SWCSpectrum *const f_) const
{
	float cosThetaO = fabsf(CosTheta(wo));
	float cosThetaI = fabsf(CosTheta(wi));
	Vector wh = Normalize(wi + wo);
	if (wh.z < 0.f) {
		if (oneSided)
			return;
		wh = -wh;
	}
	float cosThetaH = Dot(wi, wh);
	SWCSpectrum F;
	fresnel->Evaluate(tspack, cosThetaH, &F);
	f_->AddWeighted(distribution->D(wh) * distribution->G(wo, wi, wh) /
		(4.f * cosThetaI * cosThetaO), R * F);
}

bool MicrofacetReflection::Sample_f(const TsPack *tspack, const Vector &wo,
	Vector *wi, float u1, float u2, SWCSpectrum *const f_, float *pdf, 
	float *pdfBack, bool reverse) const
{
	Vector wh;
	float d;
	distribution->SampleH(u1, u2, &wh, &d, pdf);
	if (wh.z < 0.f)
		wh = -wh;
	*wi = 2.f * Dot(wo, wh) * wh - wo;
	if ((oneSided && wo.z <= 0.f) || !SameHemisphere(wo, *wi)) 
		return false;

	const float cosThetaH = Dot(wo, wh);
	*pdf /= 4.f * fabsf(cosThetaH);
	if (pdfBack)
		*pdfBack = *pdf;
	SWCSpectrum F;
	fresnel->Evaluate(tspack, cosThetaH, &F);
	*f_ = (d * distribution->G(wo, *wi, wh) /
		(4.f * fabsf(wo.z) * fabsf(wi->z))) * (R * F);
	return true;
}
float MicrofacetReflection::Pdf(const TsPack *tspack, const Vector &wo,
	const Vector &wi) const
{
	Vector wh = Normalize(wi + wo);
	if (wh.z < 0.f) {
		if (oneSided)
			return 0.f;
		wh = -wh;
	}
	return distribution->Pdf(wh) / (4.f * AbsDot(wo, wh));
}

MicrofacetTransmission::MicrofacetTransmission(const SWCSpectrum &transmitance,
	const Fresnel *fr, MicrofacetDistribution *d)
	: BxDF(BxDFType(BSDF_TRANSMISSION | BSDF_GLOSSY)),
	  T(transmitance), distribution(d), fresnel(fr)
{
}

void MicrofacetTransmission::f(const TsPack *tspack, const Vector &wo, 
	const Vector &wi, SWCSpectrum *const f_) const
{
	const bool entering = CosTheta(wo) > 0.f;
	const float eta = entering ?
		1.f / fresnel->Index(tspack) : fresnel->Index(tspack);
	Vector wh(eta * wo + wi);
	if (wh.z < 0.f)
		wh = -wh;
	const float lengthSquared = wh.LengthSquared();
	wh /= sqrtf(lengthSquared);
	const float cosThetaO = fabsf(CosTheta(wo));
	const float cosThetaI = fabsf(CosTheta(wi));
	const float cosThetaIH = AbsDot(wi, wh);
	const float cosThetaOH = Dot(wo, wh);
	SWCSpectrum F;
	fresnel->Evaluate(tspack, cosThetaOH, &F);
	f_->AddWeighted(fabsf(cosThetaOH) * cosThetaIH * distribution->D(wh) *
		distribution->G(wo, wi, wh) /
		(cosThetaI * cosThetaO * lengthSquared),
		T * (SWCSpectrum(1.f) - F));
}

bool MicrofacetTransmission::Sample_f(const TsPack *tspack, const Vector &wo,
	Vector *wi, float u1, float u2, SWCSpectrum *const f_, float *pdf, 
	float *pdfBack, bool reverse) const
{
	Vector wh;
	float d;
	distribution->SampleH(u1, u2, &wh, &d, pdf);
	if (wh.z < 0.f)
		wh = -wh;
	const bool entering = CosTheta(wo) > 0.f;
	const float eta = entering ?
		1.f / fresnel->Index(tspack) : fresnel->Index(tspack);
	const float cosThetaOH = Dot(wo, wh);
	const float sinThetaIH2 = eta * eta * max(0.f,
		1.f - cosThetaOH * cosThetaOH);
	if (sinThetaIH2 >= 1.f)
		return false;
	float cosThetaIH = sqrtf(1.f - sinThetaIH2);
	if (entering)
		cosThetaIH = -cosThetaIH;
	const float length = eta * cosThetaOH + cosThetaIH;
	*wi = length * wh - eta * wo;
	const float lengthSquared = length * length;
	if (pdfBack)
		*pdfBack = *pdf * fabsf(cosThetaOH) * eta * eta / lengthSquared;
	*pdf *= fabsf(cosThetaIH) / lengthSquared;

	SWCSpectrum F;
	if (reverse)
		fresnel->Evaluate(tspack, cosThetaIH, &F);
	else
		fresnel->Evaluate(tspack, cosThetaOH, &F);
	*f_ = (fabsf(cosThetaOH * cosThetaIH / (CosTheta(wo) * CosTheta(*wi) *
		lengthSquared)) * d * distribution->G(*wi, wo, wh)) *
		(T * (SWCSpectrum(1.f) - F));
	return true;
}
float MicrofacetTransmission::Pdf(const TsPack *tspack, const Vector &wo,
	const Vector &wi) const
{
	if (SameHemisphere(wo, wi))
		return 0.f;
	const bool entering = CosTheta(wo) > 0.f;
	const float eta = entering ?
		1.f / fresnel->Index(tspack) : fresnel->Index(tspack);
	Vector wh(eta * wo + wi);
	if (wh.z < 0.f)
		wh = -wh;
	const float lengthSquared = wh.LengthSquared();
	wh /= sqrtf(lengthSquared);
	const float cosThetaIH = AbsDot(wi, wh);
	return distribution->Pdf(wh) * cosThetaIH / lengthSquared;
}
