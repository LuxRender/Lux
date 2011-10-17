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

// bxdf.cpp*
#include "bxdf.h"
#include "spectrum.h"
#include "spectrumwavelengths.h"
#include "mc.h"
#include "sampling.h"
#include "fresnel.h"
#include "volume.h"

using namespace lux;

static RandomGenerator rng(1);

// BxDF Method Definitions
bool BxDF::SampleF(const SpectrumWavelengths &sw, const Vector &wo, Vector *wi,
	float u1, float u2, SWCSpectrum *const f_, float *pdf, float *pdfBack,
	bool reverse) const
{
	// Cosine-sample the hemisphere, flipping the direction if necessary
	*wi = CosineSampleHemisphere(u1, u2);
	if (wo.z < 0.f) wi->z *= -1.f;
	// wi may be in the tangent plane, which will 
	// fail the SameHemisphere test in Pdf()
	if (!SameHemisphere(wo, *wi)) 
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
float BxDF::Pdf(const SpectrumWavelengths &sw, const Vector &wo, const Vector &wi) const {
	return SameHemisphere(wo, wi) ? fabsf(wi.z) * INV_PI : 0.f;
}

SWCSpectrum BxDF::rho(const SpectrumWavelengths &sw, const Vector &w, u_int nSamples,
		float *samples) const {
	if (!samples) {
		samples =
			static_cast<float *>(alloca(2 * nSamples * sizeof(float)));
		LatinHypercube(rng, samples, nSamples, 2);
	}
	SWCSpectrum r(0.f);
	for (u_int i = 0; i < nSamples; ++i) {
		// Estimate one term of $\rho_{dh}$
		Vector wi;
		float pdf = 0.f;
		SWCSpectrum f_(0.f);
		if (SampleF(sw, w, &wi, samples[2*i], samples[2*i+1], &f_, &pdf,
			NULL, true) && pdf > 0.f)
			r += f_;
	}
	return r / nSamples;
}
SWCSpectrum BxDF::rho(const SpectrumWavelengths &sw, u_int nSamples, float *samples) const {
	if (!samples) {
		samples =
			static_cast<float *>(alloca(4 * nSamples * sizeof(float)));
		LatinHypercube(rng, samples, nSamples, 4);
	}
	SWCSpectrum r(0.f);
	for (u_int i = 0; i < nSamples; ++i) {
		// Estimate one term of $\rho_{hh}$
		Vector wo, wi;
		wo = UniformSampleHemisphere(samples[4*i], samples[4*i+1]);
		float pdf_o = INV_TWOPI, pdf_i = 0.f;
		SWCSpectrum f_(0.f);
		if (SampleF(sw, wo, &wi, samples[4*i+2], samples[4*i+3], &f_,
			&pdf_i, NULL, true) && pdf_i > 0.f)
			r.AddWeighted(fabsf(wo.z) / pdf_o, f_);
	}
	return r / (M_PI * nSamples);
}
float BxDF::Weight(const SpectrumWavelengths &sw, const Vector &wo) const
{
	return 1.f;
}
BSDF::BSDF(const DifferentialGeometry &dg, const Normal &ngeom,
	const Volume *ex, const Volume *in)
	: nn(dg.nn), ng(ngeom), dgShading(dg), exterior(ex), interior(in)
{
	sn = Normalize(dgShading.dpdu);
	tn = Cross(nn, sn);
	compParams = NULL; 
}
bool SingleBSDF::SampleF(const SpectrumWavelengths &sw, const Vector &woW, Vector *wiW,
	float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
	BxDFType flags, BxDFType *sampledType, float *pdfBack,
	bool reverse) const
{
	BOOST_ASSERT(bxdf); // NOBOOK
	if (!bxdf->MatchesFlags(flags))
		return false;
	// Sample chosen _BxDF_
	if (!bxdf->SampleF(sw, WorldToLocal(woW), wiW, u1, u2, f_,
		pdf, pdfBack, reverse))
		return false;
	if (sampledType)
		*sampledType = bxdf->type;
	*wiW = LocalToWorld(*wiW);
	const float sideTest = Dot(*wiW, ng) / Dot(woW, ng);
	if (sideTest > 0.f) {
		// ignore BTDFs
		if (bxdf->type & BSDF_TRANSMISSION)
			return false;
	} else if (sideTest < 0.f) {
		// ignore BRDFs
		if (bxdf->type & BSDF_REFLECTION)
			return false;
	} else
		return false;
	if (!reverse)
		*f_ *= fabsf(sideTest);
	return true;
}
float SingleBSDF::Pdf(const SpectrumWavelengths &sw, const Vector &woW,
	const Vector &wiW, BxDFType flags) const
{
	if (!bxdf->MatchesFlags(flags))
		return 0.f;
	return bxdf->Pdf(sw, WorldToLocal(woW), WorldToLocal(wiW));
}

SWCSpectrum SingleBSDF::F(const SpectrumWavelengths &sw, const Vector &woW,
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
		flags = static_cast<BxDFType>(0);
	if (!bxdf->MatchesFlags(flags))
		return SWCSpectrum(0.f);
	SWCSpectrum f_(0.f);
	bxdf->F(sw, WorldToLocal(woW), WorldToLocal(wiW), &f_);
	if (!reverse)
		f_ *= fabsf(sideTest);
	return f_;
}
SWCSpectrum SingleBSDF::rho(const SpectrumWavelengths &sw, BxDFType flags) const
{
	if (!bxdf->MatchesFlags(flags))
		return SWCSpectrum(0.f);
	return bxdf->rho(sw);
}
SWCSpectrum SingleBSDF::rho(const SpectrumWavelengths &sw, const Vector &woW,
	BxDFType flags) const
{
	if (!bxdf->MatchesFlags(flags))
		return SWCSpectrum(0.f);
	return bxdf->rho(sw, WorldToLocal(woW));
}
MultiBSDF::MultiBSDF(const DifferentialGeometry &dg, const Normal &ngeom,
	const Volume *exterior, const Volume *interior) :
	BSDF(dg, ngeom, exterior, interior)
{
	nBxDFs = 0;
}
bool MultiBSDF::SampleF(const SpectrumWavelengths &sw, const Vector &woW, Vector *wiW,
	float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
	BxDFType flags, BxDFType *sampledType, float *pdfBack,
	bool reverse) const
{
	float weights[MAX_BxDFS];
	// Choose which _BxDF_ to sample
	Vector wo(WorldToLocal(woW));
	u_int matchingComps = 0;
	float totalWeight = 0.f;
	for (u_int i = 0; i < nBxDFs; ++i) {
		if (bxdfs[i]->MatchesFlags(flags)) {
			weights[i] = bxdfs[i]->Weight(sw, wo);
			totalWeight += weights[i];
			++matchingComps;
		} else
			weights[i] = 0.f;
	}
	if (matchingComps == 0 || !(totalWeight > 0.f)) {
		*pdf = 0.f;
		if (pdfBack)
			*pdfBack = 0.f;
		return false;
	}
	u3 *= totalWeight;
	u_int which = 0;
	for (u_int i = 0; i < nBxDFs; ++i) {
		if (weights[i] > 0.f) {
			which = i;
			u3 -= weights[i];
			if (u3 < 0.f) {
				break;
			}
		}
	}
	BxDF *bxdf = bxdfs[which];
	BOOST_ASSERT(bxdf); // NOBOOK
	// Sample chosen _BxDF_
	Vector wi;
	if (!bxdf->SampleF(sw, wo, &wi, u1, u2, f_, pdf, pdfBack, reverse))
		return false;
	if (sampledType)
		*sampledType = bxdf->type;
	*wiW = LocalToWorld(wi);
	// Compute overall PDF with all matching _BxDF_s
	// Compute value of BSDF for sampled direction
	const float sideTest = Dot(*wiW, ng) / Dot(woW, ng);
	BxDFType flags2;
	if (sideTest > 0.f)
		// ignore BTDFs
		flags2 = BxDFType(flags & ~BSDF_TRANSMISSION);
	else if (sideTest < 0.f)
		// ignore BRDFs
		flags2 = BxDFType(flags & ~BSDF_REFLECTION);
	else
		return false;
	if (!bxdf->MatchesFlags(flags2))
		return false;
	if (!(bxdf->type & BSDF_SPECULAR) && matchingComps > 1 && !isinf(*pdf)) {
		*f_ *= *pdf;
		*pdf *= weights[which];
		float totalWeightR = bxdfs[which]->Weight(sw, wi);
		if (pdfBack)
			*pdfBack *= totalWeightR;
		for (u_int i = 0; i < nBxDFs; ++i) {
			if (i== which || !bxdfs[i]->MatchesFlags(flags))
				continue;
			if (bxdfs[i]->MatchesFlags(flags2)) {
				if (reverse)
					bxdfs[i]->F(sw, wi, wo, f_);
				else
					bxdfs[i]->F(sw, wo, wi, f_);
			}
			*pdf += bxdfs[i]->Pdf(sw, wo, wi) * weights[i];
			if (pdfBack) {
				const float weightR = bxdfs[i]->Weight(sw, wi);
				*pdfBack += bxdfs[i]->Pdf(sw, wi, wo) * weightR;
				totalWeightR += weightR;
			}
		}
		*pdf /= totalWeight;
		*f_ /= *pdf;
		if (pdfBack)
			*pdfBack /= totalWeightR;
	} else {
		const float w = weights[which] / totalWeight;
		*pdf *= w;
		*f_ /= w;
		if (pdfBack && matchingComps > 1) {
			float totalWeightR = bxdfs[which]->Weight(sw, wi);
			*pdfBack *= totalWeightR;
			for (u_int i = 0; i < nBxDFs; ++i) {
				if (i== which || !bxdfs[i]->MatchesFlags(flags))
					continue;
				const float weightR = bxdfs[i]->Weight(sw, wi);
				*pdfBack += bxdfs[i]->Pdf(sw, wi, wo) * weightR;
				totalWeightR += weightR;
			}
			*pdfBack /= totalWeightR;
		}
	}
	if (!reverse)
		*f_ *= fabsf(sideTest);
	return true;
}
float MultiBSDF::Pdf(const SpectrumWavelengths &sw, const Vector &woW, const Vector &wiW,
	BxDFType flags) const
{
	Vector wo(WorldToLocal(woW)), wi(WorldToLocal(wiW));
	float pdf = 0.f;
	float totalWeight = 0.f;
	for (u_int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags)) {
			float weight = bxdfs[i]->Weight(sw, wo);
			pdf += bxdfs[i]->Pdf(sw, wo, wi) * weight;
			totalWeight += weight;
		}
	return totalWeight > 0.f ? pdf / totalWeight : 0.f;
}

