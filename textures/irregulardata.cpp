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

// irregulardata.cpp*
#include "irregulardata.h"
#include "error.h"
#include "dynload.h"

using namespace lux;

// IrregularDataTexture Method Definitions
Texture<float> * IrregularDataTexture::CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
	int wlCount = 0;
	const float *wl = tp.FindFloats("wavelengths", &wlCount);
	int dataCount = 0;
	const float *data = tp.FindFloats("data", &dataCount);
	if(wlCount != dataCount) {
		std::stringstream ss;
        ss << "Number of wavelengths '" << wlCount << "' does not match number of data values '"
			<< dataCount <<"' in irregulardata texture definition.";
        luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
	}
	if (dataCount < 1 || wlCount < 1) {
		luxError(LUX_MISSINGDATA, LUX_ERROR, "No data in irregulardata texture");
	}
	return new IrregularDataFloatTexture<float>(1.f);
}

Texture<SWCSpectrum> * IrregularDataTexture::CreateSWCSpectrumTexture(const Transform &tex2world,
		const TextureParams &tp) {
	int wlCount = 0;
	const float *wl = tp.FindFloats("wavelengths", &wlCount);
	int dataCount = 0;
	const float *data = tp.FindFloats("data", &dataCount);
	if(wlCount != dataCount) {
		std::stringstream ss;
        ss << "Number of wavelengths '" << wlCount << "' does not match number of data values '"
			<< dataCount <<"' in irregulardata texture definition.";
        luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
	}
	if (dataCount < 1 || wlCount < 1) {
		luxError(LUX_MISSINGDATA, LUX_ERROR, "No data in irregulardata texture");
		float default_wl[] = {380.f, 720.f};
		float default_data[] = {1.f, 1.f};
		return new IrregularDataSpectrumTexture<SWCSpectrum>(2, default_wl, default_data);
	}
	return new IrregularDataSpectrumTexture<SWCSpectrum>(dataCount, wl, data);
}

static DynamicLoader::RegisterFloatTexture<IrregularDataTexture> r1("irregulardata");
static DynamicLoader::RegisterSWCSpectrumTexture<IrregularDataTexture> r2("irregulardata");
