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

// cauchytexture.cpp*
#include "cauchytexture.h"
#include "dynload.h"

using namespace lux;

// CauchyTexture Method Definitions
Texture<const Fresnel *> *CauchyTexture::CreateFresnelTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	const float cauchyb = tp.FindOneFloat("cauchyb", 0.f);
	const float index = tp.FindOneFloat("index", -1.f);
	float cauchya;
	if (index > 0.f)
		cauchya = tp.FindOneFloat("cauchya", index - cauchyb * 1e6f /
			(WAVELENGTH_END * WAVELENGTH_START));
	else
		cauchya = tp.FindOneFloat("cauchya", 1.5f);
	return new CauchyTexture(cauchya, cauchyb);
}

static DynamicLoader::RegisterFresnelTexture<CauchyTexture> r("cauchy");
