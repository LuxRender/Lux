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
	TextureMapping2D *map = NULL;
	string type = tp.FindOneString("mapping", "uv");
	if (type == "uv") {
		float su = tp.FindOneFloat("uscale", 1.f);
		float sv = tp.FindOneFloat("vscale", 1.f);
		float du = tp.FindOneFloat("udelta", 0.f);
		float dv = tp.FindOneFloat("vdelta", 0.f);
		map = new UVMapping2D(su, sv, du, dv);
	} else if (type == "spherical") {
		float su = tp.FindOneFloat("uscale", 1.f);
		float sv = tp.FindOneFloat("vscale", 1.f);
		float du = tp.FindOneFloat("udelta", 0.f);
		float dv = tp.FindOneFloat("vdelta", 0.f);
		map = new SphericalMapping2D(tex2world.GetInverse(),
		                             su, sv, du, dv);
	} else if (type == "cylindrical") {
		float su = tp.FindOneFloat("uscale", 1.f);
		float du = tp.FindOneFloat("udelta", 0.f);
		map = new CylindricalMapping2D(tex2world.GetInverse(), su, du);
	} else if (type == "planar") {
		map = new PlanarMapping2D(tp.FindOneVector("v1", Vector(1,0,0)),
			tp.FindOneVector("v2", Vector(0,1,0)),
			tp.FindOneFloat("udelta", 0.f),
			tp.FindOneFloat("vdelta", 0.f));
	} else {
		LOG( LUX_ERROR,LUX_UNIMPLEMENT) << "2D texture mapping '" << type << "' unknown";
		map = new UVMapping2D;
	}
	return new BilerpFloatTexture(map,
		tp.FindOneFloat("v00", 0.f), tp.FindOneFloat("v01", 1.f),
		tp.FindOneFloat("v10", 0.f), tp.FindOneFloat("v11", 1.f));
}

Texture<SWCSpectrum>* BilerpSpectrumTexture::CreateSWCSpectrumTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	// Initialize 2D texture mapping _map_ from _tp_
	TextureMapping2D *map = NULL;
	string type = tp.FindOneString("mapping", "uv");
	if (type == "uv") {
		float su = tp.FindOneFloat("uscale", 1.f);
		float sv = tp.FindOneFloat("vscale", 1.f);
		float du = tp.FindOneFloat("udelta", 0.f);
		float dv = tp.FindOneFloat("vdelta", 0.f);
		map = new UVMapping2D(su, sv, du, dv);
	} else if (type == "spherical") {
		float su = tp.FindOneFloat("uscale", 1.f);
		float sv = tp.FindOneFloat("vscale", 1.f);
		float du = tp.FindOneFloat("udelta", 0.f);
		float dv = tp.FindOneFloat("vdelta", 0.f);
		map = new SphericalMapping2D(tex2world.GetInverse(),
		                             su, sv, du, dv);
	} else if (type == "cylindrical") {
		float su = tp.FindOneFloat("uscale", 1.f);
		float du = tp.FindOneFloat("udelta", 0.f);
		map = new CylindricalMapping2D(tex2world.GetInverse(), su, du);
	} else if (type == "planar") {
		map = new PlanarMapping2D(tp.FindOneVector("v1", Vector(1,0,0)),
			tp.FindOneVector("v2", Vector(0,1,0)),
			tp.FindOneFloat("udelta", 0.f),
			tp.FindOneFloat("vdelta", 0.f));
	} else {
		LOG( LUX_ERROR,LUX_UNIMPLEMENT) << "2D texture mapping '" << type << "' unknown";
		map = new UVMapping2D;
	}
	return new BilerpSpectrumTexture(map,
		tp.FindOneRGBColor("v00", 0.f), tp.FindOneRGBColor("v01", 1.f),
		tp.FindOneRGBColor("v10", 0.f), tp.FindOneRGBColor("v11", 1.f));
}

static DynamicLoader::RegisterFloatTexture<BilerpFloatTexture> r1("bilerp");
static DynamicLoader::RegisterSWCSpectrumTexture<BilerpSpectrumTexture> r2("bilerp");
