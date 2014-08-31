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
		const ParamSet &tp) :
		BlenderTexture3D("BlenderMarbleTexture3D-" + boost::lexical_cast<string>(this), tex2world, tp, TEX_MARBLE) {
		tex.stype = GetMarbleType(tp.FindOneString("type", "soft"));
		tex.noisetype = GetNoiseType(tp.FindOneString("noisetype",
			"soft_noise"));
		tex.noisebasis = (slg::blender::BlenderNoiseBasis) GetNoiseBasis(tp.FindOneString("noisebasis",
			"blender_original"));
		tex.noisebasis2 = GetNoiseShape(tp.FindOneString("noisebasis2",
			"sin"));
		tex.noisesize = tp.FindOneFloat("noisesize", 0.25f);
		tex.noisedepth = tp.FindOneInt("noisedepth", 2);
		tex.turbul = tp.FindOneFloat("turbulence", 5.f);
	}

	static Texture<float> *CreateFloatTexture(const Transform &tex2world,
		const ParamSet &tp) {
		return new BlenderMarbleTexture3D(tex2world, tp);
	}
	const float GetBright() const { return tex.bright; }
	const float GetContrast() const { return tex.contrast; }
	const short GetType() const { return tex.stype; }
	const float GetNoiseT() const { return tex.noisetype; }
	const short GetNoiseB() const { return tex.noisebasis; }
	const short GetNoiseB2() const { return tex.noisebasis2; }
	const float GetNoiseSize() const { return tex.noisesize; }
	const int GetNoiseDepth() const { return tex.noisedepth; };		
	const float GetTurbulence() const { return tex.turbul; }
	const TextureMapping3D *GetTextureMapping3D() const { return mapping; }
};

} // namespace lux
