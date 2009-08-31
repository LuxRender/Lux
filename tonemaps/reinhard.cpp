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
#include "dynload.h"

using namespace lux;

// ReinhardOp Method Definitions
ReinhardOp::ReinhardOp(float prS, float poS, float b)
{
	pre_scale = prS;
	post_scale = poS;
	burn = b;
}

void ReinhardOp::Map(vector<XYZColor> &xyz, int xRes, int yRes, float maxDisplayY) const
{
	const float alpha = .1f;
	const int numPixels = xRes * yRes;

	float Ywa = 0.f;
	// Compute world adaptation luminance, _Ywa_
	for (int i = 0; i < numPixels; ++i)
		if (xyz[i].Y() > 0.f) Ywa += xyz[i].Y();
	Ywa /= numPixels;
	if (Ywa == 0.f)
		Ywa = 1.f;

	const float Yw = pre_scale * alpha * burn;
	const float invY2 = 1.f / (Yw * Yw);
	const float pScale = post_scale * pre_scale * alpha / Ywa;

	for (int i = 0; i < numPixels; ++i) {
		float ys = xyz[i].c[1];
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
