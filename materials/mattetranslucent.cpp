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

// mattetranslucent.cpp*
#include "mattetranslucent.h"
#include "memory.h"
#include "bxdf.h"
#include "brdftobtdf.h"
#include "lambertian.h"
#include "orennayar.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Matte Method Definitions
BSDF *MatteTranslucent::GetBSDF(MemoryArena &arena,
	const SpectrumWavelengths &sw,
	const DifferentialGeometry &dgGeom,
	const DifferentialGeometry &dgs,
	const Volume *exterior, const Volume *interior) const
{
	// Allocate _BSDF_
	MultiBSDF *bsdf = ARENA_ALLOC(arena, MultiBSDF)(dgs, dgGeom.nn,
		exterior, interior);
	// NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum R = Kr->Evaluate(sw, dgs).Clamp(0.f, 1.f);
	SWCSpectrum T = Kt->Evaluate(sw, dgs).Clamp(0.f, 1.f);
	if (energyConserving)
		T *= SWCSpectrum(1.f) - R;
	float sig = Clamp(sigma->Evaluate(sw, dgs), 0.f, 90.f);

	if (!R.Black()) {
		if (sig == 0.f)
			bsdf->Add(ARENA_ALLOC(arena, Lambertian)(R));
		else
			bsdf->Add(ARENA_ALLOC(arena, OrenNayar)(R, sig));
	}
	if (!T.Black()) {
		BxDF *base;
		if (sig == 0.f)
			base = ARENA_ALLOC(arena, Lambertian)(T);
		else
			base = ARENA_ALLOC(arena, OrenNayar)(T, sig);
		bsdf->Add(ARENA_ALLOC(arena, BRDFToBTDF)(base));
	}

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(compParams);

	return bsdf;
}
Material* MatteTranslucent::CreateMaterial(const Transform &xform,
		const ParamSet &mp) {
	boost::shared_ptr<Texture<SWCSpectrum> > Kr(mp.GetSWCSpectrumTexture("Kr", RGBColor(1.f)));
	boost::shared_ptr<Texture<SWCSpectrum> > Kt(mp.GetSWCSpectrumTexture("Kt", RGBColor(1.f)));
	boost::shared_ptr<Texture<float> > sigma(mp.GetFloatTexture("sigma", 0.f));
	boost::shared_ptr<Texture<float> > bumpMap(mp.GetFloatTexture("bumpmap"));
	bool conserving = mp.FindOneBool("energyconserving", false);

	// Get Compositing Params
	CompositingParams cP;
	FindCompositingParams(mp, &cP);

	return new MatteTranslucent(Kr, Kt, sigma, bumpMap, conserving, cP);
}

static DynamicLoader::RegisterMaterial<MatteTranslucent> r("mattetranslucent");
