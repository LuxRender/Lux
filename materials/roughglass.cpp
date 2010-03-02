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

// roughglass.cpp*
#include "roughglass.h"
#include "memory.h"
#include "bxdf.h"
#include "brdftobtdf.h"
#include "fresnelcauchy.h"
#include "microfacet.h"
#include "blinn.h"
#include "anisotropic.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// RoughGlass Method Definitions
BSDF *RoughGlass::GetBSDF(const TsPack *tspack,
	const DifferentialGeometry &dgGeom,
	const DifferentialGeometry &dgs,
	const Volume *exterior, const Volume *interior) const
{
	// Allocate _BSDF_
	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000078: index of refraction swapped and not recorded
	float ior = index->Evaluate(tspack, dgs);
	float cb = cauchyb->Evaluate(tspack, dgs);
	MultiBSDF *bsdf = ARENA_ALLOC(tspack->arena, MultiBSDF)(dgs, dgGeom.nn,
		exterior, interior);
	// NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum R = Kr->Evaluate(tspack, dgs).Clamp(0.f, 1.f);
	SWCSpectrum T = Kt->Evaluate(tspack, dgs).Clamp(0.f, 1.f);
	float urough = uroughness->Evaluate(tspack, dgs);
	float vrough = vroughness->Evaluate(tspack, dgs);
	MicrofacetDistribution *md;
	// Radiance - NOTE - added use of blinn if roughness is isotropic for efficiency reasons
	if(urough == vrough)
		md = ARENA_ALLOC(tspack->arena, Blinn)(1.f / urough);
	else
		md = ARENA_ALLOC(tspack->arena, Anisotropic)(1.f / urough,
			1.f / vrough);
	const Fresnel *fresnel = ARENA_ALLOC(tspack->arena, FresnelCauchy)(ior,
		cb, 0.f);
	if (!R.Black()) {
		bsdf->Add(ARENA_ALLOC(tspack->arena, MicrofacetReflection)(R,
			fresnel, md));
	}
	if (!T.Black()) {
		bsdf->Add(ARENA_ALLOC(tspack->arena, MicrofacetTransmission)(T,
			fresnel, md));
	}

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(compParams);

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
	boost::shared_ptr<Texture<float> > bumpMap(mp.GetFloatTexture("bumpmap"));

	// Get Compositing Params
	CompositingParams cP;
	FindCompositingParams(mp, &cP);

	return new RoughGlass(Kr, Kt, uroughness, vroughness, index, cbf, bumpMap, cP);
}

static DynamicLoader::RegisterMaterial<RoughGlass> r("roughglass");
