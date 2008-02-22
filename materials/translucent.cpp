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

// translucent.cpp*
#include "translucent.h"
#include "bxdf.h"
#include "lambertian.h"
#include "fresneldielectric.h"
#include "microfacet.h"
#include "blinn.h"

using namespace lux;

// Translucent Method Definitions
BSDF *Translucent::GetBSDF(const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const {
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000078: index of refraction swapped and not recorded
	float ior = index->Evaluate(dgs);
	BSDF *bsdf = BSDF_ALLOC( BSDF)(dgs, dgGeom.nn, ior);
    // NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum r(reflect->Evaluate(dgs).Clamp(0.f, 1.f));
	SWCSpectrum t(transmit->Evaluate(dgs).Clamp(0.f, 1.f));
	if (r.Black() && t.Black()) return bsdf;

	SWCSpectrum kd(Kd->Evaluate(dgs).Clamp());
	if (!kd.Black()) {
		if (!r.Black()) bsdf->Add(BSDF_ALLOC( Lambertian)(r * kd));
		if (!t.Black()) bsdf->Add(BSDF_ALLOC( BRDFToBTDF)(BSDF_ALLOC( Lambertian)(t * kd)));
	}
	SWCSpectrum ks(Ks->Evaluate(dgs).Clamp());
	if (!ks.Black()) {
		float rough = roughness->Evaluate(dgs);
		if (!r.Black()) {
			// NOTE - lordcrc - Bugfix, pbrt tracker id 0000078: index of refraction swapped and not recorded
			Fresnel *fresnel = BSDF_ALLOC( FresnelDielectric)(1.f, ior);
			bsdf->Add(BSDF_ALLOC( Microfacet)(r * ks, fresnel,
				BSDF_ALLOC( Blinn)(1.f / rough)));
		}
		if (!t.Black()) {
			// NOTE - lordcrc - Bugfix, pbrt tracker id 0000078: index of refraction swapped and not recorded
			Fresnel *fresnel = BSDF_ALLOC( FresnelDielectric)(1.f, ior);
			bsdf->Add(BSDF_ALLOC( BRDFToBTDF)(BSDF_ALLOC( Microfacet)(t * ks, fresnel,
				BSDF_ALLOC( Blinn)(1.f / rough))));
		}
	}
	return bsdf;
}
Material* Translucent::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	boost::shared_ptr<Texture<Spectrum> > Kd = mp.GetSpectrumTexture("Kd", Spectrum(1.f));
	boost::shared_ptr<Texture<Spectrum> > Ks = mp.GetSpectrumTexture("Ks", Spectrum(1.f));
	boost::shared_ptr<Texture<Spectrum> > reflect = mp.GetSpectrumTexture("reflect", Spectrum(0.5f));
	boost::shared_ptr<Texture<Spectrum> > transmit = mp.GetSpectrumTexture("transmit", Spectrum(0.5f));
	boost::shared_ptr<Texture<float> > roughness = mp.GetFloatTexture("roughness", .1f);
	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000078: index of refraction swapped and not recorded
	boost::shared_ptr<Texture<float> > index = mp.GetFloatTexture("index", 1.5f);
	boost::shared_ptr<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap", 0.f);
	return new Translucent(Kd, Ks, roughness, reflect, transmit, index, bumpMap);
}
