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

// contrast.cpp*
#include "contrast.h"
#include "luxrays/core/color/color.h"
#include "dynload.h"

using namespace lux;

// ContrastOp Method Definitions
void ContrastOp::Map(vector<XYZColor> &xyz, u_int xRes, u_int yRes,	float maxDisplayY) const 
{
	// Compute world adaptation luminance, _Ywa_
	float Ywa = 0.f;
	u_int nPixels = 0;
	for (u_int i = 0; i < xRes * yRes; ++i) {
		if (xyz[i].Y() <= 0) 
			continue;
		Ywa += logf(xyz[i].Y());
		nPixels++;
	}
	Ywa = expf(Ywa / max(1U, nPixels));
	// Compute contrast-preserving scalefactor, _s_
	float s = powf((1.219f + powf(displayAdaptationY, 0.4f)) /
		(1.219f + powf(Ywa, 0.4f)), 2.5f) / maxDisplayY;
	for (u_int i = 0; i < xRes*yRes; ++i)
		xyz[i] *= s;
}
ToneMap * ContrastOp::CreateToneMap(const ParamSet &ps) {
	float day = ps.FindOneFloat("ywa", 50.f);
	return new ContrastOp(day);
}

static DynamicLoader::RegisterToneMap<ContrastOp> r("contrast");
