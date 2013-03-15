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

// SMISRA - BlenderDistortedNoiseTexture3D Declarations
class BlenderDistortedNoiseTexture3D : public BlenderTexture3D {
public:
	// BlenderDistortedNoiseTexture3D Public Methods

	virtual ~BlenderDistortedNoiseTexture3D() { }

	BlenderDistortedNoiseTexture3D(const Transform &tex2world,
		const ParamSet &tp) :
		BlenderTexture3D("BlenderDistortedNoiseTexture3D-" + boost::lexical_cast<string>(this), tex2world, tp, TEX_DISTNOISE) {
		tex.noisebasis2 = GetNoiseBasis(tp.FindOneString("type",
			"blender_original"));
		tex.noisebasis = GetNoiseBasis(tp.FindOneString("noisebasis",
			"blender_original"));
		tex.noisesize = tp.FindOneFloat("noisesize", 0.25f);
		tex.dist_amount = tp.FindOneFloat("distamount", 1.f);
		tex.nabla = tp.FindOneFloat("nabla", 0.025f);
	}

	static Texture<float> *CreateFloatTexture(const Transform &tex2world,
		const ParamSet &tp) {
		return new BlenderDistortedNoiseTexture3D(tex2world, tp);
	}
};

} // namespace lux
