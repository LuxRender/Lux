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
#include "spectrum.h"
#include "spectrumwavelengths.h"
#include "mc.h"
#include "sampling.h"
#include <stdarg.h>

#include <boost/thread/tss.hpp>

using namespace lux;

extern boost::thread_specific_ptr<SpectrumWavelengths> thread_wavelengths;

SWCSpectrum SpecularTransmission::Sample_f(const Vector &wo,
	Vector *wi, float u1, float u2, float *pdf, float *pdfBack) const {
	// Figure out which $\eta$ is incident and which is transmitted
	const bool entering = CosTheta(wo) > 0.f;
	float ei = etai, et = etat;

	if(cb != 0.f) {
		// Handle dispersion using cauchy formula
		const float w = thread_wavelengths->SampleSingle();
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
		return 0.;
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
		SWCSpectrum F = fresnel.Evaluate(CosTheta(wo));
		return (SWCSpectrum(1.f) - F) * T / (fabsf(cost) * eta2);
	} else {
		SWCSpectrum F = fresnel.Evaluate(-cost);
		if (entering)
			return (SWCSpectrum(1.f) - F) * T / (fabsf(wi->z) * eta2);
		else
			return (SWCSpectrum(1.f) - F) * T * (eta2 / fabsf(wi->z));
	}
}
SWCSpectrum SpecularTransmission::f(const Vector &wo, const Vector &wi) const
{
	if (!(architectural && wi == -wo))
		return 0.f;
	// Figure out which $\eta$ is incident and which is transmitted
	const bool entering = CosTheta(wo) > 0.f;
	float ei = etai, et = etat;

	if(cb != 0.f) {
		// Handle dispersion using cauchy formula
		const float w = thread_wavelengths->SampleSingle();
		et += (cb * 1000000.f) / (w * w);
	}

	if (!entering)
		swap(ei, et);
	// Compute transmitted ray direction
	const float sini2 = SinTheta2(wo);
	const float eta = ei / et;
	const float eta2 = eta * eta;
	const float sint2 = eta2 * sini2;
	// Handle total internal reflection for transmission
	if (sint2 > 1.) {
		return 0.;
	}
	SWCSpectrum F = fresnel.Evaluate(CosTheta(wo));
	return (SWCSpectrum(1.f) - F) * T / (fabsf(CosTheta(wi)) * eta2);
}
