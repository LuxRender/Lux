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

// Glossy material - based on previous/pbrt 'substrate' material using optional absorption

// glossy2.cpp*
#include "glossy2.h"
#include "memory.h"
#include "bxdf.h"
#include "schlickbrdf.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Glossy Method Definitions
BSDF *Glossy2::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const DifferentialGeometry &dgGeom,
	const DifferentialGeometry &dgs,
	const Volume *exterior, const Volume *interior) const
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
	const float u = Clamp(nu->Evaluate(sw, dgs), 1e-4f, 1.f);
	const float v = Clamp(nv->Evaluate(sw, dgs), 1e-4f, 1.f);
	const float u2 = u * u;
	const float v2 = v * v;
	float ld = depth->Evaluate(sw, dgs);

	const float anisotropy = u2 < v2 ? 1.f - u2 / v2 : v2 / u2 - 1.f;
	SingleBSDF *bsdf = ARENA_ALLOC(arena, SingleBSDF)(dgs,
		dgGeom.nn, ARENA_ALLOC(arena, SchlickBRDF)(d, s, a, ld, u * v,
		anisotropy), exterior, interior);

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(compParams);

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
	boost::shared_ptr<Texture<float> > bumpMap(mp.GetFloatTexture("bumpmap"));

	// Get Compositing Params
	CompositingParams cP;
	FindCompositingParams(mp, &cP);

	return new Glossy2(Kd, Ks, Ka, i, d, uroughness, vroughness, bumpMap, cP);
}

static DynamicLoader::RegisterMaterial<Glossy2> r("glossy");
