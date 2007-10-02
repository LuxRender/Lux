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

// uber.cpp*
#include "lux.h"
#include "material.h"
// UberMaterial Class Declarations
class UberMaterial : public Material {
public:
	// UberMaterial Method Declarations
	UberMaterial(boost::shared_ptr<Texture<Spectrum> > kd,
		boost::shared_ptr<Texture<Spectrum> > ks,
		boost::shared_ptr<Texture<Spectrum> > kr,
		boost::shared_ptr<Texture<float> > rough,
		boost::shared_ptr<Texture<Spectrum> > op,
		boost::shared_ptr<Texture<float> > bump) {
		Kd = kd;
		Ks = ks;
		Kr = kr;
		roughness = rough;
		opacity = op;
		bumpMap = bump;
	}
	BSDF *GetBSDF(MemoryArena &arena, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const;
	
	static Material * CreateMaterial(const Transform &xform, const TextureParams &mp);
private:
	// UberMaterial Private Data
	boost::shared_ptr<Texture<Spectrum> > Kd, Ks, Kr, opacity;
	boost::shared_ptr<Texture<float> > roughness;
	boost::shared_ptr<Texture<float> > bumpMap;
};
