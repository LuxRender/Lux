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

// glass.cpp*
#include "glass.h"
#include "memory.h"
#include "bxdf.h"
#include "specularreflection.h"
#include "speculartransmission.h"
#include "fresneldielectric.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Glass Method Definitions
BSDF *Glass::GetBSDF(const TsPack *tspack, const DifferentialGeometry &dgGeom,
	const DifferentialGeometry &dgShading,
	const Volume *exterior, const Volume *interior) const
{
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000078: index of refraction swapped and not recorded
	float ior = index->Evaluate(tspack, dgs);
	float cb = cauchyb->Evaluate(tspack, dgs);

	float flm = film->Evaluate(tspack, dgs);
	float flmindex = filmindex->Evaluate(tspack, dgs);

	MultiBSDF *bsdf = ARENA_ALLOC(tspack->arena, MultiBSDF)(dgs, dgGeom.nn, ior);
    // NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum R = Kr->Evaluate(tspack, dgs).Clamp(0.f, 1.f);
	SWCSpectrum T = Kt->Evaluate(tspack, dgs).Clamp(0.f, 1.f);
	Fresnel *fresnel = ARENA_ALLOC(tspack->arena, FresnelDielectric)(ior, cb, 0.f);
	if (!R.Black()) {
		if (architectural)
			bsdf->Add(ARENA_ALLOC(tspack->arena, ArchitecturalReflection)(R,
				fresnel, flm, flmindex));
		else
			bsdf->Add(ARENA_ALLOC(tspack->arena, SpecularReflection)(R,
				fresnel, flm, flmindex));
	}
	if (!T.Black())
		bsdf->Add(ARENA_ALLOC(tspack->arena, SpecularTransmission)(T, fresnel, cb != 0.f, architectural));

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(compParams);

	return bsdf;
}
Material* Glass::CreateMaterial(const Transform &xform,
		const ParamSet &mp) {
	boost::shared_ptr<Texture<SWCSpectrum> > Kr(mp.GetSWCSpectrumTexture("Kr", RGBColor(1.f)));
	boost::shared_ptr<Texture<SWCSpectrum> > Kt(mp.GetSWCSpectrumTexture("Kt", RGBColor(1.f)));
	boost::shared_ptr<Texture<float> > index(mp.GetFloatTexture("index", 1.5f));
	boost::shared_ptr<Texture<float> > cbf(mp.GetFloatTexture("cauchyb", 0.f));				// Cauchy B coefficient
	boost::shared_ptr<Texture<float> > film(mp.GetFloatTexture("film", 0.f));				// Thin film thickness in nanometers
	boost::shared_ptr<Texture<float> > filmindex(mp.GetFloatTexture("filmindex", 1.5f));				// Thin film index of refraction
	bool archi = mp.FindOneBool("architectural", false);
	boost::shared_ptr<Texture<float> > bumpMap(mp.GetFloatTexture("bumpmap"));

	// Get Compositing Params
	CompositingParams cP;
	FindCompositingParams(mp, &cP);

	return new Glass(Kr, Kt, index, cbf, film, filmindex, archi, bumpMap, cP);
}

static DynamicLoader::RegisterMaterial<Glass> r("glass");