SWCSpectrum MultiBSDF::F(const SpectrumWavelengths &sw, const Vector &woW,
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
		flags = static_cast<BxDFType>(0);
	Vector wi(WorldToLocal(wiW)), wo(WorldToLocal(woW));
	SWCSpectrum f_(0.f);
	for (u_int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags))
			bxdfs[i]->F(sw, wo, wi, &f_);
	if (!reverse)
		f_ *= fabsf(sideTest);
	return f_;
}
SWCSpectrum MultiBSDF::rho(const SpectrumWavelengths &sw, BxDFType flags) const
{
	SWCSpectrum ret(0.f);
	for (u_int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags))
			ret += bxdfs[i]->rho(sw);
	return ret;
}
SWCSpectrum MultiBSDF::rho(const SpectrumWavelengths &sw, const Vector &woW,
	BxDFType flags) const
{
	Vector wo(WorldToLocal(woW));
	SWCSpectrum ret(0.f);
	for (u_int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags))
			ret += bxdfs[i]->rho(sw, wo);
	return ret;
}

MixBSDF::MixBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
	const Volume *exterior, const Volume *interior) :
	BSDF(dgs, ngeom, exterior, interior), totalWeight(1.f)
{
	// totalWeight is initialized to 1 to avoid divisions by 0 when there
	// are no components in the mix
	nBSDFs = 0;
}
bool MixBSDF::SampleF(const SpectrumWavelengths &sw, const Vector &wo, Vector *wi,
	float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
	BxDFType flags, BxDFType *sampledType, float *pdfBack,
	bool reverse) const
{
	if (nBSDFs == 0)
		return false;
	u3 *= totalWeight;
	u_int which = 0;
	for (u_int i = 0; i < nBSDFs; ++i) {
		if (u3 < weights[i]) {
			which = i;
			break;
		}
		u3 -= weights[i];
	}
	BxDFType sType;
	if (!bsdfs[which]->SampleF(sw,
		wo, wi, u1, u2, u3 / weights[which], f_, pdf, flags,
		&sType, pdfBack, reverse))
		return false;
	*pdf *= weights[which];
	*f_ *= *pdf;
	if (pdfBack)
		*pdfBack *= weights[which];
	if (sType & BSDF_SPECULAR)
		flags = sType;
	for (u_int i = 0; i < nBSDFs; ++i) {
		if (i == which)
			continue;
		BSDF *bsdf = bsdfs[i];
		if (bsdf->NumComponents(flags) == 0)
			continue;
		if (reverse)
			f_->AddWeighted(weights[i],
				bsdf->F(sw, *wi, wo, true, flags));
		else
			f_->AddWeighted(weights[i],
				bsdf->F(sw, wo, *wi, false, flags));
		*pdf += weights[i] * bsdf->Pdf(sw, wo, *wi, flags);
		if (pdfBack)
			*pdfBack += weights[i] * bsdf->Pdf(sw, *wi, wo, flags);
	}
	*pdf /= totalWeight;
	if (pdfBack)
		*pdfBack /= totalWeight;
	*f_ /= *pdf;
	if (sampledType)
		*sampledType = sType;
	return true;
}
float MixBSDF::Pdf(const SpectrumWavelengths &sw, const Vector &wo, const Vector &wi,
	BxDFType flags) const
{
	float pdf = 0.f;
	for (u_int i = 0; i < nBSDFs; ++i)
		pdf += weights[i] * bsdfs[i]->Pdf(sw, wo, wi, flags);
	return pdf / totalWeight;
}
SWCSpectrum MixBSDF::F(const SpectrumWavelengths &sw, const Vector &woW,
	const Vector &wiW, bool reverse, BxDFType flags) const
{
	SWCSpectrum ff(0.f);
	for (u_int i = 0; i < nBSDFs; ++i) {
		ff.AddWeighted(weights[i],
			bsdfs[i]->F(sw, woW, wiW, reverse, flags));
	}
	return ff / totalWeight;
}
SWCSpectrum MixBSDF::rho(const SpectrumWavelengths &sw, BxDFType flags) const
{
	SWCSpectrum ret(0.f);
	for (u_int i = 0; i < nBSDFs; ++i)
		ret.AddWeighted(weights[i], bsdfs[i]->rho(sw, flags));
	ret /= totalWeight;
	return ret;
}
SWCSpectrum MixBSDF::rho(const SpectrumWavelengths &sw, const Vector &wo,
	BxDFType flags) const
{
	SWCSpectrum ret(0.f);
	for (u_int i = 0; i < nBSDFs; ++i)
		ret.AddWeighted(weights[i], bsdfs[i]->rho(sw, wo, flags));
	ret /= totalWeight;
	return ret;
}

