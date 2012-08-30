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

/*
 * Reinhard Tonemapping (Indigo compatible) class
 *
 * Uses Indigo compatible parameters.
 *
 * 30/09/07 - Radiance - Initial Version
 */

#include "reinhard.h"
#include "color.h"
#include "dynload.h"

using namespace lux;

// ReinhardOp Method Definitions
ReinhardOp::ReinhardOp(float prS, float poS, float b) 
	: pre_scale(prS), post_scale(poS), burn(b) {
}

// This is the implementation of equation (4) of this paper: http://www.cs.utah.edu/~reinhard/cdrom/tonemap.pdf
// TODO implement the local operator of equation (9) with reasonable speed
void ReinhardOp::Map(vector<XYZColor> &xyz, const vector<float> &alpha, 
	u_int xRes, u_int yRes, float maxDisplayY) const
{
	const float a = .1f; // alpha parameter
	const u_int numPixels = xRes * yRes;

	float Ywa = 0.f;
	// Compute world adaptation luminance, _Ywa_
	u_int nPixels = 0;
	for (u_int i = 0; i < xRes * yRes; ++i) {
		if (alpha[i] <= 0.f)
			continue;
		if (xyz[i].Y() > 0) 
			Ywa += xyz[i].Y();
		nPixels++;
	}
	Ywa = (Ywa > 0.f) ? Ywa / max(1U, nPixels) : 1.f;

	const float Yw = pre_scale * a * burn;
	const float invY2 = Yw > 0.f ? 1.f / (Yw * Yw) : 1e5f;
	const float pScale = post_scale * pre_scale * a / Ywa;

	for (u_int i = 0; i < numPixels; ++i) {
		const float ys = xyz[i].c[1];
		xyz[i] *= pScale * (1.f + ys * invY2) / (1.f + ys);
	}
}

ToneMap * ReinhardOp::CreateToneMap(const ParamSet &ps) {
	float pre_scale = ps.FindOneFloat("prescale", 1.f);
	float post_scale = ps.FindOneFloat("postscale", 1.f);
	float burn = ps.FindOneFloat("burn", 7.f);
	return new ReinhardOp(pre_scale, post_scale, burn);
}

static DynamicLoader::RegisterToneMap<ReinhardOp> r("reinhard");
