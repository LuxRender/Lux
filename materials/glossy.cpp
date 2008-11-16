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

// Glossy material - based on previous/pbrt 'substrate' material using optional absorption

// glossy.cpp*
#include "glossy.h"
#include "bxdf.h"
#include "fresnelblend.h"
#include "blinn.h"
#include "anisotropic.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Glossy Method Definitions
BSDF *Glossy::GetBSDF(const TsPack *tspack, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading, float) const {
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	BSDF *bsdf = BSDF_ALLOC(tspack, BSDF)(dgs, dgGeom.nn);
    // NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	SWCSpectrum d = Kd->Evaluate(tspack, dgs).Clamp(0.f, 1.f);
	SWCSpectrum s = Ks->Evaluate(tspack, dgs).Clamp(0.f, 1.f);
	SWCSpectrum a = Ka->Evaluate(tspack, dgs).Clamp(0.f, 1.f);

	float u = nu->Evaluate(tspack, dgs);
	float v = nv->Evaluate(tspack, dgs);

	float ld = depth->Evaluate(tspack, dgs);

	if(u == v)
		bsdf->Add(BSDF_ALLOC(tspack, FresnelBlend)(d, s, a, ld, BSDF_ALLOC(tspack, Blinn)(1.f/u)));
	else
		bsdf->Add(BSDF_ALLOC(tspack, FresnelBlend)(d, s, a, ld, BSDF_ALLOC(tspack, Anisotropic)(1.f/u, 1.f/v)));

	return bsdf;
}
Material* Glossy::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	boost::shared_ptr<Texture<SWCSpectrum> > Kd = mp.GetSWCSpectrumTexture("Kd", RGBColor(.5f));
	boost::shared_ptr<Texture<SWCSpectrum> > Ks = mp.GetSWCSpectrumTexture("Ks", RGBColor(.5f));
	boost::shared_ptr<Texture<SWCSpectrum> > Ka = mp.GetSWCSpectrumTexture("Ka", RGBColor(.0f));
	boost::shared_ptr<Texture<float> > d = mp.GetFloatTexture("d", .0f);
	boost::shared_ptr<Texture<float> > uroughness = mp.GetFloatTexture("uroughness", .1f);
	boost::shared_ptr<Texture<float> > vroughness = mp.GetFloatTexture("vroughness", .1f);
	boost::shared_ptr<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap");
	return new Glossy(Kd, Ks, Ka, d, uroughness, vroughness, bumpMap);
}

static DynamicLoader::RegisterMaterial<Glossy> r("glossy");
static DynamicLoader::RegisterMaterial<Glossy> r1("substrate"); // Backwards compatibility for 'substrate'
static DynamicLoader::RegisterMaterial<Glossy> r2("plastic");   // Backwards compaticility for removed 'plastic'
