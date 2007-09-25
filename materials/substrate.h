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

// substrate.cpp*
#include "lux.h"
#include "material.h"
// Substrate Class Declarations
class Substrate : public Material {
public:
	// Substrate Public Methods
	Substrate(Reference<Texture<Spectrum> > kd, Reference<Texture<Spectrum> > ks,
			Reference<Texture<float> > u, Reference<Texture<float> > v,
			Reference<Texture<float> > bump) {
		Kd = kd;
		Ks = ks;
		nu = u;
		nv = v;
		bumpMap = bump;
	}
	BSDF *GetBSDF(const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const;
	
	static Material * CreateMaterial(const Transform &xform, const TextureParams &mp);
private:
	// Substrate Private Data
	Reference<Texture<Spectrum> > Kd, Ks;
	Reference<Texture<float> > nu, nv;
	Reference<Texture<float> > bumpMap;
};
