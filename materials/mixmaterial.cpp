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
#include "bxdf.h"
#include "texture.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// MixMaterial Method Definitions
BSDF *MixMaterial::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const DifferentialGeometry &dgGeom,
	const DifferentialGeometry &dgShading,
	const Volume *exterior, const Volume *interior) const {
	MixBSDF *bsdf = ARENA_ALLOC(arena, MixBSDF)(dgShading, dgGeom.nn,
		exterior, interior);
	float amt = amount->Evaluate(sw, dgShading);
	DifferentialGeometry dgS = dgShading;
	mat1->GetShadingGeometry(sw, dgGeom.nn, &dgS);
	bsdf->Add(1.f - amt, mat1->GetBSDF(arena, sw, dgGeom, dgS,
		exterior, interior));
	dgS = dgShading;
	mat2->GetShadingGeometry(sw, dgGeom.nn, &dgS);
	bsdf->Add(amt, mat2->GetBSDF(arena, sw, dgGeom, dgS,
		exterior, interior));
	bsdf->SetCompositingParams(compParams);
	return bsdf;
}
Material* MixMaterial::CreateMaterial(const Transform &xform,
		const ParamSet &mp) {
	boost::shared_ptr<Material> mat1(mp.GetMaterial("namedmaterial1"));
	if (!mat1) {
		LOG(LUX_BADTOKEN, LUX_ERROR)<<"First material of the mix is incorrect";
		return NULL;
	}
	boost::shared_ptr<Material> mat2(mp.GetMaterial("namedmaterial2"));
	if (!mat2) {
		LOG(LUX_BADTOKEN, LUX_ERROR)<<"Second material of the mix is incorrect";
		return NULL;
	}

	boost::shared_ptr<Texture<float> > amount(mp.GetFloatTexture("amount", 0.5f));

	// Get Compositing Params
	CompositingParams cP;
	FindCompositingParams(mp, &cP);
	return new MixMaterial(amount, mat1, mat2, cP);
}

static DynamicLoader::RegisterMaterial<MixMaterial> r("mix");
