/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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
#include "luxrays/core/color/swcspectrum.h"
#include "luxrays/core/color/spectrumwavelengths.h"
#include "sampling.h"
#include "fresnel.h"
#include "volume.h"

#include "luxrays/utils/mc.h"

using namespace luxrays;
using namespace lux;

namespace lux
{

static RandomGenerator rng(1);

// BxDF Method Definitions
bool BxDF::SampleF(const SpectrumWavelengths &sw, const Vector &wo, Vector *wi,
	float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack,
	bool reverse) const
{
	// Cosine-sample the hemisphere, flipping the direction if necessary
	*wi = CosineSampleHemisphere(u1, u2);
	if (wo.z < 0.f)
		wi->z = -(wi->z);
	// wi may be in the tangent plane, which will 
	// fail the SameHemisphere test in Pdf()
	if (!SameHemisphere(wo, *wi)) 
		return false;
	*pdf = Pdf(sw, wo, *wi);
	if (pdfBack)
		*pdfBack = Pdf(sw, *wi, wo);
	*f = SWCSpectrum(0.f);
	if (reverse)
		F(sw, *wi, wo, f);
	else
		F(sw, wo, *wi, f);
	*f /= *pdf;
	return true;
}
float BxDF::Pdf(const SpectrumWavelengths &sw, const Vector &wo, const Vector &wi) const {
	return SameHemisphere(wo, wi) ? fabsf(wi.z) * INV_PI : 0.f;
}

SWCSpectrum BxDF::rho(const SpectrumWavelengths &sw, const Vector &w,
	u_int nSamples, float *samples) const
{
	if (!samples) {
		samples = static_cast<float *>(alloca(2 * nSamples * sizeof(float)));
		LatinHypercube(rng, samples, nSamples, 2);
	}
	Vector wi;
	float pdf;
	SWCSpectrum r(0.f);
	for (u_int i = 0; i < nSamples; ++i) {
		// Estimate one term of $\rho_{dh}$
		// SampleF will add the BxDF to r if the sampling is successfull
		SampleF(sw, w, &wi, samples[2 * i], samples[2 * i + 1],
			&r, &pdf, NULL, true);
	}
	return r / nSamples;
}
SWCSpectrum BxDF::rho(const SpectrumWavelengths &sw, u_int nSamples,
	float *samples) const
{
	if (!samples) {
		samples = static_cast<float *>(alloca(4 * nSamples * sizeof(float)));
		LatinHypercube(rng, samples, nSamples, 4);
	}
	SWCSpectrum r(0.f);
	for (u_int i = 0; i < nSamples; ++i) {
		// Estimate one term of $\rho_{hh}$
		const Vector wo(UniformSampleSphere(samples[4 * i], samples[4 * i + 1]));
		const float pdfo = INV_TWOPI * .5f;
		Vector wi;
		float pdfi = 0.f;
		SWCSpectrum f(0.f);
		if (SampleF(sw, wo, &wi, samples[4 * i + 2], samples[4 * i + 3],
			&f, &pdfi, NULL, true))
			r.AddWeighted(fabsf(wo.z) / pdfo, f);
	}
	return r / (M_PI * nSamples);
}
float BxDF::Weight(const SpectrumWavelengths &sw, const Vector &w) const
{
	return 1.f;
}
BSDF::BSDF(const DifferentialGeometry &dg, const Normal &ngeom,
	const Volume *ex, const Volume *in)
	: ng(ngeom), dgShading(dg), exterior(ex), interior(in)
{
	sn = Normalize(dgShading.dpdu);
	tn = Cross(dg.nn, sn);
	compParams = NULL; 
}

float BSDF::ApplyTransform(const Transform &transform) {
	ng = Normalize(transform * ng);
	dgShading *= transform;
	sn = Normalize(dgShading.dpdu);
	tn = Cross(dgShading.nn, sn);
	return fabsf(Dot(Cross(dgShading.dpdu, dgShading.dpdv),
		Vector(dgShading.nn)));
}

#define IFTYPE(T) if(type & T) stream << #T << "|"

std::ostream& operator <<(std::ostream& stream, const BxDFType& type) {
	stream << "BxDFType(";

	IFTYPE(BSDF_REFLECTION);
	IFTYPE(BSDF_TRANSMISSION);
	IFTYPE(BSDF_DIFFUSE);
	IFTYPE(BSDF_GLOSSY);
	IFTYPE(BSDF_SPECULAR);

	stream << ")";

	return stream;
}
}//namespace lux
