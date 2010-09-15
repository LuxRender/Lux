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

// velvet.cpp*
#include "asperity.h"
#include "memory.h"
#include "bxdf.h"
#include "velvet.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Velvet Method Definitions
BSDF *Velvet::GetBSDF(const TsPack *tspack, const DifferentialGeometry &dgGeom,
	const DifferentialGeometry &dgs,
	const Volume *exterior, const Volume *interior) const
{
	// Allocate _BSDF_
	SWCSpectrum r = Kd->Evaluate(tspack, dgs).Clamp(0.f, 1.f);
	
	float p1 = Clamp(P1->Evaluate(tspack, dgs), -100.f, 100.f);
	float p2 = Clamp(P2->Evaluate(tspack, dgs), -100.f, 100.f);
	float p3 = Clamp(P3->Evaluate(tspack, dgs), -100.f, 100.f);
	
	float thickness = Clamp(Thickness->Evaluate(tspack, dgs), 0.0f, 1.f);
	
	BxDF *bxdf = ARENA_ALLOC(tspack->arena, Asperity)(r, p1, p2, p3, thickness);

	SingleBSDF *bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dgs,
		dgGeom.nn, bxdf, exterior, interior);

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(compParams);

	return bsdf;
}

Material* Velvet::CreateMaterial(const Transform &xform,
		const ParamSet &mp) {
	boost::shared_ptr<Texture<SWCSpectrum> > Kd(mp.GetSWCSpectrumTexture("Kd", RGBColor(.3f)));
	boost::shared_ptr<Texture<float> > P1(mp.GetFloatTexture("p1", -2.f));
	boost::shared_ptr<Texture<float> > P2(mp.GetFloatTexture("p2", 20.f));
	boost::shared_ptr<Texture<float> > P3(mp.GetFloatTexture("p3", 2.f));
	boost::shared_ptr<Texture<float> > Thickness(mp.GetFloatTexture("thickness", 0.1f));
	boost::shared_ptr<Texture<float> > bumpMap(mp.GetFloatTexture("bumpmap"));
	// Get Compositing Params
	CompositingParams cP;
	FindCompositingParams(mp, &cP);
	return new Velvet(Kd, P1, P2, P3, Thickness, bumpMap, cP);
}

static DynamicLoader::RegisterMaterial<Velvet> r("velvet");
