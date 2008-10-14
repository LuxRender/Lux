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

// shinymetal.cpp*
#include "shinymetal.h"
#include "bxdf.h"
#include "fresnelconductor.h"
#include "blinn.h"
#include "anisotropic.h"
#include "microfacet.h"
#include "specularreflection.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// ShinyMetal Method Definitions
BSDF *ShinyMetal::GetBSDF(const TsPack *tspack, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading, float) const {
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	BSDF *bsdf = BSDF_ALLOC(tspack, BSDF)(dgs, dgGeom.nn);
	SWCSpectrum spec(tspack, Ks->Evaluate(dgs).Clamp());
	SWCSpectrum R(tspack, Kr->Evaluate(dgs).Clamp());

	float u = nu->Evaluate(dgs);
	float v = nv->Evaluate(dgs);

	MicrofacetDistribution *md;
	if(u == v)
		md = BSDF_ALLOC(tspack, Blinn)(1.f / u);
	else
		md = BSDF_ALLOC(tspack, Anisotropic)(1.f/u, 1.f/v);

	SWCSpectrum k = 0.;
	Fresnel *frMf = BSDF_ALLOC(tspack, FresnelConductor)(FresnelApproxEta(spec), k);
	Fresnel *frSr = BSDF_ALLOC(tspack, FresnelConductor)(FresnelApproxEta(R), k);
	bsdf->Add(BSDF_ALLOC(tspack, Microfacet)(1., frMf, md));
	bsdf->Add(BSDF_ALLOC(tspack, SpecularReflection)(1., frSr, 0.f, 0.f));
	return bsdf;
}
Material* ShinyMetal::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	boost::shared_ptr<Texture<RGBColor> > Kr = mp.GetRGBColorTexture("Kr", RGBColor(1.f));
	boost::shared_ptr<Texture<RGBColor> > Ks = mp.GetRGBColorTexture("Ks", RGBColor(1.f));
	boost::shared_ptr<Texture<float> > uroughness = mp.GetFloatTexture("uroughness", .1f);
	boost::shared_ptr<Texture<float> > vroughness = mp.GetFloatTexture("vroughness", .1f);
	boost::shared_ptr<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap");
	return new ShinyMetal(Ks, uroughness, vroughness, Kr, bumpMap);
}

static DynamicLoader::RegisterMaterial<ShinyMetal> r("shinymetal");
