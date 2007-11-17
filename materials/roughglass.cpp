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

// roughglass.cpp*
#include "roughglass.h"

// RoughGlass Method Definitions
BSDF *RoughGlass::GetBSDF(MemoryArena &arena, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const {
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	BSDF *bsdf = BSDF_ALLOC(arena, BSDF)(dgs, dgGeom.nn);
	Spectrum R = Kr->Evaluate(dgs).Clamp();
	Spectrum T = Kt->Evaluate(dgs).Clamp();
	float rough = roughness->Evaluate(dgs);
	float ior = index->Evaluate(dgs);
	if (!R.Black()) {
		Fresnel *fresnel = BSDF_ALLOC(arena, FresnelDielectric)(1., ior);
		bsdf->Add(BSDF_ALLOC(arena, Microfacet)(R, fresnel,
				BSDF_ALLOC(arena, Anisotropic)(1.f/rough, 1.f/rough)));
	}
	if (!T.Black()) {
		Fresnel *fresnel = BSDF_ALLOC(arena, FresnelDielectric)(1., ior);
		bsdf->Add(BSDF_ALLOC(arena, BRDFToBTDF)(BSDF_ALLOC(arena, Microfacet)(T, fresnel,
				BSDF_ALLOC(arena, Anisotropic)(1.f/rough, 1.f/rough))));

	}
	return bsdf;
}
Material* RoughGlass::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	boost::shared_ptr<Texture<Spectrum> > Kr = mp.GetSpectrumTexture("Kr", Spectrum(1.f));
	boost::shared_ptr<Texture<Spectrum> > Kt = mp.GetSpectrumTexture("Kt", Spectrum(1.f));
	boost::shared_ptr<Texture<float> > roughness = mp.GetFloatTexture("roughness", .1f);
	boost::shared_ptr<Texture<float> > index = mp.GetFloatTexture("index", 1.5f);
	boost::shared_ptr<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap", 0.f);
	return new RoughGlass(Kr, Kt, roughness, index, bumpMap);
}
