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

// orennayar.cpp*
#include "orennayar.h"
#include "spectrum.h"
#include "mc.h"

using namespace lux;

void OrenNayar::F(const SpectrumWavelengths &sw, const Vector &wo, 
	const Vector &wi, SWCSpectrum *const f_) const
{
	float sinthetai = SinTheta(wi);
	float sinthetao = SinTheta(wo);
	// Compute cosine term of Oren--Nayar model
	float maxcos = 0.f;
	if (sinthetai > 1e-4f && sinthetao > 1e-4f) {
		float sinphii = SinPhi(wi), cosphii = CosPhi(wi);
		float sinphio = SinPhi(wo), cosphio = CosPhi(wo);
		float dcos = cosphii * cosphio + sinphii * sinphio;
		maxcos = max(0.f, dcos);
	}
	f_->AddWeighted(INV_PI * fabsf(CosTheta(wo)) *
		(A + B * maxcos * sinthetao * sinthetai /
		max(fabsf(CosTheta(wi)), fabsf(CosTheta(wo)))), R);
}

bool OrenNayar::SampleF(const SpectrumWavelengths &sw, const Vector &wo,
	Vector *wi, float u1, float u2, SWCSpectrum *const f_, float *pdf,
	float *pdfBack, bool reverse) const
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
	float sinthetai = SinTheta(*wi);
	float sinthetao = SinTheta(wo);
	// Compute cosine term of Oren--Nayar model
	float maxcos = 0.f;
	if (sinthetai > 1e-4f && sinthetao > 1e-4f) {
		float sinphii = SinPhi(*wi), cosphii = CosPhi(*wi);
		float sinphio = SinPhi(wo), cosphio = CosPhi(wo);
		float dcos = cosphii * cosphio + sinphii * sinphio;
		maxcos = max(0.f, dcos);
	}
	*f_ = (A + B * maxcos * sinthetao * sinthetai /
		max(fabsf(CosTheta(*wi)), fabsf(CosTheta(wo)))) * R;
	if (!reverse)
		*f_ *= fabsf(wo.z / wi->z);
	return true;
}