LayeredBSDF::LayeredBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
	BxDF *c, const Fresnel *cf, BSDF *b, const Volume *exterior, const Volume *interior)
	: coating(c), fresnel(cf), base(b), BSDF(dgs, ngeom, exterior, interior)
{
}
float LayeredBSDF::CoatingWeight(const SpectrumWavelengths &sw, const Vector &wo) const
{
	// ensures coating is never sampled less than half the time
	return 0.5f * (1.f + coating->Weight(sw, wo));
}
bool LayeredBSDF::SampleF(const SpectrumWavelengths &sw, const Vector &woW, Vector *wiW,
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
float LayeredBSDF::Pdf(const SpectrumWavelengths &sw, const Vector &woW, const Vector &wiW,
	BxDFType flags) const
{
	Vector wo(WorldToLocal(woW)), wi(WorldToLocal(wiW));

	const float w_coating = CoatingWeight(sw, wo);
	const float w_base = 1.f - w_coating;

	return w_base * base->Pdf(sw, woW, wiW, flags) + 
		w_coating *	(coating->MatchesFlags(flags) ? coating->Pdf(sw, wo, wi) : 0.f);
}
SWCSpectrum LayeredBSDF::F(const SpectrumWavelengths &sw, const Vector &woW,
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
SWCSpectrum LayeredBSDF::rho(const SpectrumWavelengths &sw, BxDFType flags) const
{
	// TODO - proper implementation
	SWCSpectrum ret(0.f);
	if (coating->MatchesFlags(flags))
		ret += coating->rho(sw);
	ret += base->rho(sw, flags);
	return ret;
}
SWCSpectrum LayeredBSDF::rho(const SpectrumWavelengths &sw, const Vector &woW,
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
