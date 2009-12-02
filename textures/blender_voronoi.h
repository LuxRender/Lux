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

#include "blender_base.h"
#include "error.h"

namespace lux {

class BlenderVoronoiTexture3D : public BlenderTexture3D {
public:
	// BlenderVoronoiTexture3D Public Methods

	virtual ~BlenderVoronoiTexture3D() { }

	BlenderVoronoiTexture3D(const Transform &tex2world,
		const TextureParams &tp) :
		BlenderTexture3D(tex2world, tp, TEX_VORONOI) {
		tex.vn_distm = GetVoronoiType(tp.FindString("distmetric"));
		tex.vn_coltype = tp.FindInt("coltype", 0);
		tex.vn_mexp = tp.FindFloat("minkowsky_exp", 2.5f);
		tex.ns_outscale = tp.FindFloat("outscale", 1.f);
		tex.noisesize = tp.FindFloat("noisesize", 0.25f);
		tex.nabla = tp.FindFloat("nabla", 0.025f);;
		tex.vn_w1 = tp.FindFloat("w1", 1.f);
		tex.vn_w2 = tp.FindFloat("w2", 0.f);
		tex.vn_w3 = tp.FindFloat("w3", 0.f);
		tex.vn_w4 = tp.FindFloat("w4", 0.f);
	}

	static Texture<float> *CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
		return new BlenderVoronoiTexture3D(tex2world, tp);
	}
};

} // namespace lux
