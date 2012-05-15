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

// mirror.cpp*
#include "mirror.h"
#include "memory.h"
#include "singlebsdf.h"
#include "primitive.h"
#include "specularreflection.h"
#include "fresnelnoop.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Mirror Method Definitions
BSDF *Mirror::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Intersection &isect, const DifferentialGeometry &dgs) const
{
	// Allocate _BSDF_
	float flm = film->Evaluate(sw, dgs);
	float flmindex = filmindex->Evaluate(sw, dgs);
	SWCSpectrum bcolor = Sc->Evaluate(sw, dgs);
	float bscale = dgs.Scale;

	// NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum R = Kr->Evaluate(sw, dgs).Clamp(0.f, 1.f);
	BxDF *bxdf = ARENA_ALLOC(arena, SpecularReflection)(R,
		ARENA_ALLOC(arena, FresnelNoOp)(), flm, flmindex);
	SingleBSDF *bsdf = ARENA_ALLOC(arena, SingleBSDF)(dgs,
		isect.dg.nn, bxdf, isect.exterior, isect.interior, bcolor, bscale);

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(&compParams);

	return bsdf;
}
Material* Mirror::CreateMaterial(const Transform &xform,
		const ParamSet &mp) {
	boost::shared_ptr<Texture<SWCSpectrum> > Sc(mp.GetSWCSpectrumTexture("Sc", RGBColor(.9f)));
	boost::shared_ptr<Texture<SWCSpectrum> > Kr(mp.GetSWCSpectrumTexture("Kr", RGBColor(1.f)));
	boost::shared_ptr<Texture<float> > film(mp.GetFloatTexture("film", 0.f));				// Thin film thickness in nanometers
	boost::shared_ptr<Texture<float> > filmindex(mp.GetFloatTexture("filmindex", 1.5f));				// Thin film index of refraction

	return new Mirror(Kr, film, filmindex, mp, Sc);
}

static DynamicLoader::RegisterMaterial<Mirror> r("mirror");
