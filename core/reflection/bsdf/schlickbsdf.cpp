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

// schlickbsdf.cpp*
#include "schlickbsdf.h"
#include "spectrum.h"
#include "fresnel.h"

using namespace lux;

SchlickBSDF::SchlickBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
	const Fresnel *cf, const MicrofacetDistribution *cd, bool mb, BSDF *b, 
	const Volume *exterior, const Volume *interior)
	: BSDF(dgs, ngeom, exterior, interior), coatingType(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
	fresnel(cf), distribution(cd), multibounce(mb), base(b)
{
}
float SchlickBSDF::CoatingWeight(const SpectrumWavelengths &sw, const Vector &wo) const
{
	// No sampling on the back face
	if (!(wo.z > 0.f))
		return 0.f;
	// approximate H by using reflection direction for wi
	const float u = fabsf(CosTheta(wo));

	SWCSpectrum S(0.f);
	fresnel->Evaluate(sw, u, &S);

	// ensures coating is never sampled less than half the time
	// unless we are on the back face
	return !(wo.z > 0.f) ? 0.f : 0.5f * (1.f + S.Filter(sw));
}
void SchlickBSDF::CoatingF(const SpectrumWavelengths &sw, const Vector &wo, 
	 const Vector &wi, SWCSpectrum *const f_) const
{
	// No sampling on the back face
	if (!(wo.z > 0.f) || !(wi.z > 0.f))
		return;
	const float coso = fabsf(CosTheta(wo));
	const float cosi = fabsf(CosTheta(wi));

	const Vector wh(Normalize(wo + wi));
	const float u = AbsDot(wi, wh);
	SWCSpectrum S;
	fresnel->Evaluate(sw, u, &S);

	const float G = distribution->G(wo, wi, wh);
	// Multibounce - alternative with interreflection in the coating creases
	const float factor = distribution->D(wh) * G / (4.f * cosi) + 
		(multibounce ? coso * Clamp((1.f - G) / (4.f * cosi * coso), 0.f, 1.f) : 0.f);
	f_->AddWeighted(factor, S);
}
bool SchlickBSDF::CoatingSampleF(const SpectrumWavelengths &sw, const Vector &wo,
	Vector *wi, float u1, float u2, SWCSpectrum *const f_, float *pdf, 
	float *pdfBack, bool reverse) const
{
	// No sampling on the back face
	if (!(wo.z > 0.f))
		return false;
	Vector wh;
	float d, specPdf;
	distribution->SampleH(u1, u2, &wh, &d, &specPdf);
	const float cosWH = Dot(wo, wh);
	*wi = 2.f * cosWH * wh - wo;

	if (!(wi->z > 0.f))
		return false;

	const float coso = fabsf(CosTheta(wo));
	const float cosi = fabsf(CosTheta(*wi));

	*pdf = specPdf / (4.f * coso);
	if (!(*pdf > 0.f))
		return false;
	if (pdfBack)
		*pdfBack = specPdf / (4.f * cosi);

	fresnel->Evaluate(sw, cosWH, f_);

	const float G = distribution->G(wo, *wi, wh);
	if (reverse)
		//CoatingF(sw, *wi, wo, f_);
		*f_ *= (d * G / (4.f * coso) + 
				(multibounce ? cosi * Clamp((1.f - G) / (4.f * coso * cosi), 0.f, 1.f) : 0.f)) / *pdf;
	else
		//CoatingF(sw, wo, *wi, f_);
		*f_ *= (d * G / (4.f * cosi) + 
				(multibounce ? coso * Clamp((1.f - G) / (4.f * cosi * coso), 0.f, 1.f) : 0.f)) / *pdf;
	return true;
}
float SchlickBSDF::CoatingPdf(const SpectrumWavelengths &sw, const Vector &wo,
	const Vector &wi) const
{
	// No sampling on the back face
	if (!(wo.z > 0.f) || !(wi.z > 0.f))
		return 0.f;
	const Vector wh(Normalize(wo + wi));
	return distribution->Pdf(wh) / (4.f * CosTheta(wo));
}
bool SchlickBSDF::SampleF(const SpectrumWavelengths &sw, const Vector &woW, Vector *wiW,
	float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
	BxDFType flags, BxDFType *sampledType, float *pdfBack,
	bool reverse) const
{
	Vector wo(WorldToLocal(woW)), wi;

	const float wCoating = !CoatingMatchesFlags(flags) ?
		0.f : CoatingWeight(sw, wo);
	const float wBase = 1.f - wCoating;

	float basePdf, basePdfBack;
	float coatingPdf, coatingPdfBack;

	SWCSpectrum baseF(0.f), coatingF(0.f);

	if (u3 < wBase) {
		u3 /= wBase;
		// Sample base layer
		BxDFType sType;
		if (!base->SampleF(sw, woW, wiW, u1, u2, u3, &baseF, &basePdf, flags, &sType, &basePdfBack, reverse))
			return false;
		wi = WorldToLocal(*wiW);
		if (sampledType)
			*sampledType = sType;

		baseF *= basePdf;

		// Don't add the coating scattering if the base sampled
		// component is specular
		if (!(sType & BSDF_SPECULAR)) {
			if (reverse)
				CoatingF(sw, wi, wo, &coatingF);
			else
				CoatingF(sw, wo, wi, &coatingF);

			coatingPdf = CoatingPdf(sw, wo, wi);
			if (pdfBack)
				coatingPdfBack = CoatingPdf(sw, wi, wo);
		} else
			coatingPdf = coatingPdfBack = 0.f;
	} else {
		// Sample coating BxDF
		if (!CoatingSampleF(sw, wo, &wi, u1, u2, &coatingF,
			&coatingPdf, &coatingPdfBack, reverse))
			return false;
		*wiW = LocalToWorld(wi);
		if (sampledType)
			*sampledType = coatingType;

		coatingF *= coatingPdf;

		if (reverse)
			baseF = base->F(sw, *wiW, woW, reverse, flags);
		else
			baseF = base->F(sw, woW, *wiW, reverse, flags);

		basePdf = base->Pdf(sw, woW, *wiW, flags);
		if (pdfBack)
			basePdfBack = base->Pdf(sw, *wiW, woW, flags);
	}

	const float wCoatingR = !CoatingMatchesFlags(flags) ?
		0.f : CoatingWeight(sw, wi);
	const float wBaseR = 1.f - wCoatingR;

	const float sideTest = Dot(*wiW, ng) / Dot(woW, ng);
	if (sideTest > 0.f) {
		// Reflection
		if (!(Dot(woW, ng) > 0.f)) {
			// Back face reflection: no coating
			*f_ = baseF;
		} else {
			// Front face reflection: coating+base
			if (!reverse)
				// only affects top layer, base bsdf should do the same in it's F()
				coatingF *= fabsf(sideTest);

			// coating fresnel factor
			SWCSpectrum S;
			const Vector H(Normalize(wo + wi));
			const float u = AbsDot(wi, H);
			fresnel->Evaluate(sw, u, &S);

			// blend in base layer Schlick style
			// coatingF already takes fresnel factor S into account
			*f_ = coatingF + (SWCSpectrum(1.f) - S) * baseF;
		}
	} else if (sideTest < 0.f) {
		// Transmission
		// coating fresnel factor
		SWCSpectrum S;
		const Vector H(Normalize(Vector(wo.x + wi.x, wo.y + wi.y, wo.z - wi.z)));
		const float u = AbsDot(wo, H);
		fresnel->Evaluate(sw, u, &S);

		// filter base layer, the square root is just a heuristic
		// so that a sheet coated on both faces gets a filtering factor
		// of 1-S like a reflection
		*f_ = (SWCSpectrum(1.f) - S).Sqrt() * baseF;
	} else
		return false;

	*pdf = coatingPdf * wCoating + basePdf * wBase;
	if (pdfBack)
		*pdfBack = coatingPdfBack * wCoatingR + basePdfBack * wBaseR;

	*f_ /= *pdf;

	return true;
}
float SchlickBSDF::Pdf(const SpectrumWavelengths &sw, const Vector &woW, const Vector &wiW,
	BxDFType flags) const
{
	Vector wo(WorldToLocal(woW)), wi(WorldToLocal(wiW));

	const float wCoating = !CoatingMatchesFlags(flags) ?
		0.f : CoatingWeight(sw, wo);
	const float wBase = 1.f - wCoating;

	return wBase * base->Pdf(sw, woW, wiW, flags) + 
		wCoating * CoatingPdf(sw, wo, wi);
}
SWCSpectrum SchlickBSDF::F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse, BxDFType flags) const
{
	const float sideTest = Dot(wiW, ng) / Dot(woW, ng);

	const Vector wi(WorldToLocal(wiW)), wo(WorldToLocal(woW));

	if (sideTest > 0.f) {
		// Reflection
		// ignore BTDFs
		flags = BxDFType(flags & ~BSDF_TRANSMISSION);
		if (!(wo.z > 0.f)) {
			// Back face: no coating
			return base->F(sw, woW, wiW, reverse, flags);
		}
		// Front face: coating+base
		SWCSpectrum coatingF(0.f);
		if (CoatingMatchesFlags(flags)) {
			CoatingF(sw, wo, wi, &coatingF);
			if (!reverse)
				// only affects top layer, base bsdf should do the same in it's F()
				coatingF *= fabsf(sideTest);
		}

		// coating fresnel factor
		SWCSpectrum S;
		const Vector H(Normalize(wo + wi));
		const float u = AbsDot(wi, H);
		fresnel->Evaluate(sw, u, &S);

		// blend in base layer Schlick style
		// assumes coating bxdf takes fresnel factor S into account
		return coatingF + (SWCSpectrum(1.f) - S) * base->F(sw, woW, wiW, reverse, flags);

	} else if (sideTest < 0.f) {
		// Transmission
		// ignore BRDFs
		flags = BxDFType(flags & ~BSDF_REFLECTION);
		// coating fresnel factor
		SWCSpectrum S;
		const Vector H(Normalize(Vector(wo.x + wi.x, wo.y + wi.y, wo.z - wi.z)));
		const float u = AbsDot(wo, H);
		fresnel->Evaluate(sw, u, &S);

		// filter base layer, the square root is just a heuristic
		// so that a sheet coated on both faces gets a filtering factor
		// of 1-S like a reflection
		return (SWCSpectrum(1.f) - S).Sqrt() * base->F(sw, woW, wiW, reverse, flags);
	} else
		return SWCSpectrum(0.f);
}
SWCSpectrum SchlickBSDF::rho(const SpectrumWavelengths &sw, BxDFType flags) const
{
	SWCSpectrum ret(0.f);
	// TODO - proper implementation
//	if (CoatingMatchesFlags(flags))
//		ret += coating->rho(sw);
	ret += base->rho(sw, flags);
	return ret;
}
SWCSpectrum SchlickBSDF::rho(const SpectrumWavelengths &sw, const Vector &woW,
	BxDFType flags) const
{
	Vector wo(WorldToLocal(woW));
	SWCSpectrum ret(0.f);
	// TODO - proper implementation
//	if (CoatingMatchesFlags(flags))
//		ret += Coating->rho(sw, wo);
	ret += base->rho(sw, woW, flags);
	return ret;
}
