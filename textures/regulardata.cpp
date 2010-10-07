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
Texture<SWCSpectrum> *RegularDataTexture::CreateSWCSpectrumTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	float start = tp.FindOneFloat("start", 380.f);
	float end = tp.FindOneFloat("end", 720.f);
	u_int dataCount = 0;
	const float *data = tp.FindFloat("data", &dataCount);
	if (dataCount < 2) {
		LOG(LUX_MISSINGDATA, LUX_ERROR)<< "Insufficient data in regulardata texture";
		const float default_data[] = {1.f, 1.f};
		return new RegularDataTexture(start, end, 2, default_data);
	}
	return new RegularDataTexture(start, end, dataCount, data);
}

static DynamicLoader::RegisterSWCSpectrumTexture<RegularDataTexture> r("regulardata");
