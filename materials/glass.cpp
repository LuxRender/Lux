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

// glass.cpp*
#include "glass.h"
#include "bxdf.h"
#include "specularreflection.h"
#include "speculartransmission.h"
#include "fresneldielectric.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Glass Method Definitions
BSDF *Glass::GetBSDF(const TsPack *tspack, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading, float u) const {
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
	SWCSpectrum R(tspack, Kr->Evaluate(dgs).Clamp(0.f, 1.f));
	SWCSpectrum T(tspack, Kt->Evaluate(dgs).Clamp(0.f, 1.f));
	if (!R.Black()) {
		if (architectural)
			bsdf->Add(BSDF_ALLOC( ArchitecturalReflection)(R,
				BSDF_ALLOC( FresnelDielectric)(1., ior)));
		else
			bsdf->Add(BSDF_ALLOC( SpecularReflection)(R,
				BSDF_ALLOC( FresnelDielectric)(1., ior)));
	}
	if (!T.Black())
		bsdf->Add(BSDF_ALLOC( SpecularTransmission)(T, 1., ior, cb, architectural));
	return bsdf;
}
Material* Glass::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	boost::shared_ptr<Texture<RGBColor> > Kr = mp.GetRGBColorTexture("Kr", RGBColor(1.f));
	boost::shared_ptr<Texture<RGBColor> > Kt = mp.GetRGBColorTexture("Kt", RGBColor(1.f));
	boost::shared_ptr<Texture<float> > index = mp.GetFloatTexture("index", 1.5f);
	boost::shared_ptr<Texture<float> > cbf = mp.GetFloatTexture("cauchyb", 0.f);				// Cauchy B coefficient
	bool archi = mp.FindBool("architectural", false);
	boost::shared_ptr<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap", 0.f);
	return new Glass(Kr, Kt, index, cbf, archi, bumpMap);
}

static DynamicLoader::RegisterMaterial<Glass> r("glass");
