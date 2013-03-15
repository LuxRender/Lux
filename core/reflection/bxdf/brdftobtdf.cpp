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

// brdftobtdf.cpp*
#include "brdftobtdf.h"
#include "spectrum.h"
#include "spectrumwavelengths.h"

using namespace lux;

// BxDF Method Definitions
void BRDFToBTDF::F(const SpectrumWavelengths &sw, const Vector &wo,
	const Vector &wi, SWCSpectrum *const f_) const
{
	if (etai == etat) {
		brdf->F(sw, wo, otherHemisphere(wi), f_);
		return;
	}
	// Figure out which $\eta$ is incident and which is transmitted
	const bool entering = CosTheta(wo) > 0.f;
	float ei = etai, et = etat;

	if (cb != 0.f) {
		// Handle dispersion using cauchy formula
		const float w = sw.SampleSingle();
		et += (cb * 1000000.f) / (w * w);
	}

	if (!entering)
		swap(ei, et);
	// Compute transmitted ray direction
	const float eta = ei / et;
	Vector H(Normalize(eta * wo + wi));
	const float cos1 = Dot(wo, H);
	if ((entering && cos1 < 0.f) || (!entering && cos1 > 0.f))
		H = -H;
	if (H.z < 0.f)
		return;
	Vector wiR(2.f * cos1 * H - wo);
	brdf->F(sw, wo, wiR, f_);
}
bool BRDFToBTDF::SampleF(const SpectrumWavelengths &sw, const Vector &wo, Vector *wi,
	float u1, float u2, SWCSpectrum *const f_, float *pdf, float *pdfBack,
	bool reverse) const
{
	if (etai == etat) {
		if (!brdf->SampleF(sw, wo, wi, u1, u2, f_, pdf, pdfBack, reverse))
			return false;
		*wi = otherHemisphere(*wi);
		return true;
	}
	if (!brdf->SampleF(sw, wo, wi, u1, u2, f_, pdf, pdfBack, reverse))
		return false;
	Vector H(Normalize(wo + *wi));
	if (H.z < 0.f)
		H = -H;
	const float cosi = Dot(wo, H);
	// Figure out which $\eta$ is incident and which is transmitted
	const bool entering = cosi > 0.f;
	float ei = etai, et = etat;

	if(cb != 0.f) {
		// Handle dispersion using cauchy formula
		const float w = sw.SampleSingle();
		et += (cb * 1000000.f) / (w * w);
	}

	if (!entering)
		swap(ei, et);
	// Compute transmitted ray direction
	const float sini2 = max(0.f, 1.f - cosi * cosi);
	const float eta = ei / et;
	const float eta2 = eta * eta;
	const float sint2 = eta2 * sini2;
	// Handle total internal reflection for transmission
	if (sint2 > 1.f) {
		*pdf = 0.f;
		if (pdfBack)
			*pdfBack = 0.f;
		return false;
	}
	float cost = sqrtf(max(0.f, 1.f - sint2));
	if (entering)
		cost = -cost;
	*wi = (cost + eta * cosi) * H - eta * wo;
	if (reverse)
		*f_ *= eta2;
	return true;
}

float BRDFToBTDF::Pdf(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi) const
{
	if (etai == etat)
		return brdf->Pdf(sw, wo, otherHemisphere(wi));
	// Figure out which $\eta$ is incident and which is transmitted
	const bool entering = CosTheta(wo) > 0.f;
	float ei = etai, et = etat;

	if(cb != 0.f) {
		// Handle dispersion using cauchy formula
		const float w = sw.SampleSingle();
		et += (cb * 1000000.f) / (w * w);
	}

	if (!entering)
		swap(ei, et);
	// Compute transmitted ray direction
	const float eta = ei / et;
	Vector H(Normalize(eta * wo + wi));
	const float cos1 = Dot(wo, H);
	if ((entering && cos1 < 0.f) || (!entering && cos1 > 0.f))
		H = -H;
	if (H.z < 0.f)
		return 0.f;
	return brdf->Pdf(sw, wo, 2.f * Dot(wo, H) * H - wo);
}
