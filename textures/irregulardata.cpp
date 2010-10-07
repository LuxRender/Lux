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
Texture<SWCSpectrum> *IrregularDataTexture::CreateSWCSpectrumTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	u_int wlCount = 0;
	const float *wl = tp.FindFloat("wavelengths", &wlCount);
	u_int dataCount = 0;
	const float *data = tp.FindFloat("data", &dataCount);
	if (wlCount != dataCount) {
		LOG(LUX_BADTOKEN, LUX_ERROR) << "Number of wavelengths '" << wlCount <<
			"' does not match number of data values '" <<
			dataCount << "' in irregulardata texture definition.";
		wlCount = dataCount = 0;
	}
	if (dataCount < 2 || wlCount < 2) {
		LOG(LUX_MISSINGDATA, LUX_ERROR)<< "Insufficient data in irregulardata texture";
		const float default_wl[] = {380.f, 720.f};
		const float default_data[] = {1.f, 1.f};
		return new IrregularDataTexture(2, default_wl, default_data);
	}
	return new IrregularDataTexture(dataCount, wl, data);
}

static DynamicLoader::RegisterSWCSpectrumTexture<IrregularDataTexture> r("irregulardata");
