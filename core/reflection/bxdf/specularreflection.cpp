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

// specularreflection.cpp*
#include "specularreflection.h"
#include "color.h"
#include "color.h"
#include "spectrum.h"
#include "spectrumwavelengths.h"
#include "mc.h"
#include "sampling.h"
#include "fresnel.h"
#include <stdarg.h>

using namespace lux;

//phase_diff(lambda, theta) = (4*PI*d/lambda)*sqrt(nf*nf-na*na*sin(theta)*sin(theta)) + PI

bool SpecularReflection::Sample_f(const TsPack *tspack, const Vector &wo,
	Vector *wi, float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack, bool reverse) const {
	// Compute perfect specular reflection direction
	float Rf = 1.f;

	// Calculate reflectance weight for thin film interference
	if(film > 0.f) {
		const float w = tspack->swl->SampleSingle();
		const float swo = SinTheta(wo);
		const float pd = (4*M_PI*film/w)*sqrtf(filmindex*filmindex-1.0f*1.0f*swo*swo) + M_PI;
		const float cpd = cosf(pd);
		Rf = cpd*cpd;
	}

	*wi = Vector(-wo.x, -wo.y, wo.z);
	*pdf = 1.f;
	if (pdfBack)
		*pdfBack = 1.f;
	*f = Rf * fresnel->Evaluate(tspack, CosTheta(wo));
	*f *= R;
	*f /= fabsf(CosTheta(wo));
	return true;
}
float SpecularReflection::Weight(const TsPack *tspack, const Vector &wo, bool reverse) const
{
	return fresnel->Evaluate(tspack, CosTheta(wo)).filter(tspack) / fabsf(CosTheta(wo));
}

bool ArchitecturalReflection::Sample_f(const TsPack *tspack, const Vector &wo,
	Vector *wi, float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack, bool reverse) const
{
	if (wo.z <= 0.f) {
		*pdf = 0.f;
		if (pdfBack)
			*pdfBack = 0.f;
		return false;
	}
	return SpecularReflection::Sample_f(tspack, wo, wi, u1, u2, f, pdf, pdfBack, reverse);
}
float ArchitecturalReflection::Weight(const TsPack *tspack, const Vector &wo, bool reverse) const
{
	if (wo.z <= 0.f)
		return 0.f;
	return SpecularReflection::Weight(tspack, wo, reverse);
}
