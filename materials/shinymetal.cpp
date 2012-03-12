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

// shinymetal.cpp*
#include "shinymetal.h"
#include "memory.h"
#include "multibsdf.h"
#include "primitive.h"
#include "fresnelgeneral.h"
#include "schlickdistribution.h"
#include "microfacet.h"
#include "specularreflection.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// ShinyMetal Method Definitions
BSDF *ShinyMetal::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Intersection &isect, const DifferentialGeometry &dgs) const
{
	// Allocate _BSDF_
	SWCSpectrum bcolor = (Sc->Evaluate(sw, dgs).Clamp(0.f, 10000.f))*dgs.Scale;

	MultiBSDF<2> *bsdf = ARENA_ALLOC(arena, MultiBSDF<2>)(dgs, isect.dg.nn,
		isect.exterior, isect.interior, bcolor);
	SWCSpectrum spec = Ks->Evaluate(sw, dgs).Clamp();
	SWCSpectrum R = Kr->Evaluate(sw, dgs).Clamp();

	float flm = film->Evaluate(sw, dgs);
	float flmindex = filmindex->Evaluate(sw, dgs);

	float u = nu->Evaluate(sw, dgs);
	float v = nv->Evaluate(sw, dgs);
	const float u2 = u * u;
	const float v2 = v * v;

	const float anisotropy = u2 < v2 ? 1.f - u2 / v2 : v2 / u2 - 1.f;
	SchlickDistribution *md = ARENA_ALLOC(arena, SchlickDistribution)(u * v, anisotropy);

	FresnelGeneral *frMf = ARENA_ALLOC(arena, FresnelGeneral)(AUTO_FRESNEL,
		FresnelApproxEta(spec), FresnelApproxK(spec));
	FresnelGeneral *frSr = ARENA_ALLOC(arena, FresnelGeneral)(AUTO_FRESNEL,
		FresnelApproxEta(R), FresnelApproxK(R));
	bsdf->Add(ARENA_ALLOC(arena, MicrofacetReflection)(1.f, frMf, md));
	bsdf->Add(ARENA_ALLOC(arena, SpecularReflection)(1.f, frSr,
		flm, flmindex));

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(&compParams);

	return bsdf;
}
Material* ShinyMetal::CreateMaterial(const Transform &xform,
		const ParamSet &mp) {
	boost::shared_ptr<Texture<SWCSpectrum> > Sc(mp.GetSWCSpectrumTexture("Sc", RGBColor(.9f)));
	boost::shared_ptr<Texture<SWCSpectrum> > Kr(mp.GetSWCSpectrumTexture("Kr", RGBColor(1.f)));
	boost::shared_ptr<Texture<SWCSpectrum> > Ks(mp.GetSWCSpectrumTexture("Ks", RGBColor(1.f)));
	boost::shared_ptr<Texture<float> > uroughness(mp.GetFloatTexture("uroughness", .1f));
	boost::shared_ptr<Texture<float> > vroughness(mp.GetFloatTexture("vroughness", .1f));
	boost::shared_ptr<Texture<float> > film(mp.GetFloatTexture("film", 0.f));				// Thin film thickness in nanometers
	boost::shared_ptr<Texture<float> > filmindex(mp.GetFloatTexture("filmindex", 1.5f));				// Thin film index of refraction

	return new ShinyMetal(Ks, uroughness, vroughness, film, filmindex, Kr, mp, Sc);
}

static DynamicLoader::RegisterMaterial<ShinyMetal> r("shinymetal");
