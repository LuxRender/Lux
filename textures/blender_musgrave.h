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
// Dade - BlenderMusgraveTexture3D Declarations
class BlenderMusgraveTexture3D : public BlenderTexture3D {
public:
	// BlenderMusgraveTexture3D Public Methods

	virtual ~BlenderMusgraveTexture3D() { }

	BlenderMusgraveTexture3D(const Transform &tex2world,
		const TextureParams &tp) :
		BlenderTexture3D(tex2world, tp, TEX_MUSGRAVE) {
		tex.stype = GetMusgraveType(tp.FindString("type"));
		tex.noisebasis = GetNoiseBasis(tp.FindString("noisebasis"));
		tex.mg_H = tp.FindFloat("h", 1.f);
		tex.mg_lacunarity = tp.FindFloat("lacu", 2.f);
		tex.mg_octaves = tp.FindFloat("octs", 2.f);
		tex.mg_gain = tp.FindFloat("gain", 1.f);
		tex.mg_offset = tp.FindFloat("offset", 1.f);
		tex.noisesize = tp.FindFloat("noisesize", 0.25f);
		tex.ns_outscale = tp.FindFloat("outscale", 1.f);
	}

	static Texture<float> *CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
		return new BlenderMusgraveTexture3D(tex2world, tp);
	}
};

} // namespace lux
