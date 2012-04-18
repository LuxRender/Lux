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

// mixmaterial.cpp*
#include "mixmaterial.h"
#include "memory.h"
#include "mixbsdf.h"
#include "primitive.h"
#include "texture.h"
#include "paramset.h"
#include "dynload.h"
#include "color.h"

using namespace lux;

// MixMaterial Method Definitions
BSDF *MixMaterial::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Intersection &isect, const DifferentialGeometry &dgShading) const {
	SWCSpectrum bcolor = Sc->Evaluate(sw, dgShading);
	float bscale = dgShading.Scale;
	MixBSDF *bsdf = ARENA_ALLOC(arena, MixBSDF)(dgShading, isect.dg.nn,
		isect.exterior, isect.interior, bcolor, bscale);
	float amt = amount->Evaluate(sw, dgShading);
	DifferentialGeometry dgS = dgShading;
	mat1->GetShadingGeometry(sw, isect.dg.nn, &dgS);
	bsdf->Add(1.f - amt, mat1->GetBSDF(arena, sw, isect, dgS));
	dgS = dgShading;
	mat2->GetShadingGeometry(sw, isect.dg.nn, &dgS);
	bsdf->Add(amt, mat2->GetBSDF(arena, sw, isect, dgS));
	bsdf->SetCompositingParams(&compParams);
	return bsdf;
}
Material* MixMaterial::CreateMaterial(const Transform &xform,
		const ParamSet &mp) {
	boost::shared_ptr<Material> mat1(mp.GetMaterial("namedmaterial1"));
	if (!mat1) {
		LOG( LUX_ERROR,LUX_BADTOKEN)<<"First material of the mix is incorrect";
		return NULL;
	}
	boost::shared_ptr<Material> mat2(mp.GetMaterial("namedmaterial2"));
	if (!mat2) {
		LOG( LUX_ERROR,LUX_BADTOKEN)<<"Second material of the mix is incorrect";
		return NULL;
	}

	boost::shared_ptr<Texture<SWCSpectrum> > Sc(mp.GetSWCSpectrumTexture("Sc", RGBColor(.9f)));
	boost::shared_ptr<Texture<float> > amount(mp.GetFloatTexture("amount", 0.5f));

	return new MixMaterial(amount, mat1, mat2, mp, Sc);
}

static DynamicLoader::RegisterMaterial<MixMaterial> r("mix");
