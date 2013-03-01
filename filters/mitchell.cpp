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

// mitchell.cpp*
#include "mitchell.h"
#include "dynload.h"

using namespace lux;

// Mitchell Filter Method Definitions
float MitchellFilter::Evaluate(float x, float y) const {
	const float distance = sqrtf(x * x * invXWidth * invXWidth +
		y * y * invYWidth * invYWidth);
	if (!super)
		return Mitchell1D(distance);
	const float dist = distance / .6f;
	return a1 * Mitchell1D(dist - 2.f / 3.f) +
		a0 * Mitchell1D(dist) +
		a1 * Mitchell1D(dist + 2.f / 3.f);
}
Filter* MitchellFilter::CreateFilter(const ParamSet &ps) {
	// Find common filter parameters
	float xw = ps.FindOneFloat("xwidth", 2.);
	float yw = ps.FindOneFloat("ywidth", 2.);
	float B = ps.FindOneFloat("B", 1.f/3.f);
	float C = ps.FindOneFloat("C", 1.f/3.f);
	bool sup = ps.FindOneBool("supersample", false);
	return new MitchellFilter(sup, B, C, xw, yw);
}

static DynamicLoader::RegisterFilter<MitchellFilter> r("mitchell");
