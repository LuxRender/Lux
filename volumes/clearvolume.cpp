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

// clearvolume.cpp*
#include "clearvolume.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// ClearVolume Method Definitions
Volume * ClearVolume::CreateVolume(const Transform &volume2world,
		const ParamSet &params) {
	// Initialize common volume region parameters
	boost::shared_ptr<Texture<FresnelGeneral> > fr(params.GetFresnelTexture("fresnel", 1.5f));
	boost::shared_ptr<Texture<SWCSpectrum> > absorption(params.GetSWCSpectrumTexture("absorption", RGBColor(0.f)));

	return new ClearVolume(fr, absorption);
}

static DynamicLoader::RegisterVolume<ClearVolume> r("clear");
