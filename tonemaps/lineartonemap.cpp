/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

// MaxWhiteOp Method Definitions
ToneMap * LinearOp::CreateToneMap(const ParamSet &ps) {
	float sensitivity = ps.FindOneFloat("sensitivity", 100.f);
	float exposure = ps.FindOneFloat("exposure", 1.f / 1000.f);
	float fstop = ps.FindOneFloat("fstop", 2.8f);
	float gamma = ps.FindOneFloat("gamma", 2.2f);
	return new LinearOp(sensitivity, exposure, fstop, gamma);
}

static DynamicLoader::RegisterToneMap<LinearOp> r("linear");
