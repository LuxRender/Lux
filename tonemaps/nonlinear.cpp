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

// nonlinear.cpp*
#include "nonlinear.h"
#include "dynload.h"

using namespace lux;

// NonLinearOp Method Definitions
void NonLinearOp::Map(vector<XYZColor> &xyz, u_int xRes, u_int yRes, float maxDisplayY) const {
	const u_int numPixels = xRes * yRes;
	float invY2;
	if (maxY <= 0.f) {
		// Compute world adaptation luminance, _Ywa_
		float Ywa = 0.f;
		for (u_int i = 0; i < numPixels; ++i)
			if (xyz[i].Y() > 0.f)
				Ywa += logf(xyz[i].Y());
		Ywa = expf(Ywa / (xRes * yRes));
		invY2 = 1.f / (Ywa * Ywa);
	} else
		invY2 = 1.f / (maxY * maxY);
	for (u_int i = 0; i < numPixels; ++i) {
		const float ys = xyz[i].c[1];
		xyz[i] *= (1.f + ys * invY2) / (1.f + ys);
	}
}
ToneMap* NonLinearOp::CreateToneMap(const ParamSet &ps) {
	float maxy = ps.FindOneFloat("maxY", 0.f);
	return new NonLinearOp(maxy);
}

static DynamicLoader::RegisterToneMap<NonLinearOp> r("nonlinear");
