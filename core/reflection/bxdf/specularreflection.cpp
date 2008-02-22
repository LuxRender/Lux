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

// specularreflection.cpp*
#include "specularreflection.h"
#include "color.h"
#include "spectrum.h"
#include "mc.h"
#include "sampling.h"
#include "fresnel.h"
#include <stdarg.h>

using namespace lux;

SWCSpectrum SpecularReflection::Sample_f(const Vector &wo,
		Vector *wi, float u1, float u2, float *pdf) const {
	// Compute perfect specular reflection direction
	*wi = Vector(-wo.x, -wo.y, wo.z);
	*pdf = 1.f;
	return fresnel->Evaluate(CosTheta(wo)) * R /
		fabsf(CosTheta(*wi));
}