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

// bxdf.cpp*
#include "bxdf.h"
#include "color.h"
#include "spectrum.h"
#include "mc.h"
#include "sampling.h"
#include <stdarg.h>

using namespace lux;

// BxDF Method Definitions
SWCSpectrum BRDFToBTDF::f(const Vector &wo,
                       const Vector &wi) const {
	return brdf->f(wo, otherHemisphere(wi));
}
SWCSpectrum BRDFToBTDF::Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf) const {
	SWCSpectrum f = brdf->Sample_f(wo, wi, u1, u2, pdf);
	*wi = otherHemisphere(*wi);
	return f;
}

SWCSpectrum BxDF::Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf) const {
	// Cosine-sample the hemisphere, flipping the direction if necessary
	*wi = CosineSampleHemisphere(u1, u2);
	if (wo.z < 0.) wi->z *= -1.f;
	*pdf = Pdf(wo, *wi);
	return f(wo, *wi);
}
float BxDF::Pdf(const Vector &wo, const Vector &wi) const {
	return
		SameHemisphere(wo, wi) ? fabsf(wi.z) * INV_PI : 0.f;
}
float BRDFToBTDF::Pdf(const Vector &wo,
		const Vector &wi) const {
	return brdf->Pdf(wo, -wi);
}

SWCSpectrum BxDF::rho(const Vector &w, int nSamples,
		float *samples) const {
	if (!samples) {
		samples =
			(float *)alloca(2 * nSamples * sizeof(float));
		LatinHypercube(samples, nSamples, 2);
	}
	SWCSpectrum r = 0.;
	for (int i = 0; i < nSamples; ++i) {
		// Estimate one term of $\rho_{dh}$
		Vector wi;
		float pdf = 0.f;
		SWCSpectrum f =
			Sample_f(w, &wi, samples[2*i], samples[2*i+1], &pdf);
		if (pdf > 0.) r += f * fabsf(wi.z) / pdf;
	}
	return r / nSamples;
}
SWCSpectrum BxDF::rho(int nSamples, float *samples) const {
	if (!samples) {
		samples =
			(float *)alloca(4 * nSamples * sizeof(float));
		LatinHypercube(samples, nSamples, 4);
	}
	SWCSpectrum r = 0.;
	for (int i = 0; i < nSamples; ++i) {
		// Estimate one term of $\rho_{hh}$
		Vector wo, wi;
		wo = UniformSampleHemisphere(samples[4*i], samples[4*i+1]);
		float pdf_o = INV_TWOPI, pdf_i = 0.f;
		SWCSpectrum f =
			Sample_f(wo, &wi, samples[4*i+2], samples[4*i+3],
				&pdf_i);
		if (pdf_i > 0.)
			r += f * fabsf(wi.z * wo.z) / (pdf_o * pdf_i);
	}
	return r / (M_PI*nSamples);
}
// BSDF Method Definitions
SWCSpectrum BSDF::Sample_f(const Vector &wo, Vector *wi, BxDFType flags,
	BxDFType *sampledType) const {
	float pdf;
	SWCSpectrum f = Sample_f(wo, wi, lux::random::floatValue(), lux::random::floatValue(),
		lux::random::floatValue(), &pdf, flags, sampledType);
	if (!f.Black() && pdf > 0.) f /= pdf;
	return f;
}
SWCSpectrum BSDF::Sample_f(const Vector &woW, Vector *wiW,
		float u1, float u2, float u3, float *pdf,
		BxDFType flags, BxDFType *sampledType) const {
	// Choose which _BxDF_ to sample
	int matchingComps = NumComponents(flags);
	if (matchingComps == 0) {
		*pdf = 0.f;
		return SWCSpectrum(0.f);
	}
	int which = min(Floor2Int(u3 * matchingComps),
		matchingComps-1);
	BxDF *bxdf = NULL;
	int count = which;
	for (int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags))
			if (count-- == 0) {
				bxdf = bxdfs[i];
				break;
			}
	BOOST_ASSERT(bxdf); // NOBOOK
	// Sample chosen _BxDF_
	Vector wi;
	Vector wo = WorldToLocal(woW);
	*pdf = 0.f;
	SWCSpectrum f = bxdf->Sample_f(wo, &wi, u1, u2, pdf);
	if (*pdf == 0.f) return 0.f;
	if (sampledType) *sampledType = bxdf->type;
	*wiW = LocalToWorld(wi);
	// Compute overall PDF with all matching _BxDF_s
	if (!(bxdf->type & BSDF_SPECULAR) && matchingComps > 1) {
		for (int i = 0; i < nBxDFs; ++i) {
			if (bxdfs[i] != bxdf &&
			    bxdfs[i]->MatchesFlags(flags))
				*pdf += bxdfs[i]->Pdf(wo, wi);
		}
	}
	if (matchingComps > 1) *pdf /= matchingComps;
	// Compute value of BSDF for sampled direction
	if (!(bxdf->type & BSDF_SPECULAR)) {
		f = 0.;
		if (Dot(*wiW, ng) * Dot(woW, ng) > 0)
			// ignore BTDFs
			flags = BxDFType(flags & ~BSDF_TRANSMISSION);
		else
			// ignore BRDFs
			flags = BxDFType(flags & ~BSDF_REFLECTION);
		for (int i = 0; i < nBxDFs; ++i)
			if (bxdfs[i]->MatchesFlags(flags))
				f += bxdfs[i]->f(wo, wi);
	}
	return f;
}
float BSDF::Pdf(const Vector &woW, const Vector &wiW,
		BxDFType flags) const {
	if (nBxDFs == 0.) return 0.;
	Vector wo = WorldToLocal(woW), wi = WorldToLocal(wiW);
	float pdf = 0.f;
	int matchingComps = 0;
	for (int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags)) {
			++matchingComps;
			pdf += bxdfs[i]->Pdf(wo, wi);
		}
	return matchingComps > 0 ? pdf / matchingComps : 0.f;
}
BSDF::BSDF(const DifferentialGeometry &dg,
           const Normal &ngeom, float e)
	: dgShading(dg), eta(e) {
	ng = ngeom;
	nn = dgShading.nn;
	sn = Normalize(dgShading.dpdu);
	tn = Cross(nn, sn);
	nBxDFs = 0;
}
SWCSpectrum BSDF::f(const Vector &woW,
		const Vector &wiW, BxDFType flags) const {
	Vector wi = WorldToLocal(wiW), wo = WorldToLocal(woW);
	if (Dot(wiW, ng) * Dot(woW, ng) > 0)
		// ignore BTDFs
		flags = BxDFType(flags & ~BSDF_TRANSMISSION);
	else
		// ignore BRDFs
		flags = BxDFType(flags & ~BSDF_REFLECTION);
	SWCSpectrum f = 0.;
	for (int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags))
			f += bxdfs[i]->f(wo, wi);
	return f;
}
SWCSpectrum BSDF::rho(BxDFType flags) const {
	SWCSpectrum ret(0.);
	for (int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags))
			ret += bxdfs[i]->rho();
	return ret;
}
SWCSpectrum BSDF::rho(const Vector &wo, BxDFType flags) const {
	SWCSpectrum ret(0.);
	for (int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags))
			ret += bxdfs[i]->rho(wo);
	return ret;
}
//MemoryArena BSDF::arena;
boost::thread_specific_ptr<MemoryArena> BSDF::arena;

