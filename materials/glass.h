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
#include "lux.h"
#include "material.h"
// Glass Class Declarations
class Glass : public Material {
public:
	// Glass Public Methods
	Glass(Reference<Texture<Spectrum> > r, Reference<Texture<Spectrum> > t,
			Reference<Texture<float> > i, Reference<Texture<float> > bump) {
		Kr = r;
		Kt = t;
		index = i;
		bumpMap = bump;
	}
	BSDF *GetBSDF(MemoryArena &arena, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const;
	
	static Material * CreateMaterial(const Transform &xform, const TextureParams &mp);
private:
	// Glass Private Data
	Reference<Texture<Spectrum> > Kr, Kt;
	Reference<Texture<float> > index;
	Reference<Texture<float> > bumpMap;
};
