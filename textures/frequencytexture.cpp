/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

// frequency.cpp*
#include "frequencytexture.h"
#include "dynload.h"

using namespace lux;

// FrequencyTexture Method Definitions
Texture<float> * FrequencyTexture::CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
	return new FrequencyFloatTexture<float>(tp.FindFloat("energy", 1.f));
}

Texture<SWCSpectrum> * FrequencyTexture::CreateSWCSpectrumTexture(const Transform &tex2world,
		const TextureParams &tp) {
	return new FrequencySpectrumTexture<SWCSpectrum>(tp.FindFloat("freq", .03f),
			tp.FindFloat("phase", .5f), tp.FindFloat("energy", 1.f));
}

static DynamicLoader::RegisterFloatTexture<FrequencyTexture> r1("frequency");
static DynamicLoader::RegisterSWCSpectrumTexture<FrequencyTexture> r2("frequency");
