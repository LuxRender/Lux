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

// speculartransmission.cpp*
#include "speculartransmission.h"
#include "color.h"
#include "color.h"
#include "spectrum.h"
#include "spectrumwavelengths.h"
#include "mc.h"
#include "sampling.h"
#include <stdarg.h>

using namespace lux;

bool SpecularTransmission::Sample_f(const TsPack *tspack, const Vector &wo,
	Vector *wi, float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack, bool reverse) const {
	// Figure out which $\eta$ is incident and which is transmitted
	const bool entering = CosTheta(wo) > 0.f;
	float ei = etai, et = etat;

	if(cb != 0.f) {
		// Handle dispersion using cauchy formula
		const float w = tspack->swl->SampleSingle();
		et += (cb * 1000000.f) / (w * w);
	}

	if (!entering && !architectural)
		swap(ei, et);
	// Compute transmitted ray direction
	const float sini2 = SinTheta2(wo);
	const float eta = ei / et;
	const float eta2 = eta * eta;
	const float sint2 = eta2 * sini2;
	// Handle total internal reflection for transmission
	if (sint2 > 1.) {
		*pdf = 0.f;
		if (pdfBack)
			*pdfBack = 0.f;
		return false;
	}
	float cost = sqrtf(max(0.f, 1.f - sint2));
	if (entering) cost = -cost;
	if (architectural)
		*wi = -wo;
	else
		*wi = Vector(-eta * wo.x, -eta * wo.y, cost);
	*pdf = 1.f;
	if (pdfBack)
		*pdfBack = 1.f;
	if (!architectural) {
		if (reverse) {
			SWCSpectrum F = fresnel.Evaluate(tspack, cost);
			*f = (SWCSpectrum(1.f) - F) * T * (eta2 / fabsf(cost));
		} else {
			SWCSpectrum F = fresnel.Evaluate(tspack, CosTheta(wo));
			*f = (SWCSpectrum(1.f) - F) * T / (fabsf(cost) * eta2);
		}
	} else {
		if (reverse) {
			if (entering) {
				SWCSpectrum F = fresnel.Evaluate(tspack, -cost);
				*f = (SWCSpectrum(1.f) - F) * T * (eta2 / fabsf(wi->z));
			} else {
				SWCSpectrum F = fresnel.Evaluate(tspack, -CosTheta(wo));
				*f = (SWCSpectrum(1.f) - F) * T / (fabsf(wi->z) * eta2);
			}
		} else {
			if (entering) {
				SWCSpectrum F = fresnel.Evaluate(tspack, CosTheta(wo));
				*f = (SWCSpectrum(1.f) - F) * T / (fabsf(wi->z) * eta2);
			} else {
				SWCSpectrum F = fresnel.Evaluate(tspack, -cost);
				*f = (SWCSpectrum(1.f) - F) * T * (eta2 / fabsf(wi->z));
			}
		}
	}
	return true;
}
float SpecularTransmission::Weight(const TsPack *tspack, const Vector &wo, bool reverse) const
{
	// Figure out which $\eta$ is incident and which is transmitted
	const bool entering = CosTheta(wo) > 0.f;
	float ei = etai, et = etat;

	if(cb != 0.f) {
		// Handle dispersion using cauchy formula
		const float w = tspack->swl->w[tspack->swl->single_w];
		et += (cb * 1000000.f) / (w * w);
	}

	if (!entering && !architectural)
		swap(ei, et);
	// Compute transmitted ray direction
	const float sini2 = SinTheta2(wo);
	const float eta = ei / et;
	const float eta2 = eta * eta;
	const float sint2 = eta2 * sini2;
	// Handle total internal reflection for transmission
	if (sint2 > 1.f)
		return 0.f;
	float cost = sqrtf(max(0.f, 1.f - sint2));
	if (entering) cost = -cost;
	float F, factor;
	bool single = tspack->swl->single;
	tspack->swl->single = true;
	if (!architectural) {
		if (reverse) {
			F = fresnel.Evaluate(tspack, cost).filter(tspack);
			factor = eta2 / fabsf(cost);
		} else {
			F = fresnel.Evaluate(tspack, CosTheta(wo)).filter(tspack);
			factor = 1.f / (fabsf(cost) * eta2);
		}
	} else {
		if (reverse) {
			if (entering) {
				F = fresnel.Evaluate(tspack, -cost).filter(tspack);
				factor = eta2 / fabsf(wo.z);
			} else {
				F = fresnel.Evaluate(tspack, -CosTheta(wo)).filter(tspack);
				factor = 1.f / (fabsf(wo.z) * eta2);
			}
		} else {
			if (entering) {
				F = fresnel.Evaluate(tspack, CosTheta(wo)).filter(tspack);
				factor = 1.f / (fabsf(wo.z) * eta2);
			} else {
				F = fresnel.Evaluate(tspack, -cost).filter(tspack);
				factor = eta2 / fabsf(wo.z);
			}
		}
	}
	tspack->swl->single = single;
	return (1.f - F) * factor;
}
void SpecularTransmission::f(const TsPack *tspack, const Vector &wo, 
							 const Vector &wi, SWCSpectrum *const f) const {
	if (!(architectural && wi == -wo))
		return;
	// Figure out which $\eta$ is incident and which is transmitted
	const bool entering = CosTheta(wo) > 0.f;
	float ei = etai, et = etat;

	if(cb != 0.f) {
		// Handle dispersion using cauchy formula
		const float w = tspack->swl->SampleSingle();
		et += (cb * 1000000.f) / (w * w);
	}

	// Compute transmitted ray direction
	const float sini2 = SinTheta2(wo);
	const float eta = ei / et;
	const float eta2 = eta * eta;
	const float sint2 = eta2 * sini2;
	// Handle total internal reflection for transmission
	if (sint2 > 1.)
		return;	
	float cost = sqrtf(max(0.f, 1.f - sint2));
	if (entering) cost = -cost;
	SWCSpectrum F = fresnel.Evaluate(tspack, -cost);
	if (entering)
		f->AddWeighted(1.f / (fabsf(wi.z) * eta2), (SWCSpectrum(1.f) - F) * T);
	else
		f->AddWeighted(eta2 / fabsf(wi.z), (SWCSpectrum(1.f) - F) * T);
}
