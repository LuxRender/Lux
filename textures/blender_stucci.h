/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

#include "blender_base.h"
#include "error.h"

namespace lux {
// BlenderStucciTexture3D Declarations
class BlenderStucciTexture3D : public BlenderTexture3D {
public:
	// BlenderStucciTexture3D Public Methods

	virtual ~BlenderStucciTexture3D() { }

	BlenderStucciTexture3D(const Transform &tex2world,
		const ParamSet &tp) :
		BlenderTexture3D("BlenderStucciTexture3D-" + boost::lexical_cast<string>(this), tex2world, tp, TEX_STUCCI) {
		tex.stype = GetStucciType(tp.FindOneString("type", "plastic"));
		tex.noisetype = GetNoiseType(tp.FindOneString("noisetype",
			"soft_noise"));
		tex.noisebasis = GetNoiseBasis(tp.FindOneString("noisebasis",
			"blender_original"));
		tex.noisesize = tp.FindOneFloat("noisesize", 0.25f);;
		tex.turbul = tp.FindOneFloat("turbulence", 5.f);
	}

	static Texture<float> *CreateFloatTexture(const Transform &tex2world,
		const ParamSet &tp) {
		return new BlenderStucciTexture3D(tex2world, tp);
	}

	const float GetBright() const { return tex.bright; }
	const float GetContrast() const { return tex.contrast; }
	const short GetNoiseT() const { return tex.noisetype; }
	const float GetNoiseSize() const { return tex.noisesize; }
	const float GetTurbulence() const { return tex.turbul; }
	const short GetType() const { return tex.stype; }
	const short GetNoiseB() const { return tex.noisebasis; }
	const TextureMapping3D *GetTextureMapping3D() const { return mapping; }
};

} // namespace lux
