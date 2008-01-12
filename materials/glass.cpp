/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// glass.cpp*
#include "glass.h"

using namespace lux;

// Glass Method Definitions
BSDF *Glass::GetBSDF(const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const {
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000078: index of refraction swapped and not recorded
	float ior = index->Evaluate(dgs);
	BSDF *bsdf = BSDF_ALLOC( BSDF)(dgs, dgGeom.nn, ior);
	Spectrum R = Kr->Evaluate(dgs).Clamp();
	Spectrum T = Kt->Evaluate(dgs).Clamp();
	if (!R.Black())
		bsdf->Add(BSDF_ALLOC( SpecularReflection)(R,
			BSDF_ALLOC( FresnelDielectric)(1., ior)));
	if (!T.Black())
		bsdf->Add(BSDF_ALLOC( SpecularTransmission)(T, 1., ior));
	return bsdf;
}
Material* Glass::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	boost::shared_ptr<Texture<Spectrum> > Kr = mp.GetSpectrumTexture("Kr", Spectrum(1.f));
	boost::shared_ptr<Texture<Spectrum> > Kt = mp.GetSpectrumTexture("Kt", Spectrum(1.f));
	boost::shared_ptr<Texture<float> > index = mp.GetFloatTexture("index", 1.5f);
	boost::shared_ptr<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap", 0.f);
	return new Glass(Kr, Kt, index, bumpMap);
}
