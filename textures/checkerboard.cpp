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

// checkerboard.cpp*
#include "checkerboard.h"
#include "dynload.h"

using namespace lux;

// CheckerboardTexture Method Definitions
Texture<float> * Checkerboard::CreateFloatTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	int dim = tp.FindOneInt("dimension", 2);
	if (dim != 2 && dim != 3) {
		std::stringstream ss;
		ss << dim << " dimensional checkerboard texture not supported";
		luxError(LUX_UNIMPLEMENT, LUX_ERROR, ss.str().c_str());
		return NULL;
	}
	boost::shared_ptr<Texture<float> > tex1(tp.GetFloatTexture("tex1", 1.f));
	boost::shared_ptr<Texture<float> > tex2(tp.GetFloatTexture("tex2", 0.f));
	if (dim == 2) {
		// Initialize 2D texture mapping _map_ from _tp_
		TextureMapping2D *map = NULL;
		string type = tp.FindOneString("mapping", "uv");
		if (type == "uv") {
			float su = tp.FindOneFloat("uscale", 1.f);
			float sv = tp.FindOneFloat("vscale", 1.f);
			float du = tp.FindOneFloat("udelta", 0.f);
			float dv = tp.FindOneFloat("vdelta", 0.f);
			map = new UVMapping2D(su, sv, du, dv);
		} else if (type == "spherical") {
			float su = tp.FindOneFloat("uscale", 1.f);
			float sv = tp.FindOneFloat("vscale", 1.f);
			float du = tp.FindOneFloat("udelta", 0.f);
			float dv = tp.FindOneFloat("vdelta", 0.f);
			map = new SphericalMapping2D(tex2world.GetInverse(),
			                             su, sv, du, dv);
		} else if (type == "cylindrical") {
		float su = tp.FindOneFloat("uscale", 1.f);
		float du = tp.FindOneFloat("udelta", 0.f);
		map = new CylindricalMapping2D(tex2world.GetInverse(), su, du);
		} else if (type == "planar") {
			map = new PlanarMapping2D(tp.FindOneVector("v1",
				Vector(1, 0, 0)),
				tp.FindOneVector("v2", Vector(0, 1, 0)),
				tp.FindOneFloat("udelta", 0.f),
				tp.FindOneFloat("vdelta", 0.f));
		} else {
			std::stringstream ss;
			ss << "2D texture mapping '" << type << "' unknown";
			luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
			map = new UVMapping2D;
		}
		string aamode = tp.FindOneString("aamode", "none");
		return new Checkerboard2D(map, tex1, tex2, aamode);
	} else {
		// Initialize 3D texture mapping _map_ from _tp_
		IdentityMapping3D *imap = new IdentityMapping3D(tex2world);
		// Apply texture specified transformation option for 3D mapping
		imap->Apply3DTextureMappingOptions(tp);
		return new Checkerboard3D(imap, tex1, tex2);
	}
}

static DynamicLoader::RegisterFloatTexture<Checkerboard> r("checkerboard");
