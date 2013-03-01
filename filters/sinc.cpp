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

// sinc.cpp*
#include "sinc.h"
#include "dynload.h"

using namespace lux;

// Sinc Filter Method Definitions
float LanczosSincFilter::Evaluate(float x, float y) const{
	return Sinc1D(x * invXWidth) * Sinc1D(y * invYWidth);
}
float LanczosSincFilter::Sinc1D(float x) const {
	x = fabsf(x);
	if (x < 1e-5) return 1.f;
	if (x > 1.)   return 0.f;
	x *= M_PI;
	float sinc = sinf(x * tau) / (x * tau);
	float lanczos = sinf(x) / x;
	return sinc * lanczos;
}
Filter* LanczosSincFilter::CreateFilter(const ParamSet &ps) {
	float xw = ps.FindOneFloat("xwidth", 4.);
	float yw = ps.FindOneFloat("ywidth", 4.);
	float tau = ps.FindOneFloat("tau", 3.f);
	return new LanczosSincFilter(xw, yw, tau);
}

static DynamicLoader::RegisterFilter<LanczosSincFilter> r("sinc");
