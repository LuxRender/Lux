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

#include <limits>

#include "hitpointcolor.h"
#include "dynload.h"

using namespace lux;

Texture<float> *HitPointAlphaTexture::CreateFloatTexture(const Transform &tex2world,
		const ParamSet &tp) {
	return new HitPointAlphaTexture();
}

Texture<SWCSpectrum> *HitPointRGBColorTexture::CreateSWCSpectrumTexture(const Transform &tex2world,
		const ParamSet &tp) {
	return new HitPointRGBColorTexture();
}

Texture<float> *HitPointGreyTexture::CreateFloatTexture(const Transform &tex2world,
		const ParamSet &tp) {
	int channel = tp.FindOneInt("channel", -1);

	return new HitPointGreyTexture(((channel != 0) && (channel != 1) && (channel != 2)) ?
		std::numeric_limits<u_int>::max() : static_cast<u_int>(channel));
}

static DynamicLoader::RegisterFloatTexture<HitPointAlphaTexture> r1("hitpointalpha");
static DynamicLoader::RegisterSWCSpectrumTexture<HitPointRGBColorTexture> r2("hitpointcolor");
static DynamicLoader::RegisterFloatTexture<HitPointGreyTexture> r3("hitpointgrey");
