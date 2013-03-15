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

// lineartonemap.cpp*
#include "lineartonemap.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// EVOp Method Definitions
void EVOp::Map(vector<XYZColor> &xyz, u_int xRes, u_int yRes, float maxDisplayY) const 
{
	// read data from film
	const float gamma = luxGetParameterValue(LUX_FILM, LUX_FILM_TORGB_GAMMA);

	const u_int numPixels = xRes * yRes;

	float Y = 0.f;
	u_int nPixels = 0;
	for (u_int i = 0; i < numPixels; ++i) {
		if (xyz[i].Y() <= 0.f)
			continue;
		Y += xyz[i].Y();
		nPixels++;
	}
	Y = Y / max(1U, nPixels);

	if (Y <= 0.f)
		return;

	/*
	(fstop * fstop) / exposure = Y*sensitivity/K

	take K = 12.5

	(fstop * fstop) / exposure = Y * sensitivity / 12.5

	exposure = 12.5*(fstop * fstop) / Y * sensitivity

	*/

	// linear tonemap operation
	//float factor = (exposure / (fstop * fstop) * sensitivity / 10.f * powf(118.f / 255.f, gamma));
		
	// substitute exposure, fstop and sensitivity cancel out; collect constants
	const float factor = (1.25f / Y * powf(118.f / 255.f, gamma));

	for (u_int i = 0; i < numPixels; ++i)
		xyz[i] *= factor;
}
ToneMap * EVOp::CreateToneMap(const ParamSet &ps) {
	return new EVOp();
}

// LinearOp Method Definitions
void LinearOp::Map(vector<XYZColor> &xyz, u_int xRes, u_int yRes, float maxDisplayY) const
{
	const u_int numPixels = xRes * yRes;
	for (u_int i = 0; i < numPixels; ++i)
		xyz[i] *= factor;
}
ToneMap * LinearOp::CreateToneMap(const ParamSet &ps) {
	float sensitivity = ps.FindOneFloat("sensitivity", 100.f);
	float exposure = ps.FindOneFloat("exposure", 1.f / 1000.f);
	float fstop = ps.FindOneFloat("fstop", 2.8f);
	float gamma = ps.FindOneFloat("gamma", 2.2f);
	return new LinearOp(sensitivity, exposure, fstop, gamma);
}

static DynamicLoader::RegisterToneMap<EVOp> r1("autolinear");
static DynamicLoader::RegisterToneMap<LinearOp> r2("linear");
