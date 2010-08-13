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

// glass2.cpp*
#include "glass2.h"
#include "memory.h"
#include "bxdf.h"
#include "specularreflection.h"
#include "speculartransmission.h"
#include "fresneldielectric.h"
#include "texture.h"
#include "volume.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Glass Method Definitions
BSDF *Glass2::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const DifferentialGeometry &dgGeom,
	const DifferentialGeometry &dgs,
	const Volume *exterior, const Volume *interior) const
{
	// Allocate _BSDF_
	Fresnel *fresnel;
	if (exterior) {
		const FresnelGeneral fre(exterior->Fresnel(sw, dgs.p,
			Vector(dgs.nn)));
		if (interior) {
			const FresnelGeneral fri(interior->Fresnel(sw, dgs.p,
				Vector(dgs.nn)));
			SWCSpectrum fer, fir, f;
			fre.ComplexEvaluate(sw, &fer, &f);
			fri.ComplexEvaluate(sw, &fir, &f);
			const float ior = fri.Index(sw) / fre.Index(sw);
			fresnel = ARENA_ALLOC(arena, FresnelDielectric)
				(ior, fir / fer, SWCSpectrum(0.f));
		} else {
			SWCSpectrum fer, f;
			fre.ComplexEvaluate(sw, &fer, &f);
			const float ior = 1.f / fre.Index(sw);
			fresnel = ARENA_ALLOC(arena, FresnelDielectric)
				(ior, SWCSpectrum(1.f) / fer, SWCSpectrum(0.f));
		}
	} else if (interior)
		fresnel = ARENA_ALLOC(arena, FresnelGeneral)
			(interior->Fresnel(sw, dgs.p, Vector(dgs.nn)));
	else
		fresnel = ARENA_ALLOC(arena, FresnelDielectric)(1.f,
			SWCSpectrum(1.f), SWCSpectrum(0.f));

	MultiBSDF *bsdf = ARENA_ALLOC(arena, MultiBSDF)(dgs, dgGeom.nn,
		exterior, interior);
	if (architectural)
		bsdf->Add(ARENA_ALLOC(arena,
			SimpleArchitecturalReflection)(fresnel));
	else
		bsdf->Add(ARENA_ALLOC(arena,
			SimpleSpecularReflection)(fresnel));
	bsdf->Add(ARENA_ALLOC(arena,
		SimpleSpecularTransmission)(fresnel, dispersion, architectural));

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(compParams);

	return bsdf;
}

Material* Glass2::CreateMaterial(const Transform &xform,
	const ParamSet &mp)
{
	bool archi = mp.FindOneBool("architectural", false);
	bool disp = mp.FindOneBool("dispersion", false);
	boost::shared_ptr<Texture<float> > bumpMap(mp.GetFloatTexture("bumpmap"));

	// Get Compositing Params
	CompositingParams cP;
	FindCompositingParams(mp, &cP);

	return new Glass2(archi, disp, bumpMap, cP);
}

static DynamicLoader::RegisterMaterial<Glass2> r("glass2");
