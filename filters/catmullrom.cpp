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

//catmullrom.cpp
#include "catmullrom.h"
#include "dynload.h"

using namespace lux;

float CatmullRomFilter::Evaluate(float x, float y) const {
	return CatmullRom1D(x) * CatmullRom1D(y);
}

Filter* CatmullRomFilter::CreateFilter(const ParamSet &ps) {
	float xw = ps.FindOneFloat("xwidth", 4.f);
	float yw = ps.FindOneFloat("ywidth", 4.f);
	return new CatmullRomFilter(xw, yw);
}

static DynamicLoader::RegisterFilter<CatmullRomFilter> r("catmullrom");