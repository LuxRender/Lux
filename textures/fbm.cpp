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

// fbm.cpp*
#include "fbm.h"
#include "dynload.h"

using namespace lux;

// FBmTexture Method Definitions
Texture<float> * FBmTexture::CreateFloatTexture(const Transform &tex2world,
	const ParamSet &tp) {
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

	return new FBmTexture(tp.FindOneInt("octaves", 8),
		tp.FindOneFloat("roughness", .5f), imap);
}

static DynamicLoader::RegisterFloatTexture<FBmTexture> r("fbm");
