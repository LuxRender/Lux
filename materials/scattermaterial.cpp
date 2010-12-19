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

// scattermaterial.cpp*
#include "scattermaterial.h"
#include "memory.h"
#include "bxdf.h"
#include "primitive.h"
#include "schlickscatter.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Matte Method Definitions
BSDF *ScatterMaterial::GetBSDF(MemoryArena &arena,
	const SpectrumWavelengths &sw,
	const Intersection &isect, const DifferentialGeometry &dgs) const
{
	// Allocate _BSDF_
	// Evaluate textures for _Matte_ material and allocate BRDF
	// NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum r = Kd->Evaluate(sw, dgs).Clamp(0.f, 1.f);
	float g = Clamp(G->Evaluate(sw, dgs), -1.f, 1.f);
	SchlickScatter *bsdf = ARENA_ALLOC(arena, SchlickScatter)(dgs,
		isect.dg.nn, isect.exterior, isect.interior, r, g);

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(&compParams);

	return bsdf;
}
Material* ScatterMaterial::CreateMaterial(const Transform &xform,
	const ParamSet &mp)
{
	boost::shared_ptr<Texture<SWCSpectrum> > Kd(mp.GetSWCSpectrumTexture("Kd", RGBColor(.9f)));
	boost::shared_ptr<Texture<float> > g(mp.GetFloatTexture("g", 0.f));
	return new ScatterMaterial(Kd, g, mp);
}

static DynamicLoader::RegisterMaterial<ScatterMaterial> r("scatter");
