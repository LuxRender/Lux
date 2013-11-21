/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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
#include "doubleside.h"
#include "memory.h"
#include "doublesidebsdf.h"
#include "primitive.h"
#include "texture.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;
using namespace luxrays;

// DoubleSideMaterial Method Definitions
BSDF *DoubleSideMaterial::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Intersection &isect, const DifferentialGeometry &dgShading) const {
	DoubleSideBSDF *bsdf = ARENA_ALLOC(arena, DoubleSideBSDF)(dgShading, isect.dg.nn,
		isect.exterior, isect.interior);

	DifferentialGeometry dgS = dgShading;
	if (useFrontForFrontMat) {
		frontMat->GetShadingGeometry(sw, isect.dg.nn, &dgS);
		bsdf->SetFrontMaterial(frontMat->GetBSDF(arena, sw, isect, dgS));
	} else {
		// I need to flip the geometry normal in order to fool the material
		dgS.nn = -dgS.nn;
		Intersection backIsect = isect;
		backIsect.dg.nn = -backIsect.dg.nn;

		frontMat->GetShadingGeometry(sw, backIsect.dg.nn, &dgS);
		bsdf->SetFrontMaterial(frontMat->GetBSDF(arena, sw, backIsect, dgS));
	}

	dgS = dgShading;
	if (useFrontForBackMat) {
		// I need to flip the geometry normal in order to fool the material
		dgS.nn = -dgS.nn;
		Intersection backIsect = isect;
		backIsect.dg.nn = -backIsect.dg.nn;

		backMat->GetShadingGeometry(sw, backIsect.dg.nn, &dgS);
		bsdf->SetBackMaterial(backMat->GetBSDF(arena, sw, backIsect, dgS));
	} else {
		backMat->GetShadingGeometry(sw, isect.dg.nn, &dgS);
		bsdf->SetBackMaterial(backMat->GetBSDF(arena, sw, isect, dgS));		
	}

	bsdf->SetCompositingParams(&compParams);

	return bsdf;
}

Material *DoubleSideMaterial::CreateMaterial(const Transform &xform,
		const ParamSet &mp) {
	boost::shared_ptr<Material> mat1(mp.GetMaterial("frontnamedmaterial"));
	if (!mat1) {
		LOG( LUX_ERROR,LUX_BADTOKEN)<<"Front side material of the doubleside is incorrect";
		return NULL;
	}

	boost::shared_ptr<Material> mat2(mp.GetMaterial("backnamedmaterial"));
	if (!mat2) {
		LOG( LUX_ERROR,LUX_BADTOKEN)<<"Back side material of the doubleside is incorrect";
		return NULL;
	}

	const bool useFrontForFront = mp.FindOneBool("usefrontforfront", true);
	const bool useFrontForBack = mp.FindOneBool("usefrontforback", true);

	return new DoubleSideMaterial(mat1, mat2, useFrontForFront, useFrontForBack, mp);
}

static DynamicLoader::RegisterMaterial<DoubleSideMaterial> r("doubleside");
