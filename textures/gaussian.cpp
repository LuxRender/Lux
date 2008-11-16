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

// gaussian.cpp*
#include "gaussian.h"
#include "dynload.h"

using namespace lux;

// GaussianTexture Method Definitions
Texture<float> * GaussianTexture::CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
	return new GaussianFloatTexture<float>(tp.FindFloat("energy", 1.f));
}

Texture<SWCSpectrum> * GaussianTexture::CreateSWCSpectrumTexture(const Transform &tex2world,
		const TextureParams &tp) {
	return new GaussianSpectrumTexture<SWCSpectrum>(tp.FindFloat("wavelength", 550.f),
			tp.FindFloat("width", 50.f), tp.FindFloat("energy", 1.f));
}

static DynamicLoader::RegisterFloatTexture<GaussianTexture> r1("gaussian");
static DynamicLoader::RegisterSWCSpectrumTexture<GaussianTexture> r2("gaussian");
