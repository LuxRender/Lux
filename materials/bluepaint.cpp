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

// bluepaint.cpp*
#include "bluepaint.h"

using namespace lux;

// BluePaint Method Definitions
BSDF *BluePaint::GetBSDF(const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const {
	// Declare bluepaint coefficients
	static float diffuse[3] = {  0.3094f,    0.39667f,   0.70837f  };
	static float xy0[3] =     {  0.870567f,  0.857255f,  0.670982f };
	static float z0[3] =      {  0.803624f,  0.774290f,  0.586674f };
	static float e0[3] =      { 21.820103f, 18.597755f,  7.472717f };
	static float xy1[3] =     { -0.451218f, -0.406681f, -0.477976f };
	static float z1[3] =      {  0.023123f,  0.017625f,  0.227295f };
	static float e1[3] =      {  2.774499f,  2.581499f,  3.677653f };
	static float xy2[3] =     { -1.031545f, -1.029426f, -1.026588f };
	static float z2[3] =      {  0.706734f,  0.696530f,  0.687715f };
	static float e2[3] =      { 66.899060f, 63.767912f, 57.489181f };
	static SWCSpectrum xy[3] = { SWCSpectrum(Spectrum(xy0)), SWCSpectrum(Spectrum(xy1)), SWCSpectrum(Spectrum(xy2)) };
	static SWCSpectrum z[3] = { SWCSpectrum(Spectrum(z0)), SWCSpectrum(Spectrum(z1)), SWCSpectrum(Spectrum(z2)) };
	static SWCSpectrum e[3] = { SWCSpectrum(Spectrum(e0)), SWCSpectrum(Spectrum(e1)), SWCSpectrum(Spectrum(e2)) };
	// Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
	DifferentialGeometry dgs;
	if (bumpMap)
		Bump(bumpMap, dgGeom, dgShading, &dgs);
	else
		dgs = dgShading;
	BSDF *bsdf = BSDF_ALLOC( BSDF)(dgs, dgGeom.nn);
	bsdf->Add(BSDF_ALLOC( Lafortune)(Spectrum(diffuse), 3, xy, xy, z, e,
		BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)));
	return bsdf;
}
Material* BluePaint::CreateMaterial(const Transform &xform,
		const TextureParams &mp) {
	boost::shared_ptr<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap", 0.f);
	return new BluePaint(bumpMap);
}
