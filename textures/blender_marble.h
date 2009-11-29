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

// blender_musgrave.cpp*
#include "blender_base.h"
#include "error.h"

namespace lux {

// Dade - BlenderMarbleTexture3D Declarations
class BlenderMarbleTexture3D : public BlenderTexture3D {
public:
	// BlenderMarbleTexture3D Public Methods

	virtual ~BlenderMarbleTexture3D() { }

	BlenderMarbleTexture3D(const Transform &tex2world,
		const TextureParams &tp) :
		BlenderTexture3D(tex2world, tp, TEX_MARBLE) {
		tex.stype = GetMarbleType(tp.FindString("type"));
		tex.noisetype = GetNoiseType(tp.FindString("noisetype"));
		tex.noisebasis = GetNoiseBasis(tp.FindString("noisebasis"));
		tex.noisebasis2 = GetNoiseShape(tp.FindString("noisebasis2"));
		tex.noisesize = tp.FindFloat("noisesize", 0.25f);
		tex.noisedepth = tp.FindInt("noisedepth", 2);
		tex.turbul = tp.FindFloat("turbulence", 5.f);
	}

	static Texture<float> *CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
		return new BlenderMarbleTexture3D(tex2world, tp);
	}
};

} // namespace lux
