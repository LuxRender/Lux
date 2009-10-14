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
#include "bxdf.h"
#include "fresnelgeneral.h"
#include "blinn.h"
#include "anisotropic.h"
#include "microfacet.h"
#include "specularreflection.h"
#include "texture.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// ShinyMetal Method Definitions
BSDF *ShinyMetal::GetBSDF(const TsPack *tspack, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const {
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	MultiBSDF *bsdf = BSDF_ALLOC(tspack, MultiBSDF)(dgs, dgGeom.nn);
	SWCSpectrum spec = Ks->Evaluate(tspack, dgs).Clamp();
	SWCSpectrum R = Kr->Evaluate(tspack, dgs).Clamp();

	float u = nu->Evaluate(tspack, dgs);
	float v = nv->Evaluate(tspack, dgs);

	float flm = film->Evaluate(tspack, dgs);
	float flmindex = filmindex->Evaluate(tspack, dgs);

	MicrofacetDistribution *md;
	if(u == v)
		md = BSDF_ALLOC(tspack, Blinn)(1.f / u);
	else
		md = BSDF_ALLOC(tspack, Anisotropic)(1.f/u, 1.f/v);

	Fresnel *frMf = BSDF_ALLOC(tspack, FresnelGeneral)(FresnelApproxEta(spec), FresnelApproxK(spec));
	Fresnel *frSr = BSDF_ALLOC(tspack, FresnelGeneral)(FresnelApproxEta(R), FresnelApproxK(R));
	bsdf->Add(BSDF_ALLOC(tspack, Microfacet)(1., frMf, md));
	bsdf->Add(BSDF_ALLOC(tspack, SpecularReflection)(1., frSr, flm, flmindex));

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(compParams);

	return bsdf;
}
Material* ShinyMetal::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	boost::shared_ptr<Texture<SWCSpectrum> > Kr = mp.GetSWCSpectrumTexture("Kr", RGBColor(1.f));
	boost::shared_ptr<Texture<SWCSpectrum> > Ks = mp.GetSWCSpectrumTexture("Ks", RGBColor(1.f));
	boost::shared_ptr<Texture<float> > uroughness = mp.GetFloatTexture("uroughness", .1f);
	boost::shared_ptr<Texture<float> > vroughness = mp.GetFloatTexture("vroughness", .1f);
	boost::shared_ptr<Texture<float> > film = mp.GetFloatTexture("film", 0.f);				// Thin film thickness in nanometers
	boost::shared_ptr<Texture<float> > filmindex = mp.GetFloatTexture("filmindex", 1.5f);				// Thin film index of refraction
	boost::shared_ptr<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap");

	// Get Compositing Params
	CompositingParams cP;
	FindCompositingParams(mp, &cP);

	return new ShinyMetal(Ks, uroughness, vroughness, film, filmindex, Kr, bumpMap, cP);
}

static DynamicLoader::RegisterMaterial<ShinyMetal> r("shinymetal");
