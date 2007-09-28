/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// translucent.cpp*
#include "translucent.h"

// Translucent Method Definitions
BSDF *Translucent::GetBSDF(MemoryArena &arena, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const {
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	BSDF *bsdf = BSDF_ALLOC(arena, BSDF)(dgs, dgGeom.nn);
	Spectrum r = reflect->Evaluate(dgs).Clamp();
	Spectrum t = transmit->Evaluate(dgs).Clamp();
	if (r.Black() && t.Black()) return bsdf;

	Spectrum kd = Kd->Evaluate(dgs).Clamp();
	if (!kd.Black()) {
		if (!r.Black()) bsdf->Add(BSDF_ALLOC(arena, Lambertian)(r * kd));
		if (!t.Black()) bsdf->Add(BSDF_ALLOC(arena, BRDFToBTDF)(BSDF_ALLOC(arena, Lambertian)(t * kd)));
	}
	Spectrum ks = Ks->Evaluate(dgs).Clamp();
	if (!ks.Black()) {
		float rough = roughness->Evaluate(dgs);
		if (!r.Black()) {
			Fresnel *fresnel = BSDF_ALLOC(arena, FresnelDielectric)(1.5f, 1.f);
			bsdf->Add(BSDF_ALLOC(arena, Microfacet)(r * ks, fresnel,
				BSDF_ALLOC(arena, Blinn)(1.f / rough)));
		}
		if (!t.Black()) {
			Fresnel *fresnel = BSDF_ALLOC(arena, FresnelDielectric)(1.5f, 1.f);
			bsdf->Add(BSDF_ALLOC(arena, BRDFToBTDF)(BSDF_ALLOC(arena, Microfacet)(t * ks, fresnel,
				BSDF_ALLOC(arena, Blinn)(1.f / rough))));
		}
	}
	return bsdf;
}
Material* Translucent::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	Reference<Texture<Spectrum> > Kd = mp.GetSpectrumTexture("Kd", Spectrum(1.f));
	Reference<Texture<Spectrum> > Ks = mp.GetSpectrumTexture("Ks", Spectrum(1.f));
	Reference<Texture<Spectrum> > reflect = mp.GetSpectrumTexture("reflect", Spectrum(0.5f));
	Reference<Texture<Spectrum> > transmit = mp.GetSpectrumTexture("transmit", Spectrum(0.5f));
	Reference<Texture<float> > roughness = mp.GetFloatTexture("roughness", .1f);
	Reference<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap", 0.f);
	return new Translucent(Kd, Ks, roughness, reflect, transmit, bumpMap);
}
