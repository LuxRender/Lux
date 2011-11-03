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
	BxDF *c, const Fresnel *cf, BSDF *b, const Volume *exterior, const Volume *interior)
	: BSDF(dgs, ngeom, exterior, interior), coating(c), fresnel(cf), base(b)
{
}
float SchlickBSDF::CoatingWeight(const SpectrumWavelengths &sw, const Vector &wo) const
{
	// ensures coating is never sampled less than half the time
	return 0.5f * (1.f + coating->Weight(sw, wo));
}
bool SchlickBSDF::SampleF(const SpectrumWavelengths &sw, const Vector &woW, Vector *wiW,
	float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
	BxDFType flags, BxDFType *sampledType, float *pdfBack,
	bool reverse) const
{
	Vector wo(WorldToLocal(woW)), wi;

	const float w_coating = CoatingWeight(sw, wo);
	const float w_base = 1.f - w_coating;

	float basePdf, basePdfBack;
	float coatingPdf, coatingPdfBack;

	SWCSpectrum baseF(0.f), coatingF(0.f);

	if (u3 < w_base) {
		u3 /= w_base;
		// Sample base layer
		if (!base->SampleF(sw, woW, wiW, u1, u2, u3, &baseF, &basePdf, flags, sampledType, &basePdfBack, reverse))
			return false;
		wi = WorldToLocal(*wiW);

		baseF *= basePdf;

		if (reverse)
			coating->F(sw, wi, wo, &coatingF);
		else
			coating->F(sw, wo, wi, &coatingF);

		coatingPdf = coating->Pdf(sw, wo, wi);
		if (pdfBack)
			coatingPdfBack = coating->Pdf(sw, wi, wo);
	} else {
		if (!coating->MatchesFlags(flags))
			return false;
		// Sample coating BxDF
		if (!coating->SampleF(sw, wo, &wi, u1, u2, &coatingF,
			&coatingPdf, &coatingPdfBack, reverse))
			return false;
		*wiW = LocalToWorld(wi);
		if (sampledType)
			*sampledType = coating->type;

		coatingF *= coatingPdf;

		if (reverse)
			baseF = base->F(sw, *wiW, woW, reverse, flags);
		else
			baseF = base->F(sw, woW, *wiW, reverse, flags);

		basePdf = base->Pdf(sw, woW, *wiW, flags);
		if (pdfBack)
			basePdfBack = base->Pdf(sw, woW, *wiW, flags);
	}

	const float w_coatingR = CoatingWeight(sw, wi);
	const float w_baseR = 1.f - w_coatingR;

	const float sideTest = Dot(*wiW, ng) / Dot(woW, ng);
	if (sideTest > 0.f)
		// ignore BTDFs
		flags = BxDFType(flags & ~BSDF_TRANSMISSION);
	else if (sideTest < 0.f)
		// ignore BRDFs
		flags = BxDFType(flags & ~BSDF_REFLECTION);
	else
		return false;

	if (sideTest > 0.f) {
		if (!coating->MatchesFlags(flags))
			return false;
		if (!reverse)
			// only affects top layer, base bsdf should do the same in it's F()
			coatingF *= fabsf(sideTest);

		// coating fresnel factor
		SWCSpectrum S;
		const Vector H(Normalize(wo + wi));
		const float u = AbsDot(wi, H);
		fresnel->Evaluate(sw, u, &S);

		// blend in base layer Schlick style
		// assumes coating bxdf takes fresnel factor S into account
		*f_ = coatingF + (SWCSpectrum(1.f) - S) * baseF;
	} else if (sideTest < 0.f) {
		// FIXME - we're ignoring the coating when entering from below
		*f_ = baseF;
	}

	// FIXME - different pdf's based on sideTest?
	*pdf = coatingPdf * w_coating + basePdf * w_base;
	if (pdfBack)
		*pdfBack = coatingPdfBack * w_coatingR + basePdfBack * w_baseR;

	*f_ /= *pdf;

	return true;
}
float SchlickBSDF::Pdf(const SpectrumWavelengths &sw, const Vector &woW, const Vector &wiW,
	BxDFType flags) const
{
	Vector wo(WorldToLocal(woW)), wi(WorldToLocal(wiW));

	const float w_coating = CoatingWeight(sw, wo);
	const float w_base = 1.f - w_coating;

	return w_base * base->Pdf(sw, woW, wiW, flags) + 
		w_coating *	(coating->MatchesFlags(flags) ? coating->Pdf(sw, wo, wi) : 0.f);
}
SWCSpectrum SchlickBSDF::F(const SpectrumWavelengths &sw, const Vector &woW,
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
		return SWCSpectrum(0.f);

	if (!coating->MatchesFlags(flags))
		if (sideTest > 0.f)
			return SWCSpectrum(0.f);
		else
			// FIXME
			return base->F(sw, woW, wiW, reverse, flags);

	Vector wi(WorldToLocal(wiW)), wo(WorldToLocal(woW));
	SWCSpectrum f_(0.f);

	if (sideTest > 0.f) {
		SWCSpectrum coatingF;
		coating->F(sw, wo, wi, &coatingF);
		if (!reverse)
			// only affects top layer, base bsdf should do the same in it's F()
			coatingF *= fabsf(sideTest);

		// coating fresnel factor
		SWCSpectrum S;
		const Vector H(Normalize(wo + wi));
		const float u = AbsDot(wi, H);
		fresnel->Evaluate(sw, u, &S);

		// blend in base layer Schlick style
		// assumes coating bxdf takes fresnel factor S into account
		f_ = coatingF + (SWCSpectrum(1.f) - S) * base->F(sw, woW, wiW, reverse, flags);

	} else if (sideTest < 0.f) {
		// FIXME - we're ignoring the coating when entering from below
		f_ = base->F(sw, woW, wiW, reverse, flags);
	} else {
		return SWCSpectrum(0.f);
	}

	return f_;
}
SWCSpectrum SchlickBSDF::rho(const SpectrumWavelengths &sw, BxDFType flags) const
{
	// TODO - proper implementation
	SWCSpectrum ret(0.f);
	if (coating->MatchesFlags(flags))
		ret += coating->rho(sw);
	ret += base->rho(sw, flags);
	return ret;
}
SWCSpectrum SchlickBSDF::rho(const SpectrumWavelengths &sw, const Vector &woW,
	BxDFType flags) const
{
	// TODO - proper implementation
	Vector wo(WorldToLocal(woW));
	SWCSpectrum ret(0.f);
	if (coating->MatchesFlags(flags))
		ret += coating->rho(sw, wo);
	ret += base->rho(sw, woW, flags);
	return ret;
}
