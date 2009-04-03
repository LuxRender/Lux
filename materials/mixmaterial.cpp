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

// mixmaterial.cpp*
#include "mixmaterial.h"
#include "bxdf.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// MixMaterial Method Definitions
BSDF *MixMaterial::GetBSDF(const TsPack *tspack, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading, float u) const {
	float amt = amount->Evaluate(tspack, dgShading);
	if(u < amt)
		return child1->GetBSDF(tspack, dgGeom, dgShading, u / amt);
	else
		return child2->GetBSDF(tspack, dgGeom, dgShading, (u - amt) / (1.f - amt));
}
Material* MixMaterial::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	string namedmaterial1 = mp.FindString("namedmaterial1"); // discarded as these are passed trough Context::Shape()
    string namedmaterial2 = mp.FindString("namedmaterial2"); // discarded as these are passed trough Context::Shape()
	boost::shared_ptr<Texture<float> > amount = mp.GetFloatTexture("amount", 0.5f);

	return new MixMaterial(amount);
}

static DynamicLoader::RegisterMaterial<MixMaterial> r("mix");
