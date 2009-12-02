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

// blender_wood.cpp*
#include "blender_base.h"
#include "error.h"

namespace lux {

// Dade - BlenderWoodTexture3D Declarations
class BlenderWoodTexture3D : public BlenderTexture3D {
public:
	// BlenderWoodTexture3D Public Methods

	virtual ~BlenderWoodTexture3D() { }

	BlenderWoodTexture3D(const Transform &tex2world,
		const TextureParams &tp) :
		BlenderTexture3D(tex2world, tp, TEX_WOOD) {
		tex.stype = GetWoodType(tp.FindString("type"));
		tex.noisetype = GetNoiseType(tp.FindString("noisetype"));
		tex.noisebasis = GetNoiseBasis(tp.FindString("noisebasis"));
		tex.noisebasis2 = GetNoiseShape(tp.FindString("noisebasis2"));
		tex.noisesize = tp.FindFloat("noisesize", 0.25f);
		tex.turbul = tp.FindFloat("turbulence", 5.0f);
	}

	static Texture<float> *CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
		return new BlenderWoodTexture3D(tex2world, tp);
	}
};

} // namespace lux
