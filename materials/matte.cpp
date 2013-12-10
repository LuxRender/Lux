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

// matte.cpp*
#include "matte.h"
#include "memory.h"
#include "singlebsdf.h"
#include "primitive.h"
#include "lambertian.h"
#include "orennayar.h"
#include "texture.h"
#include "luxrays/core/color/color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;
using namespace luxrays;

// Matte Method Definitions
BSDF *Matte::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Intersection &isect, const DifferentialGeometry &dgs) const
{
	// Allocate _BSDF_
	// Evaluate textures for _Matte_ material and allocate BRDF
	// NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum r = Kd->Evaluate(sw, dgs).Clamp(0.f, 1.f);
	float sig = Clamp(sigma->Evaluate(sw, dgs), 0.f, 90.f);
	BxDF *bxdf;
	if (sig == 0.f)
		bxdf = ARENA_ALLOC(arena, Lambertian)(r);
	else
		bxdf = ARENA_ALLOC(arena, OrenNayar)(r, sig);
	SingleBSDF *bsdf = ARENA_ALLOC(arena, SingleBSDF)(dgs,
		isect.dg.nn, bxdf, isect.exterior, isect.interior);

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(&compParams);

	return bsdf;
}
Material* Matte::CreateMaterial(const Transform &xform,
		const ParamSet &mp) {
	boost::shared_ptr<Texture<SWCSpectrum> > Kd(mp.GetSWCSpectrumTexture("Kd", RGBColor(.9f)));
	boost::shared_ptr<Texture<float> > sigma(mp.GetFloatTexture("sigma", 0.f));
	return new Matte(Kd, sigma, mp);
}

static DynamicLoader::RegisterMaterial<Matte> r("matte");
