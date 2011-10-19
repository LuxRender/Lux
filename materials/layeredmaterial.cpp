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

// layeredmaterial.cpp*
#include "layeredmaterial.h"
#include "layeredbsdf.h"
#include "memory.h"
#include "bxdf.h"
#include "primitive.h"
#include "texture.h"
#include "paramset.h"
#include "dynload.h"
#include "color.h"

using namespace lux;

// MixMaterial Method Definitions
BSDF *LayeredMaterial::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Intersection &isect, const DifferentialGeometry &dgShading) const {
	
	LayeredBSDF *bsdf=ARENA_ALLOC(arena, LayeredBSDF)(dgShading, isect.dg.nn,
		isect.exterior, isect.interior);
	
	if (mat1) { // mat1
		DifferentialGeometry dgS = dgShading;
		mat1->GetShadingGeometry(sw, isect.dg.nn, &dgS);
		BSDF *bsdfmat=mat1->GetBSDF(arena,sw,isect, dgS);

		float op = 1.0f;
		if (opacity1) {
			op= opacity1->Evaluate(sw, dgS);
		}
		else { LOG( LUX_ERROR,LUX_BADTOKEN)<<"No opacity value found for mat 1 - using 1.0"; } 
		//LOG( LUX_ERROR,LUX_BADTOKEN)<<"opacity mat 1 "<<op;
		if (op>.0f) {
			bsdf->Add(bsdfmat,op);
		}
	}

	if (mat2) { // mat2
		DifferentialGeometry dgS = dgShading;
		mat2->GetShadingGeometry(sw, isect.dg.nn, &dgS);
		BSDF *bsdfmat=mat2->GetBSDF(arena,sw,isect, dgS);
		float op = 1.0f;
		if (opacity2) {
			op= opacity2->Evaluate(sw, dgS);
		}
		else { LOG( LUX_ERROR,LUX_BADTOKEN)<<"No opacity value found for mat 2 - using 1.0"; } 
		if (op>.0f) {
			bsdf->Add(bsdfmat,op);
		}
	}
	if (mat3) { // mat3
		DifferentialGeometry dgS = dgShading;
		mat3->GetShadingGeometry(sw, isect.dg.nn, &dgS);
		BSDF *bsdfmat=mat3->GetBSDF(arena,sw,isect, dgS);
		float op = 1.0f;
		if (opacity3) {
			op= opacity3->Evaluate(sw, dgS);
		}
		else { LOG( LUX_ERROR,LUX_BADTOKEN)<<"No opacity value found for mat 3 - using 1.0"; } 
		if (op>.0f) {
			bsdf->Add(bsdfmat,op);
		}
	}
	if (mat4) { // mat4
		DifferentialGeometry dgS = dgShading;
		mat4->GetShadingGeometry(sw, isect.dg.nn, &dgS);
		BSDF *bsdfmat=mat4->GetBSDF(arena,sw,isect, dgS);
		float op = 1.0f;
		if (opacity4) {
			op= opacity4->Evaluate(sw, dgS);
		}
		else { LOG( LUX_ERROR,LUX_BADTOKEN)<<"No opacity value found for mat 4 - using 1.0"; } 
		if (op>.0f) {
			bsdf->Add(bsdfmat,op);
		}
	}

	bsdf->SetCompositingParams(&compParams);
	//LOG( LUX_ERROR,LUX_BADTOKEN)<<"Layered getbsdf called";
	
	return bsdf;

}
Material* LayeredMaterial::CreateMaterial(const Transform &xform,
		const ParamSet &mp) {
	
	
	boost::shared_ptr<Material> mat1(mp.GetMaterial("namedmaterial1"));
	boost::shared_ptr<Material> mat2(mp.GetMaterial("namedmaterial2"));
	boost::shared_ptr<Material> mat3(mp.GetMaterial("namedmaterial3"));
	boost::shared_ptr<Material> mat4(mp.GetMaterial("namedmaterial4"));
	
	boost::shared_ptr<Texture<float> > opacity1(mp.GetFloatTexture("opacity1",1.0f));
	boost::shared_ptr<Texture<float> > opacity2(mp.GetFloatTexture("opacity2",1.0f));
	boost::shared_ptr<Texture<float> > opacity3(mp.GetFloatTexture("opacity3",1.0f));
	boost::shared_ptr<Texture<float> > opacity4(mp.GetFloatTexture("opacity4",1.0f));
	
	return new LayeredMaterial(mp,mat1,mat2,mat3,mat4,opacity1,opacity2,opacity3,opacity4);
}
/*
void LayeredMaterial::setMaterial(int slot, boost::shared_ptr<Material> &mat){
	if (slot==0) { mat1=mat; numset=1;}
	if (slot==1) { mat2=mat; numset=2;}
	if (slot==2) { mat3=mat; numset=3;}
	if (slot==3) { mat4=mat; numset=4;}
	return;
}*/

static DynamicLoader::RegisterMaterial<LayeredMaterial> r("layered");

