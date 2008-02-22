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
#include "microfacet.h"
#include "specularreflection.h"

using namespace lux;

// ShinyMetal Method Definitions
BSDF *ShinyMetal::GetBSDF(const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const {
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	BSDF *bsdf = BSDF_ALLOC( BSDF)(dgs, dgGeom.nn);
	SWCSpectrum spec(Ks->Evaluate(dgs).Clamp());
	float rough = roughness->Evaluate(dgs);
	SWCSpectrum R(Kr->Evaluate(dgs).Clamp());

	MicrofacetDistribution *md = BSDF_ALLOC( Blinn)(1.f / rough);
	SWCSpectrum k = 0.;
	Fresnel *frMf = BSDF_ALLOC( FresnelConductor)(FresnelApproxEta(spec), k);
	Fresnel *frSr = BSDF_ALLOC( FresnelConductor)(FresnelApproxEta(R), k);
	bsdf->Add(BSDF_ALLOC( Microfacet)(1., frMf, md));
	bsdf->Add(BSDF_ALLOC( SpecularReflection)(1., frSr));
	return bsdf;
}
Material* ShinyMetal::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	boost::shared_ptr<Texture<Spectrum> > Kr = mp.GetSpectrumTexture("Kr", Spectrum(1.f));
	boost::shared_ptr<Texture<Spectrum> > Ks = mp.GetSpectrumTexture("Ks", Spectrum(1.f));
	boost::shared_ptr<Texture<float> > roughness = mp.GetFloatTexture("roughness", .1f);
	boost::shared_ptr<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap", 0.f);
	return new ShinyMetal(Ks, roughness, Kr, bumpMap);
}
