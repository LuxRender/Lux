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

// Modified material based on glossy2 by paco

#include "glossytranslucent.h"
#include "memory.h"
#include "bxdf.h"
#include "primitive.h"
#include "schlicktranslucentbtdf.h"
#include "schlickbrdf.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Glossy Method Definitions
BSDF *GlossyTranslucent::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Intersection &isect, const DifferentialGeometry &dgs) const
{
	// Allocate _BSDF_
	// NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum d(Kd->Evaluate(sw, dgs).Clamp(0.f, 1.f));
	SWCSpectrum t(Kt->Evaluate(sw, dgs).Clamp(0.f, 1.f));
	
	SWCSpectrum s(Ks->Evaluate(sw, dgs));
	float i = index->Evaluate(sw, dgs);
	if (i > 0.f) {
		const float ti = (i - 1.f) / (i + 1.f);
		s *= ti * ti;
	}
	s = s.Clamp(0.f, 1.f);
	
	SWCSpectrum a(Ka->Evaluate(sw, dgs).Clamp(0.f, 1.f));
	float ld = depth->Evaluate(sw, dgs);

	SWCSpectrum bs(Ks_bf->Evaluate(sw, dgs));
	float bi = index_bf->Evaluate(sw, dgs);
	if (bi > 0.f) {
		const float bti = (bi - 1.f) / (bi + 1.f);
		bs *= bti * bti;
	}
	bs = bs.Clamp(0.f, 1.f);

	SWCSpectrum ba(Ka_bf->Evaluate(sw, dgs).Clamp(0.f, 1.f));
	float bld = depth_bf->Evaluate(sw, dgs);

	// Clamp roughness values to avoid artifacts with too small values
	const float u = Clamp(nu->Evaluate(sw, dgs), 6e-3f, 1.f);
	const float v = Clamp(nv->Evaluate(sw, dgs), 6e-3f, 1.f);
	const float u2 = u * u;
	const float v2 = v * v;

	const float roughness = u * v;
	const float anisotropy = u2 < v2 ? 1.f - u2 / v2 : v2 / u2 - 1.f;
	
	// Clamp roughness values to avoid artifacts with too small values
	const float bu = Clamp(nu_bf->Evaluate(sw, dgs), 6e-3f, 1.f);
	const float bv = Clamp(nv_bf->Evaluate(sw, dgs), 6e-3f, 1.f);
	const float bu2 = bu * bu;
	const float bv2 = bv * bv;

	const float broughness = bu * bv;
	const float banisotropy = bu2 < bv2 ? 1.f - bu2 / bv2 : bv2 / bu2 - 1.f;
	
	MultiBSDF *bsdf = ARENA_ALLOC(arena, MultiBSDF)(dgs, isect.dg.nn,
		isect.exterior, isect.interior);
	
	// add the BRDF
	bsdf->Add(ARENA_ALLOC(arena, SchlickDoubleSidedBRDF)(d, s, bs, a, ba,
		ld, bld, roughness, broughness, anisotropy, banisotropy,
		multibounce, multibounce_bf));
	
	// add the BTDF
	bsdf->Add(ARENA_ALLOC(arena, SchlickTranslucentBTDF)(d, t, s, bs, a, ba,
		ld, bld));

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(&compParams);

	return bsdf;
}

Material* GlossyTranslucent::CreateMaterial(const Transform &xform,
	const ParamSet &mp)
{
	boost::shared_ptr<Texture<SWCSpectrum> > Kd(mp.GetSWCSpectrumTexture("Kd", RGBColor(1.f)));
	boost::shared_ptr<Texture<SWCSpectrum> > Kt(mp.GetSWCSpectrumTexture("Kt", RGBColor(1.f)));
	bool ones = mp.FindOneBool("onesided", true);

	boost::shared_ptr<Texture<SWCSpectrum> > Ks(mp.GetSWCSpectrumTexture("Ks", RGBColor(1.f)));
	boost::shared_ptr<Texture<float> > i(mp.GetFloatTexture("index", 0.0f));
	boost::shared_ptr<Texture<SWCSpectrum> > Ka(mp.GetSWCSpectrumTexture("Ka", RGBColor(0.0f)));
	boost::shared_ptr<Texture<float> > d(mp.GetFloatTexture("d", 0.0f));
	boost::shared_ptr<Texture<float> > uroughness(mp.GetFloatTexture("uroughness", 0.1f));
	boost::shared_ptr<Texture<float> > vroughness(mp.GetFloatTexture("vroughness", 0.1f));
	bool mb = mp.FindOneBool("multibounce", false);

	if (ones) { // copy parameters from frontface to backface
		return new GlossyTranslucent(Kd, Kt, Ks, Ks, Ka, Ka, i, i, d, d,
			uroughness, uroughness, vroughness, vroughness, mb, mb, mp);
	}

	// otherwise use backface parameters
	boost::shared_ptr<Texture<SWCSpectrum> > Ks2(mp.GetSWCSpectrumTexture("backface_Ks", RGBColor(0.f)));
	boost::shared_ptr<Texture<float> > i2(mp.GetFloatTexture("backface_index", 0.0f));
	boost::shared_ptr<Texture<SWCSpectrum> > Ka2(mp.GetSWCSpectrumTexture("backface_Ka", RGBColor(0.0f)));
	boost::shared_ptr<Texture<float> > d2(mp.GetFloatTexture("backface_d", 0.0f));
	boost::shared_ptr<Texture<float> > uroughness2(mp.GetFloatTexture("backface_uroughness", 0.f));
	boost::shared_ptr<Texture<float> > vroughness2(mp.GetFloatTexture("backface_vroughness", 0.f));
	bool mb2 = mp.FindOneBool("backface_multibounce", false);

	return new GlossyTranslucent(Kd, Kt, Ks, Ks2, Ka, Ka2, i, i2, d, d2,
		uroughness, uroughness2, vroughness, vroughness2, mb, mb2, mp);
}

static DynamicLoader::RegisterMaterial<GlossyTranslucent> r("glossytranslucent");
