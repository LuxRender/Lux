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

// Glossy material - based on previous/pbrt 'substrate' material using optional absorption

// glossy2.cpp*
#include "glossy2.h"
#include "memory.h"
#include "singlebsdf.h"
#include "schlickbsdf.h"
#include "primitive.h"
#include "schlickbrdf.h"
#include "lambertian.h"
#include "orennayar.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"
#include "fresnelslick.h"
#include "schlickdistribution.h"
#include "microfacet.h"

using namespace lux;

// Glossy Method Definitions
BSDF *GlossyCombined::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Intersection &isect, const DifferentialGeometry &dgs) const
{
	// Allocate _BSDF_
	// NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum d(Kd->Evaluate(sw, dgs).Clamp(0.f, 1.f));
	SWCSpectrum s(Ks->Evaluate(sw, dgs));
	float i = index->Evaluate(sw, dgs);
	if (i > 0.f) {
		const float ti = (i - 1.f) / (i + 1.f);
		s *= ti * ti;
	}
	s = s.Clamp(0.f, 1.f);

	SWCSpectrum a(Ka->Evaluate(sw, dgs).Clamp(0.f, 1.f));

	// Clamp roughness values to avoid artifacts with too small values
	const float u = Clamp(nu->Evaluate(sw, dgs), 6e-3f, 1.f);
	const float v = Clamp(nv->Evaluate(sw, dgs), 6e-3f, 1.f);
	const float u2 = u * u;
	const float v2 = v * v;
	float ld = depth->Evaluate(sw, dgs);

	const float anisotropy = u2 < v2 ? 1.f - u2 / v2 : v2 / u2 - 1.f;
	SingleBSDF *bsdf = ARENA_ALLOC(arena, SingleBSDF)(dgs,
		isect.dg.nn, ARENA_ALLOC(arena, SchlickBRDF)(d, s, a, ld, u * v,
		anisotropy, multibounce), isect.exterior, isect.interior);

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(&compParams);

	return bsdf;
}

BSDF *Glossy2::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Intersection &isect, const DifferentialGeometry &dgs) const
{
	// Allocate _BSDF_
	// NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum d(Kd->Evaluate(sw, dgs).Clamp(0.f, 1.f));
	SWCSpectrum s(Ks->Evaluate(sw, dgs));
	float i = index->Evaluate(sw, dgs);
	if (i > 0.f) {
		const float ti = (i - 1.f) / (i + 1.f);
		s *= ti * ti;
	}
	s = s.Clamp(0.f, 1.f);

	SWCSpectrum a(Ka->Evaluate(sw, dgs).Clamp(0.f, 1.f));

	// Clamp roughness values to avoid artifacts with too small values
	const float u = Clamp(nu->Evaluate(sw, dgs), 6e-3f, 1.f);
	const float v = Clamp(nv->Evaluate(sw, dgs), 6e-3f, 1.f);
	const float u2 = u * u;
	const float v2 = v * v;
	float ld = depth->Evaluate(sw, dgs);

	const float anisotropy = u2 < v2 ? 1.f - u2 / v2 : v2 / u2 - 1.f;
	float sig = Clamp(sigma->Evaluate(sw, dgs), 0.f, 90.f);
	BxDF *bxdf;
	if (sig == 0.f)
		bxdf = ARENA_ALLOC(arena, Lambertian)(d);
	else
		bxdf = ARENA_ALLOC(arena, OrenNayar)(d, sig);

	SingleBSDF *base = ARENA_ALLOC(arena, SingleBSDF)(dgs,
		isect.dg.nn, bxdf, isect.exterior, isect.interior);

	Fresnel *fresnel = ARENA_ALLOC(arena, FresnelSlick)(s, a);
	MicrofacetDistribution* md = ARENA_ALLOC(arena, SchlickDistribution)(u * v, anisotropy);

	SchlickBSDF *bsdf = ARENA_ALLOC(arena, SchlickBSDF)(dgs, isect.dg.nn, fresnel, md, multibounce, 
		a, ld, base, isect.exterior, isect.interior);

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(&compParams);

	return bsdf;
}
Material* Glossy2::CreateMaterial(const Transform &xform,
		const ParamSet &mp) {
	boost::shared_ptr<Texture<SWCSpectrum> > Kd(mp.GetSWCSpectrumTexture("Kd", RGBColor(1.f)));
	boost::shared_ptr<Texture<SWCSpectrum> > Ks(mp.GetSWCSpectrumTexture("Ks", RGBColor(1.f)));
	boost::shared_ptr<Texture<SWCSpectrum> > Ka(mp.GetSWCSpectrumTexture("Ka", RGBColor(.0f)));
	boost::shared_ptr<Texture<float> > i(mp.GetFloatTexture("index", 0.0f));
	boost::shared_ptr<Texture<float> > d(mp.GetFloatTexture("d", .0f));
	boost::shared_ptr<Texture<float> > uroughness(mp.GetFloatTexture("uroughness", .1f));
	boost::shared_ptr<Texture<float> > vroughness(mp.GetFloatTexture("vroughness", .1f));
	bool mb = mp.FindOneBool("multibounce", false);

	bool separable = mp.FindOneBool("separable", true);

	if (separable) {
		boost::shared_ptr<Texture<float> > sigma(mp.GetFloatTexture("sigma", .0f));
		return new Glossy2(Kd, Ks, Ka, i, d, uroughness, vroughness, sigma, mb, mp);
	} else {
		return new GlossyCombined(Kd, Ks, Ka, i, d, uroughness, vroughness, mb, mp);
	}
}

