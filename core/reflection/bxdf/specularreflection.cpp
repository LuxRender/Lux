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

// specularreflection.cpp*
#include "specularreflection.h"
#include "spectrum.h"
#include "spectrumwavelengths.h"
#include "fresnel.h"

using namespace lux;

bool SimpleSpecularReflection::SampleF(const SpectrumWavelengths &sw,
	const Vector &wo, Vector *wi, float u1, float u2, SWCSpectrum *const f_,
	float *pdf, float *pdfBack, bool reverse) const
{
	// Compute perfect specular reflection direction
	*wi = Vector(-wo.x, -wo.y, wo.z);
	*pdf = 1.f;
	if (pdfBack)
		*pdfBack = 1.f;
	fresnel->Evaluate(sw, CosTheta(wo), f_);
	return true;
}
float SimpleSpecularReflection::Weight(const SpectrumWavelengths &sw,
	const Vector &wo) const
{
	SWCSpectrum F;
	fresnel->Evaluate(sw, CosTheta(wo), &F);
	return F.Filter(sw);
}

bool SimpleArchitecturalReflection::SampleF(const SpectrumWavelengths &sw,
	const Vector &wo, Vector *wi, float u1, float u2,
	SWCSpectrum *const f_, float *pdf, float *pdfBack, bool reverse) const
{
	if (wo.z <= 0.f) {
		*pdf = 0.f;
		if (pdfBack)
			*pdfBack = 0.f;
		return false;
	}
	return SimpleSpecularReflection::SampleF(sw, wo, wi, u1, u2, f_,
		pdf, pdfBack, reverse);
}
float SimpleArchitecturalReflection::Weight(const SpectrumWavelengths &sw,
	const Vector &wo) const
{
	if (wo.z <= 0.f)
		return 0.f;
	return SimpleSpecularReflection::Weight(sw, wo);
}

// Compute reflectance weight for thin film interference with phase difference
inline void PhaseDifference(const SpectrumWavelengths &sw, const Vector &wo,
	float film, float filmindex, SWCSpectrum *const Pd)
{
	const float swo = SinTheta(wo);
	const float s = sqrtf(max(0.f, filmindex * filmindex - /*1.f * 1.f **/ swo * swo));
	for(int i = 0; i < WAVELENGTH_SAMPLES; ++i) {
		const float pd = (4.f * M_PI * film / sw.w[i]) * s + M_PI;
		const float cpd = cosf(pd);
		Pd->c[i] *= cpd*cpd;
	}
}

bool SpecularReflection::SampleF(const SpectrumWavelengths &sw,
	const Vector &wo, Vector *wi, float u1, float u2, SWCSpectrum *const f_,
	float *pdf, float *pdfBack, bool reverse) const
{
	// Compute perfect specular reflection direction
	if (!SimpleSpecularReflection::SampleF(sw, wo, wi, u1, u2, f_,
		pdf, pdfBack, reverse))
		return false;
	if (film > 0.f)
		PhaseDifference(sw, wo, film, filmindex, f_);
	*f_ *= R;
	return true;
}

bool ArchitecturalReflection::SampleF(const SpectrumWavelengths &sw,
	const Vector &wo, Vector *wi, float u1, float u2, SWCSpectrum *const f_,
	float *pdf, float *pdfBack, bool reverse) const
{
	if (wo.z <= 0.f) {
		*pdf = 0.f;
		if (pdfBack)
			*pdfBack = 0.f;
		return false;
	}
	return SpecularReflection::SampleF(sw, wo, wi, u1, u2, f_, pdf,
		pdfBack, reverse);
}
float ArchitecturalReflection::Weight(const SpectrumWavelengths &sw,
	const Vector &wo) const
{
	if (wo.z <= 0.f)
		return 0.f;
	return SpecularReflection::Weight(sw, wo);
}
