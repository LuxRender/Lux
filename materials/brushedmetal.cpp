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

// brushedmetal.cpp*
#include "brushedmetal.h"

// BrushedMetal Method Definitions
BSDF *BrushedMetal::GetBSDF(const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const {
	// Declare brushedmetal coefficients
	static float diffuse[3] = { 0, 0, 0 };
	static float xy0[3] =     {  -1.11854f, -1.11845f, -1.11999f  };
	static float z0[3] =      {   1.01272f,  1.01469f,  1.01942f  };
	static float e0[3] =      {  15.8708f,  15.6489f,  15.4571f   };
	static float xy1[3] =     {  -1.05334f, -1.06409f, -1.08378f  };
	static float z1[3] =      {   0.69541f,  0.662178f, 0.626672f };
	static float e1[3] =      { 111.267f,   88.9222f,  65.2179f   };
	static float xy2[3] =     {  -1.01684f, -1.01635f, -1.01529f  };
	static float z2[3] =      {   1.00132f,  1.00112f,  1.00108f  };
	static float e2[3] =      { 180.181f,  184.152f,  195.773f    };
	static Spectrum xy[3] = { Spectrum(xy0), Spectrum(xy1), Spectrum(xy2) };
	static Spectrum z[3] = { Spectrum(z0), Spectrum(z1), Spectrum(z2) };
	static Spectrum e[3] = { Spectrum(e0), Spectrum(e1), Spectrum(e2) };
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	BSDF *bsdf = BSDF_ALLOC(BSDF)(dgs, dgGeom.nn);
	bsdf->Add(BSDF_ALLOC(Lafortune)(Spectrum(*diffuse), 3, xy, xy, z, e,
		BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)));
	return bsdf;
}
Material* BrushedMetal::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	Reference<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap", 0.f);
	return new BrushedMetal(bumpMap);
}
