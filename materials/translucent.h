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

// translucent.cpp*
#include "lux.h"
#include "material.h"

namespace lux
{

// Translucent Class Declarations
class Translucent : public Material {
public:
	// Translucent Public Methods
	Translucent(boost::shared_ptr<Texture<Spectrum> > kd, boost::shared_ptr<Texture<Spectrum> > ks,
			boost::shared_ptr<Texture<float> > rough,
			boost::shared_ptr<Texture<Spectrum> > refl,
			boost::shared_ptr<Texture<Spectrum> > trans,
			// NOTE - lordcrc - Bugfix, pbrt tracker id 0000078: index of refraction swapped and not recorded
			boost::shared_ptr<Texture<float> > i,
			boost::shared_ptr<Texture<float> > bump) {
		Kd = kd;
		Ks = ks;
		roughness = rough;
		reflect = refl;
		transmit = trans;
		// NOTE - lordcrc - Bugfix, pbrt tracker id 0000078: index of refraction swapped and not recorded
		index = i;
		bumpMap = bump;
	}
	BSDF *GetBSDF(const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const;
	
	static Material * CreateMaterial(const Transform &xform, const TextureParams &mp);
private:
	// Translucent Private Data
	boost::shared_ptr<Texture<Spectrum> > Kd, Ks;
	boost::shared_ptr<Texture<float> > roughness;
	boost::shared_ptr<Texture<Spectrum> > reflect, transmit;
	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000078: index of refraction swapped and not recorded
	boost::shared_ptr<Texture<float> > index;
	boost::shared_ptr<Texture<float> > bumpMap;
};

}//namespace lux
