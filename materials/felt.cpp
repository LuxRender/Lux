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

// felt.cpp*
#include "felt.h"

// Felt Method Definitions
BSDF *Felt::GetBSDF(MemoryArena &arena, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const {
	// Declare felt coefficients
	static float diffuse[3] = {  0.025865f,  0.025865f,  0.025865f};
	static float xy0[3] =     { -0.304075f, -0.304075f, -0.304075f};
	static float z0[3] =      { -0.065992f, -0.065992f, -0.065992f};
	static float e0[3] =      {  3.047892f,  3.047892f,  3.047892f};
	static float xy1[3] =     { -0.749561f, -0.749561f, -0.749561f};
	static float z1[3] =      { -1.167929f, -1.167929f, -1.167929f};
	static float e1[3] =      {  6.931827f,  6.931827f,  6.931827f};
	static float xy2[3] =     {  1.004921f,  1.004921f,  1.004921f};
	static float z2[3] =      { -0.205529f, -0.205529f, -0.205529f};
	static float e2[3] =      { 94.117332f, 94.117332f, 94.117332f};
	static Spectrum xy[3] = { Spectrum(xy0), Spectrum(xy1), Spectrum(xy2) };
	static Spectrum z[3] = { Spectrum(z0), Spectrum(z1), Spectrum(z2) };
	static Spectrum e[3] = { Spectrum(e0), Spectrum(e1), Spectrum(e2) };
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	BSDF *bsdf = BSDF_ALLOC(arena, BSDF)(dgs, dgGeom.nn);
	bsdf->Add(BSDF_ALLOC(arena, Lafortune)(Spectrum(diffuse), 3, xy, xy, z, e,
		BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)));
	return bsdf;
}
Material* Felt::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	boost::shared_ptr<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap", 0.f);
	return new Felt(bumpMap);
}
