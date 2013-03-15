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

// roughglass.cpp*
#include "roughglass.h"
#include "memory.h"
#include "multibsdf.h"
#include "primitive.h"
#include "fresnelcauchy.h"
#include "microfacet.h"
#include "schlickdistribution.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// RoughGlass Method Definitions
BSDF *RoughGlass::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Intersection &isect, const DifferentialGeometry &dgs) const
{
	// Allocate _BSDF_
	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000078: index of refraction swapped and not recorded
	float ior = index->Evaluate(sw, dgs);
	float cb = cauchyb->Evaluate(sw, dgs);
	SWCSpectrum bcolor = Sc->Evaluate(sw, dgs);
	float bscale = dgs.Scale;

	MultiBSDF<2> *bsdf = ARENA_ALLOC(arena, MultiBSDF<2>)(dgs, isect.dg.nn,
		isect.exterior, isect.interior, bcolor, bscale);
	// NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum R = Kr->Evaluate(sw, dgs).Clamp(0.f, 1.f);
	SWCSpectrum T = Kt->Evaluate(sw, dgs).Clamp(0.f, 1.f);
	float u = Clamp(uroughness->Evaluate(sw, dgs), 6e-3f, 1.f);
	float v = Clamp(vroughness->Evaluate(sw, dgs), 6e-3f, 1.f);
	const float u2 = u * u;
	const float v2 = v * v;

	const float anisotropy = u2 < v2 ? 1.f - u2 / v2 : v2 / u2 - 1.f;
	SchlickDistribution *md = ARENA_ALLOC(arena, SchlickDistribution)(u * v, anisotropy);
	const FresnelCauchy *fresnel = ARENA_ALLOC(arena, FresnelCauchy)
		(ior, cb, 0.f);
	if (!R.Black()) {
		bsdf->Add(ARENA_ALLOC(arena, MicrofacetReflection)(R,
			fresnel, md));
	}
	if (!T.Black()) {
		bsdf->Add(ARENA_ALLOC(arena, MicrofacetTransmission)(T,
			fresnel, md, dispersion));
	}

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(&compParams);

	return bsdf;
}
Material* RoughGlass::CreateMaterial(const Transform &xform,
		const ParamSet &mp) {
	boost::shared_ptr<Texture<SWCSpectrum> > Kr(mp.GetSWCSpectrumTexture("Kr", RGBColor(1.f)));
	boost::shared_ptr<Texture<SWCSpectrum> > Kt(mp.GetSWCSpectrumTexture("Kt", RGBColor(1.f)));
	boost::shared_ptr<Texture<float> > uroughness(mp.GetFloatTexture("uroughness", .001f));
	boost::shared_ptr<Texture<float> > vroughness(mp.GetFloatTexture("vroughness", .001f));
	boost::shared_ptr<Texture<float> > index(mp.GetFloatTexture("index", 1.5f));
	boost::shared_ptr<Texture<float> > cbf(mp.GetFloatTexture("cauchyb", 0.f));				// Cauchy B coefficient
	bool disp = mp.FindOneBool("dispersion", false);

	return new RoughGlass(Kr, Kt, uroughness, vroughness, index, cbf, disp, mp);
}

static DynamicLoader::RegisterMaterial<RoughGlass> r("roughglass");
