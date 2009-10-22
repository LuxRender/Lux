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

// regulardata.cpp*
#include "regulardata.h"
#include "error.h"
#include "dynload.h"

using namespace lux;

// RegularDataTexture Method Definitions
Texture<float> * RegularDataTexture::CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
	// Dade - unused parameters
	/*float start =*/ tp.FindFloat("start", 380.f);
	/*float end =*/ tp.FindFloat("end", 720.f);
	u_int dataCount = 0;
	/*const float *data =*/ tp.FindFloats("data", &dataCount);
	if (dataCount < 1) {
		luxError(LUX_MISSINGDATA, LUX_ERROR, "No data in regulardata texture");
	}
	return new RegularDataFloatTexture<float>(1.f);
}

Texture<SWCSpectrum> * RegularDataTexture::CreateSWCSpectrumTexture(const Transform &tex2world,
		const TextureParams &tp) {
	float start = tp.FindFloat("start", 380.f);
	float end = tp.FindFloat("end", 720.f);
	u_int dataCount = 0;
	const float *data = tp.FindFloats("data", &dataCount);
	if (dataCount < 1) {
		luxError(LUX_MISSINGDATA, LUX_ERROR, "No data in regulardata texture");
		float default_data[] = {1.f};
		return new RegularDataSpectrumTexture<SWCSpectrum>(start, end, 1, default_data);
	}
	return new RegularDataSpectrumTexture<SWCSpectrum>(start, end, dataCount, data);
}

static DynamicLoader::RegisterFloatTexture<RegularDataTexture> r1("regulardata");
static DynamicLoader::RegisterSWCSpectrumTexture<RegularDataTexture> r2("regulardata");