// GlossyCoating Method Definitions
BSDF *GlossyCoating::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Intersection &isect, const DifferentialGeometry &dgs) const
{
	DifferentialGeometry dgShading = dgs;
	basemat->GetShadingGeometry(sw, isect.dg.nn, &dgShading);
	BSDF *base = basemat->GetBSDF(arena, sw, isect, dgShading);

	// Allocate _BSDF_
	// NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum s(Ks->Evaluate(sw, dgs));
	float i = index->Evaluate(sw, dgs);
	if (i > 0.f) {
		const float ti = (i - 1.f) / (i + 1.f);
		s *= ti * ti;
	}
	s = s.Clamp(0.f, 1.f);

	SWCSpectrum a(Ka->Evaluate(sw, dgs).Clamp(0.f, 1.f));

	// Clamp roughness values to avoid artifacts with too small values
	const float u = Clamp(nu->Evaluate(sw, dgs), 6e-3f, 1.f);
	const float v = Clamp(nv->Evaluate(sw, dgs), 6e-3f, 1.f);
	const float u2 = u * u;
	const float v2 = v * v;
	float ld = depth->Evaluate(sw, dgs);

	const float anisotropy = u2 < v2 ? 1.f - u2 / v2 : v2 / u2 - 1.f;

	Fresnel *fresnel = ARENA_ALLOC(arena, FresnelSlick)(s, a);
	MicrofacetDistribution* md = ARENA_ALLOC(arena, SchlickDistribution)(u * v, anisotropy);

	SchlickBSDF *bsdf = ARENA_ALLOC(arena, SchlickBSDF)(dgs, isect.dg.nn, fresnel, md, multibounce, 
		a, ld, base, isect.exterior, isect.interior);

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(&compParams);

	return bsdf;
}
Material* GlossyCoating::CreateMaterial(const Transform &xform,
		const ParamSet &mp) {
	boost::shared_ptr<Material> basemat(mp.GetMaterial("basematerial"));
	if (!basemat) {
		LOG( LUX_ERROR,LUX_BADTOKEN)<<"Base material for glossycoating is incorrect";
		return NULL;
	}
	boost::shared_ptr<Texture<SWCSpectrum> > Ks(mp.GetSWCSpectrumTexture("Ks", RGBColor(1.f)));
	boost::shared_ptr<Texture<SWCSpectrum> > Ka(mp.GetSWCSpectrumTexture("Ka", RGBColor(.0f)));
	boost::shared_ptr<Texture<float> > i(mp.GetFloatTexture("index", 0.0f));
	boost::shared_ptr<Texture<float> > d(mp.GetFloatTexture("d", .0f));
	boost::shared_ptr<Texture<float> > uroughness(mp.GetFloatTexture("uroughness", .1f));
	boost::shared_ptr<Texture<float> > vroughness(mp.GetFloatTexture("vroughness", .1f));
	bool mb = mp.FindOneBool("multibounce", false);

	return new GlossyCoating(basemat, Ks, Ka, i, d, uroughness, vroughness, mb, mp);
}

static DynamicLoader::RegisterMaterial<Glossy2> r("glossy");
static DynamicLoader::RegisterMaterial<GlossyCoating> r2("glossycoating");
