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

// blinn.cpp*
#include "blinn.h"
#include "color.h"
#include "spectrum.h"
#include "mc.h"
#include "sampling.h"
#include <stdarg.h>

using namespace lux;

void Blinn::Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf) const {
	// Compute sampled half-angle vector $\wh$ for Blinn distribution
	float costheta = powf(u1, 1.f / (exponent+2.0f));
	float sintheta = sqrtf(max(0.f, 1.f - costheta*costheta));
	float phi = u2 * 2.f * M_PI;
	Vector H = SphericalDirection(sintheta, costheta, phi);
	if (!SameHemisphere(wo, H)) H.z *= -1.f;
	// Compute incident direction by reflecting about $\wh$
	*wi = -wo + 2.f * Dot(wo, H) * H;
	// Compute PDF for \wi from Blinn distribution
	float blinn_pdf = ((exponent + 2.f) *
	                   powf(costheta, exponent)) /
		(2.f * M_PI * 4.f * Dot(wo, H));

	*pdf = blinn_pdf;
}
float Blinn::Pdf(const Vector &wo, const Vector &wi) const {
	Vector H = Normalize(wo + wi);
	float costheta = fabsf(H.z);
	// Compute PDF for \wi from Blinn distribution
	float blinn_pdf = ((exponent + 2.f) *
	                   powf(costheta, exponent)) /
		(2.f * M_PI * 4.f * Dot(wo, H));
	return blinn_pdf;
}

