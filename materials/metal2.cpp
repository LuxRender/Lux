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

// metal2.cpp
#include "metal2.h"
#include "memory.h"
#include "bxdf.h"
#include "primitive.h"
#include "microfacet.h"
#include "schlickdistribution.h"
#include "paramset.h"
#include "dynload.h"
#include "error.h"

using namespace lux;

Metal2::Metal2(boost::shared_ptr<Texture<FresnelGeneral> > &fr, 
	boost::shared_ptr<Texture<float> > &u,
	boost::shared_ptr<Texture<float> > &v,
	const ParamSet &mp) : Material(mp), fresnel(fr), nu(u), nv(v)
{
}

BSDF *Metal2::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Intersection &isect, const DifferentialGeometry &dgs) const
{
	// Allocate _BSDF_
	float u = nu->Evaluate(sw, dgs);
	float v = nv->Evaluate(sw, dgs);
	const float u2 = u * u;
	const float v2 = v * v;

	const float anisotropy = u2 < v2 ? 1.f - u2 / v2 : v2 / u2 - 1.f;
	SchlickDistribution *md = ARENA_ALLOC(arena, SchlickDistribution)(u * v, anisotropy);
	FresnelGeneral *fr = ARENA_ALLOC(arena, FresnelGeneral)(fresnel->Evaluate(sw, dgs));

	MicrofacetReflection *bxdf = ARENA_ALLOC(arena, MicrofacetReflection)(1.f,
		fr, md, false);
	SingleBSDF *bsdf = ARENA_ALLOC(arena, SingleBSDF)(dgs,
		isect.dg.nn, bxdf, isect.exterior, isect.interior);

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(&compParams);

	return bsdf;
}

Material *Metal2::CreateMaterial(const Transform &xform, const ParamSet &tp)
{
	boost::shared_ptr<Texture<FresnelGeneral> > fr(tp.GetFresnelTexture("fresnel", 5.f));

	boost::shared_ptr<Texture<float> > uroughness(tp.GetFloatTexture("uroughness", .1f));
	boost::shared_ptr<Texture<float> > vroughness(tp.GetFloatTexture("vroughness", .1f));

	return new Metal2(fr, uroughness, vroughness, tp);
}

static DynamicLoader::RegisterMaterial<Metal2> r("metal2");
