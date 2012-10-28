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

// bilerp.cpp*
#include "bilerp.h"
#include "dynload.h"

using namespace lux;

RGBIllumSPD BilerpSpectrumTexture::whiteRGBIllum;

// BilerpTexture Method Definitions
Texture<float>* BilerpFloatTexture::CreateFloatTexture(const Transform &tex2world, const ParamSet &tp)
{
	// Initialize 2D texture mapping _map_ from _tp_
	return new BilerpFloatTexture(TextureMapping2D::Create(tex2world, tp),
		tp.FindOneFloat("v00", 0.f), tp.FindOneFloat("v01", 1.f),
		tp.FindOneFloat("v10", 0.f), tp.FindOneFloat("v11", 1.f));
}

Texture<SWCSpectrum>* BilerpSpectrumTexture::CreateSWCSpectrumTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	// Initialize 2D texture mapping _map_ from _tp_
	return new BilerpSpectrumTexture(TextureMapping2D::Create(tex2world, tp),
		tp.FindOneRGBColor("v00", 0.f), tp.FindOneRGBColor("v01", 1.f),
		tp.FindOneRGBColor("v10", 0.f), tp.FindOneRGBColor("v11", 1.f));
}

Texture<FresnelGeneral>* BilerpFresnelTexture::CreateFresnelTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	// Initialize 2D texture mapping _map_ from _tp_
	return new BilerpFresnelTexture(TextureMapping2D::Create(tex2world, tp),
		tp.GetFresnelTexture("v00", 1.f), tp.GetFresnelTexture("v01", 1.5f),
		tp.GetFresnelTexture("v10", 1.f), tp.GetFresnelTexture("v11", 1.5f));
}

static DynamicLoader::RegisterFloatTexture<BilerpFloatTexture> r1("bilerp");
static DynamicLoader::RegisterSWCSpectrumTexture<BilerpSpectrumTexture> r2("bilerp");
static DynamicLoader::RegisterFresnelTexture<BilerpFresnelTexture> r3("bilerp");
