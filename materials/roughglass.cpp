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

using namespace lux;

// RoughGlass Method Definitions
BSDF *RoughGlass::GetBSDF(const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading, float u) const {
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000078: index of refraction swapped and not recorded
	float ior = index->Evaluate(dgs);
	float cb = cauchyb->Evaluate(dgs);
	BSDF *bsdf = BSDF_ALLOC( BSDF)(dgs, dgGeom.nn, ior);
    // NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum R(Kr->Evaluate(dgs).Clamp(0.f, 1.f));
	SWCSpectrum T(Kt->Evaluate(dgs).Clamp(0.f, 1.f));
	float urough = uroughness->Evaluate(dgs);
	float vrough = vroughness->Evaluate(dgs);
	if (!R.Black()) {
		Fresnel *fresnel = BSDF_ALLOC( FresnelDielectric)(ior, 1.);
		if(urough == vrough)
			bsdf->Add(BSDF_ALLOC( Microfacet)(R *.5, fresnel,
				BSDF_ALLOC( Blinn)(1.f/urough)));
		else
			bsdf->Add(BSDF_ALLOC( Microfacet)(R *.5, fresnel,
				BSDF_ALLOC( Anisotropic)(1.f/urough, 1.f/vrough)));
	}
	if (!T.Black()) {
		Fresnel *fresnel = BSDF_ALLOC( FresnelDielectric)(ior, 1., cb);
		// Radiance - NOTE - added use of blinn if roughness is isotropic for efficiency reasons
		if(urough == vrough)
			bsdf->Add(BSDF_ALLOC( BRDFToBTDF)(BSDF_ALLOC( Microfacet)(T *.5, fresnel,
				BSDF_ALLOC( Blinn)(1.f/urough))));
		else
			bsdf->Add(BSDF_ALLOC( BRDFToBTDF)(BSDF_ALLOC( Microfacet)(T *.5, fresnel,
				BSDF_ALLOC( Anisotropic)(1.f/urough, 1.f/vrough))));
	}
	return bsdf;
}
Material* RoughGlass::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	boost::shared_ptr<Texture<Spectrum> > Kr = mp.GetSpectrumTexture("Kr", Spectrum(1.f));
	boost::shared_ptr<Texture<Spectrum> > Kt = mp.GetSpectrumTexture("Kt", Spectrum(1.f));
	boost::shared_ptr<Texture<float> > uroughness = mp.GetFloatTexture("uroughness", .001f);
	boost::shared_ptr<Texture<float> > vroughness = mp.GetFloatTexture("vroughness", .001f);
	boost::shared_ptr<Texture<float> > index = mp.GetFloatTexture("index", 1.5f);
	boost::shared_ptr<Texture<float> > cbf = mp.GetFloatTexture("cauchyb", 0.f);				// Cauchy B coefficient
	boost::shared_ptr<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap", 0.f);
	return new RoughGlass(Kr, Kt, uroughness, vroughness, index, cbf, bumpMap);
}
