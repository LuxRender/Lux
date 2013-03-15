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

// anisotropic.cpp*
#include "anisotropic.h"
using luxrays::SphericalDirection;

using namespace lux;

void Anisotropic::SampleH(float u1, float u2, Vector *wh, float *d,
	float *pdf) const
{
	// Sample from first quadrant and remap to hemisphere to sample \wh
	float phi, cosTheta;
	if (u1 < .25f) {
		SampleFirstQuadrant(4.f * u1, u2, &phi, &cosTheta);
	} else if (u1 < .5f) {
		u1 = 4.f * (.5f - u1);
		SampleFirstQuadrant(u1, u2, &phi, &cosTheta);
		phi = M_PI - phi;
	} else if (u1 < .75f) {
		u1 = 4.f * (u1 - .5f);
		SampleFirstQuadrant(u1, u2, &phi, &cosTheta);
		phi += M_PI;
	} else {
		u1 = 4.f * (1.f - u1);
		SampleFirstQuadrant(u1, u2, &phi, &cosTheta);
		phi = 2.f * M_PI - phi;
	}
	const float sin2Theta = max(0.f, 1.f - cosTheta * cosTheta);
	const float sinTheta = sqrtf(sin2Theta);
	*wh = SphericalDirection(sinTheta, cosTheta, phi);
	// Compute PDF for \wi from Anisotropic distribution
	const float e = (ex * wh->x * wh->x + ey * wh->y * wh->y) / sin2Theta;
	const float f = INV_TWOPI * powf(cosTheta, e);
	*d = sqrtf((ex + 2.f) * (ey + 2.f)) * f;
	*pdf = sqrtf((ex + 1.f) * (ey + 1.f)) * f;
}
void Anisotropic::SampleFirstQuadrant(float u1, float u2,
	float *phi, float *cosTheta) const
{
	if (ex == ey)
		*phi = M_PI * u1 * 0.5f;
	else
		*phi = atanf(sqrtf((ex + 1.f)/(ey + 1.f)) *
			tanf(M_PI * u1 * 0.5f));
	const float cosPhi = cosf(*phi), sinPhi = sinf(*phi);
	*cosTheta = powf(u2, 1.f / (ex * cosPhi * cosPhi +
		ey * sinPhi * sinPhi + 1.f));
}
float Anisotropic::Pdf(const Vector &wh) const
{
	// Compute PDF for \wh from Anisotropic distribution
	const float cosTheta = fabsf(wh.z);
	const float e = (ex * wh.x * wh.x + ey * wh.y * wh.y) /
		max(0.f, 1.f - cosTheta * cosTheta);
	return sqrtf((ex + 1.f) * (ey + 1.f)) * INV_TWOPI * powf(cosTheta, e);
}

