/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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
#include "bxdf.h"
#include "fresneldielectric.h"
#include "microfacet.h"
#include "blinn.h"
#include "anisotropic.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// RoughGlass Method Definitions
BSDF *RoughGlass::GetBSDF(const TsPack *tspack, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading, float u) const {
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000078: index of refraction swapped and not recorded
	float ior = index->Evaluate(dgs);
	float cb = cauchyb->Evaluate(dgs);
	BSDF *bsdf = BSDF_ALLOC(tspack, BSDF)(dgs, dgGeom.nn, ior);
    // NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum R(tspack, Kr->Evaluate(dgs).Clamp(0.f, 1.f));
	SWCSpectrum T(tspack, Kt->Evaluate(dgs).Clamp(0.f, 1.f));
	float urough = uroughness->Evaluate(dgs);
	float vrough = vroughness->Evaluate(dgs);
	MicrofacetDistribution *md;
	if(urough == vrough)
		md = BSDF_ALLOC(tspack, Blinn)(1.f / urough);
	else
		md = BSDF_ALLOC(tspack, Anisotropic)(1.f / urough, 1.f / vrough);
	if (!R.Black()) {
		Fresnel *fresnel = BSDF_ALLOC(tspack, FresnelDielectric)(1.f, ior);
		bsdf->Add(BSDF_ALLOC(tspack, Microfacet)(R, fresnel, md));
	}
	if (!T.Black()) {
		Fresnel *fresnel = BSDF_ALLOC(tspack, FresnelDielectricComplement)(1.f, ior, cb);
		// Radiance - NOTE - added use of blinn if roughness is isotropic for efficiency reasons
		bsdf->Add(BSDF_ALLOC(tspack, BRDFToBTDF)(BSDF_ALLOC(tspack, Microfacet)(T, fresnel, md), 1.f, ior, cb));
	}
	return bsdf;
}
Material* RoughGlass::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	boost::shared_ptr<Texture<RGBColor> > Kr = mp.GetRGBColorTexture("Kr", RGBColor(1.f));
	boost::shared_ptr<Texture<RGBColor> > Kt = mp.GetRGBColorTexture("Kt", RGBColor(1.f));
	boost::shared_ptr<Texture<float> > uroughness = mp.GetFloatTexture("uroughness", .001f);
	boost::shared_ptr<Texture<float> > vroughness = mp.GetFloatTexture("vroughness", .001f);
	boost::shared_ptr<Texture<float> > index = mp.GetFloatTexture("index", 1.5f);
	boost::shared_ptr<Texture<float> > cbf = mp.GetFloatTexture("cauchyb", 0.f);				// Cauchy B coefficient
	boost::shared_ptr<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap");
	return new RoughGlass(Kr, Kt, uroughness, vroughness, index, cbf, bumpMap);
}

static DynamicLoader::RegisterMaterial<RoughGlass> r("roughglass");
