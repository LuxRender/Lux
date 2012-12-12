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
		LOG( LUX_ERROR,LUX_UNIMPLEMENT) << dim << " dimensional checkerboard texture not supported";
		return NULL;
	}
	boost::shared_ptr<Texture<float> > tex1(tp.GetFloatTexture("tex1", 1.f));
	boost::shared_ptr<Texture<float> > tex2(tp.GetFloatTexture("tex2", 0.f));
	if (dim == 2) {
		// Initialize 2D texture mapping _map_ from _tp_
		string aamode = tp.FindOneString("aamode", "none");
		return new Checkerboard2D(TextureMapping2D::Create(tex2world, tp), tex1, tex2, aamode);
	} else {
		TextureMapping3D *imap;
		// Read mapping coordinates
		string coords = tp.FindOneString("coordinates", "global");
		if (coords == "global")
			imap = new GlobalMapping3D(tex2world);
		else if (coords == "local")
			imap = new LocalMapping3D(tex2world);
		else if (coords == "uv")
			imap = new UVMapping3D(tex2world);
		else if (coords == "globalnormal")
			imap = new GlobalNormalMapping3D(tex2world);
		else if (coords == "localnormal")
			imap = new LocalNormalMapping3D(tex2world);
		else
			imap = new GlobalMapping3D(tex2world);
		// Apply texture specified transformation option for 3D mapping
		imap->Apply3DTextureMappingOptions(tp);
		return new Checkerboard3D(imap, tex1, tex2);
	}
}

static DynamicLoader::RegisterFloatTexture<Checkerboard> r("checkerboard");
