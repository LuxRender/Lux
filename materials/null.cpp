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

// null.cpp*
#include "null.h"
#include "memory.h"
#include "bxdf.h"
#include "nulltransmission.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Glass Method Definitions
BSDF *Null::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const DifferentialGeometry &dgGeom,
	const DifferentialGeometry &dgShading,
	const Volume *exterior, const Volume *interior) const
{
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	SingleBSDF *bsdf = ARENA_ALLOC(arena, SingleBSDF)(dgShading,
		dgGeom.nn, ARENA_ALLOC(arena, NullTransmission)(),
		exterior, interior);

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(compParams);

	return bsdf;
}
Material* Null::CreateMaterial(const Transform &xform,
		const ParamSet &mp) {

	// Get Compositing Params
	CompositingParams cP;
	FindCompositingParams(mp, &cP);

	return new Null(cP);
}

static DynamicLoader::RegisterMaterial<Null> r("null");
