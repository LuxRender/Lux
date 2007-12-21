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

// matte.cpp*
#include "lux.h"
#include "material.h"

namespace lux
{

// Matte Class Declarations
class Matte : public Material {
public:
	// Matte Public Methods
	Matte(boost::shared_ptr<Texture<Spectrum> > kd,
			boost::shared_ptr<Texture<float> > sig,
			boost::shared_ptr<Texture<float> > bump) {
		Kd = kd;
		sigma = sig;
		bumpMap = bump;
	}
	BSDF *GetBSDF(MemoryArena &arena, const DifferentialGeometry &dgGeom,
	              const DifferentialGeometry &dgShading) const;
	              
	static Material * CreateMaterial(const Transform &xform, const TextureParams &mp);
private:
	// Matte Private Data
	boost::shared_ptr<Texture<Spectrum> > Kd;
	boost::shared_ptr<Texture<float> > sigma, bumpMap;
};

}//namespace lux

