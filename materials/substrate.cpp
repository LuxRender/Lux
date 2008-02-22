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

// substrate.cpp*
#include "substrate.h"
#include "bxdf.h"
#include "fresnelblend.h"
#include "blinn.h"
#include "anisotropic.h"

using namespace lux;

// Substrate Method Definitions
BSDF *Substrate::GetBSDF(const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const {
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	BSDF *bsdf = BSDF_ALLOC( BSDF)(dgs, dgGeom.nn);
    // NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum d(Kd->Evaluate(dgs).Clamp(0.f, 1.f));
	SWCSpectrum s(Ks->Evaluate(dgs).Clamp(0.f, 1.f));

	float u = nu->Evaluate(dgs);
	float v = nv->Evaluate(dgs);

	// Radiance - NOTE - added use of blinn if roughness is isotropic for efficiency reasons
	if(u == v)
		bsdf->Add(BSDF_ALLOC( FresnelBlend)(d, s, BSDF_ALLOC( Blinn)(1.f/u)));
	else
		bsdf->Add(BSDF_ALLOC( FresnelBlend)(d, s, BSDF_ALLOC( Anisotropic)(1.f/u, 1.f/v)));

	return bsdf;
}
Material* Substrate::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	boost::shared_ptr<Texture<Spectrum> > Kd = mp.GetSpectrumTexture("Kd", Spectrum(.5f));
	boost::shared_ptr<Texture<Spectrum> > Ks = mp.GetSpectrumTexture("Ks", Spectrum(.5f));
	boost::shared_ptr<Texture<float> > uroughness = mp.GetFloatTexture("uroughness", .1f);
	boost::shared_ptr<Texture<float> > vroughness = mp.GetFloatTexture("vroughness", .1f);
	boost::shared_ptr<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap", 0.f);
	return new Substrate(Kd, Ks, uroughness, vroughness, bumpMap);
}
