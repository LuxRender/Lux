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

// speculartransmission.cpp*
#include "speculartransmission.h"
#include "spectrum.h"
#include "spectrumwavelengths.h"
#include "fresnel.h"

using namespace lux;

bool SimpleSpecularTransmission::SampleF(const SpectrumWavelengths &sw,
	const Vector &wo, Vector *wi, float u1, float u2, SWCSpectrum *const f_,
	float *pdf, float *pdfBack, bool reverse) const
{
	// Figure out which $\eta$ is incident and which is transmitted
	const bool entering = CosTheta(wo) > 0.f;

	// Handle dispersion using cauchy formula
	if (dispersive)
		sw.SampleSingle();

	// Compute transmitted ray direction
	const float sini2 = SinTheta2(wo);
	const float eta = (entering || architectural) ? 1.f / fresnel->Index(sw) : fresnel->Index(sw);
	const float eta2 = eta * eta;
	const float sint2 = eta2 * sini2;
	// Handle total internal reflection for transmission
	if (sint2 >= 1.f) {
		*f_ = 0.f;
		*pdf = 0.f;
		if (pdfBack)
			*pdfBack = 0.f;
		return false;
	}
	float cost = sqrtf(max(0.f, 1.f - sint2));
	if (entering)
		cost = -cost;
	if (architectural)
		*wi = -wo;
	else
		*wi = Vector(-eta * wo.x, -eta * wo.y, cost);
	*pdf = 1.f;
	if (pdfBack)
		*pdfBack = 1.f;
	SWCSpectrum F_;
	if (!architectural) {
		if (reverse) {
			fresnel->Evaluate(sw, cost, &F_);
			*f_ = (SWCSpectrum(1.f) - F_) * eta2;
		} else {
			fresnel->Evaluate(sw, CosTheta(wo), &F_);
			*f_ = (SWCSpectrum(1.f) - F_) * fabsf(wo.z / cost);
		}
	} else {
		if (reverse) {
			if (entering)
				F_ = SWCSpectrum(0.f);
			else
				fresnel->Evaluate(sw, -CosTheta(wo), &F_);
		} else {
			if (entering)
				fresnel->Evaluate(sw, CosTheta(wo), &F_);
			else
				F_ = SWCSpectrum(0.f);
		}
		F_ *= SWCSpectrum(1.f) + (SWCSpectrum(1.f) - F_) * (SWCSpectrum(1.f) - F_);
		*f_ = SWCSpectrum(1.f) - F_;
	}
	return true;
}
float SimpleSpecularTransmission::Weight(const SpectrumWavelengths &sw,
	const Vector &wo) const
{
	if (architectural && wo.z < 0.f)
		return 1.f;
	SWCSpectrum F_;
	fresnel->Evaluate(sw, CosTheta(wo), &F_);
	const float w = F_.Filter(sw);
	if (architectural)
		return 1.f - w * (1.f + (1.f - w) * (1.f - w));
	else
		return 1.f - w;
}
void SimpleSpecularTransmission::F(const SpectrumWavelengths &sw,
	const Vector &wo, const Vector &wi, SWCSpectrum *const f_) const
{
	if (!(architectural && Dot(wo, wi) <= -1.f + MachineEpsilon::E(1.f)))
		return;
	// Figure out which $\eta$ is incident and which is transmitted
	const bool entering = CosTheta(wo) > 0.f;

	// Handle dispersion using cauchy formula
	if (dispersive)
		sw.SampleSingle();

	// Compute transmitted ray direction
	const float sini2 = SinTheta2(wo);
	const float eta = 1.f / fresnel->Index(sw);
	const float eta2 = eta * eta;
	const float sint2 = eta2 * sini2;
	// Handle total internal reflection for transmission
	if (sint2 >= 1.f)
		return;	
	SWCSpectrum F_;
	if (entering)
		fresnel->Evaluate(sw, CosTheta(wo), &F_);
	else
		F_ = SWCSpectrum(0.f);
	F_ *= SWCSpectrum(1.f) + (SWCSpectrum(1.f) - F_) * (SWCSpectrum(1.f) - F_);
	*f_ += SWCSpectrum(1.f) - F_;
}

bool SpecularTransmission::SampleF(const SpectrumWavelengths &sw,
	const Vector &wo, Vector *wi, float u1, float u2, SWCSpectrum *const f_,
	float *pdf, float *pdfBack, bool reverse) const
{
	if (!SimpleSpecularTransmission::SampleF(sw, wo, wi, u1, u2, f_,
		pdf, pdfBack, reverse))
		return false;
	*f_ *= T;
	return true;
}
void SpecularTransmission::F(const SpectrumWavelengths &sw, const Vector &wo, 
	const Vector &wi, SWCSpectrum *const f_) const
{
	SWCSpectrum F_(0.f);
	SimpleSpecularTransmission::F(sw, wo, wi, &F_);
	*f_ += T * F_;
}
