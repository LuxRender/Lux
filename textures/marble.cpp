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

// marble.cpp*
#include "marble.h"
#include "dynload.h"

using namespace lux;

// MarbleTexture Method Definitions
Texture<SWCSpectrum> * MarbleTexture::CreateSWCSpectrumTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	// Read mapping coordinates
	TextureMapping3D *imap = TextureMapping3D::Create(tex2world, tp);
	return new MarbleTexture(tp.FindOneInt("octaves", 8),
		tp.FindOneFloat("roughness", .5f),
		tp.FindOneFloat("scale", 1.f),
		tp.FindOneFloat("variation", .2f),
		imap);
}

static DynamicLoader::RegisterSWCSpectrumTexture<MarbleTexture> r2("marble");
