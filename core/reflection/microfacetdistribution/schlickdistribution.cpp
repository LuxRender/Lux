/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
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

// schlickdistribution.cpp*
#include "schlickdistribution.h"

using namespace lux;

static float GetPhi(float a, float b)
{
	return M_PI * .5f * sqrtf(a * b / (1.f - a * (1.f - b)));
}

void SchlickDistribution::SampleH(float u1, float u2, Vector *wh, float *d, float *pdf) const
{
	u2 *= 4.f;
	const float cos2Theta = u1 / (roughness * (1 - u1) + u1);
	const float cosTheta = sqrtf(cos2Theta);
	const float sinTheta = sqrtf(1.f - cos2Theta);
	const float p = 1.f - fabsf(anisotropy);
	float phi;
	if (u2 < 1.f) {
		phi = GetPhi(u2 * u2, p * p);
	} else if (u2 < 2.f) {
		u2 = 2.f - u2;
		phi = M_PI - GetPhi(u2 * u2, p * p);
	} else if (u2 < 3.f) {
		u2 -= 2.f;
		phi = M_PI + GetPhi(u2 * u2, p * p);
	} else {
		u2 = 4.f - u2;
		phi = M_PI * 2.f - GetPhi(u2 * u2, p * p);
	}
	if (anisotropy > 0.f)
		phi += M_PI * .5f;
	*wh = Vector(sinTheta * cosf(phi), sinTheta * sinf(phi), cosTheta);
	*d = SchlickZ(cosTheta) * SchlickA(*wh) * INV_PI;
	*pdf = *d;
}
