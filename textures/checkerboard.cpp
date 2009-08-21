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

// checkerboard.cpp*
#include "checkerboard.h"
#include "dynload.h"


namespace lux
{
MethodType aaMethod;
}

using namespace lux;

// CheckerboardTexture Method Definitions
Texture<float> * Checkerboard::CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
	int dim = tp.FindInt("dimension", 2);
	if (dim != 2 && dim != 3) {
		//Error("%d dimensional checkerboard texture not supported", dim);
		std::stringstream ss;
		ss<<dim<<" dimensional checkerboard texture not supported";
		luxError(LUX_UNIMPLEMENT,LUX_ERROR,ss.str().c_str());
		return NULL;
	}
	boost::shared_ptr<Texture<float> > tex1 = tp.GetFloatTexture("tex1", 1.f);
	boost::shared_ptr<Texture<float> > tex2 = tp.GetFloatTexture("tex2", 0.f);
	if (dim == 2) {
		// Initialize 2D texture mapping _map_ from _tp_
		TextureMapping2D *map = NULL;
		string type = tp.FindString("mapping");
		if (type == "" || type == "uv") {
			float su = tp.FindFloat("uscale", 1.);
			float sv = tp.FindFloat("vscale", 1.);
			float du = tp.FindFloat("udelta", 0.);
			float dv = tp.FindFloat("vdelta", 0.);
			map = new UVMapping2D(su, sv, du, dv);
		}
		else if (type == "spherical") map = new SphericalMapping2D(tex2world.GetInverse());
		else if (type == "cylindrical") map = new CylindricalMapping2D(tex2world.GetInverse());
		else if (type == "planar")
			map = new PlanarMapping2D(tp.FindVector("v1", Vector(1,0,0)),
				tp.FindVector("v2", Vector(0,1,0)),
				tp.FindFloat("udelta", 0.f), tp.FindFloat("vdelta", 0.f));
		else {
			//Error("2D texture mapping \"%s\" unknown", type.c_str());
			std::stringstream ss;
			ss<<"2D texture mapping '"<<type<<"' unknown";
			luxError(LUX_BADTOKEN,LUX_ERROR,ss.str().c_str());
			map = new UVMapping2D;
		}
		string aamode = tp.FindString("aamode");
		if (aamode == "") aamode = "closedform";
		return new Checkerboard2D<float>(map, tex1, tex2, aamode);
	}
	else {
		// Initialize 3D texture mapping _map_ from _tp_
		TextureMapping3D *map = new IdentityMapping3D(tex2world);
		// Apply texture specified transformation option for 3D mapping
		IdentityMapping3D *imap = (IdentityMapping3D*) map;
		imap->Apply3DTextureMappingOptions(tp);
		return new Checkerboard3D<float>(map, tex1, tex2);
	}
}

Texture<SWCSpectrum> * Checkerboard::CreateSWCSpectrumTexture(const Transform &tex2world,
		const TextureParams &tp) {
	int dim = tp.FindInt("dimension", 2);
	if (dim != 2 && dim != 3) {
		//Error("%d dimensional checkerboard texture not supported", dim);
		std::stringstream ss;
		ss<<dim<<" dimensional checkerboard texture not supported";
		luxError(LUX_UNIMPLEMENT,LUX_ERROR,ss.str().c_str());
		return NULL;
	}
	boost::shared_ptr<Texture<SWCSpectrum> > tex1 = tp.GetSWCSpectrumTexture("tex1", 1.f);
	boost::shared_ptr<Texture<SWCSpectrum> > tex2 = tp.GetSWCSpectrumTexture("tex2", 0.f);
	if (dim == 2) {
		// Initialize 2D texture mapping _map_ from _tp_
		TextureMapping2D *map = NULL;
		string type = tp.FindString("mapping");
		if (type == "" || type == "uv") {
			float su = tp.FindFloat("uscale", 1.);
			float sv = tp.FindFloat("vscale", 1.);
			float du = tp.FindFloat("udelta", 0.);
			float dv = tp.FindFloat("vdelta", 0.);
			map = new UVMapping2D(su, sv, du, dv);
		}
		else if (type == "spherical") map = new SphericalMapping2D(tex2world.GetInverse());
		else if (type == "cylindrical") map = new CylindricalMapping2D(tex2world.GetInverse());
		else if (type == "planar")
			map = new PlanarMapping2D(tp.FindVector("v1", Vector(1,0,0)),
				tp.FindVector("v2", Vector(0,1,0)),
				tp.FindFloat("udelta", 0.f), tp.FindFloat("vdelta", 0.f));
		else {
			//Error("2D texture mapping \"%s\" unknown", type.c_str());
			std::stringstream ss;
			ss<<"2D texture mapping '"<<type<<"' unknown";
			luxError(LUX_BADTOKEN,LUX_ERROR,ss.str().c_str());
			map = new UVMapping2D;
		}
		string aamode = tp.FindString("aamode");
		if (aamode == "") aamode = "closedform";
		return new Checkerboard2D<SWCSpectrum>(map, tex1, tex2, aamode);
	}
	else {
		// Initialize 3D texture mapping _map_ from _tp_
		TextureMapping3D *map = new IdentityMapping3D(tex2world);
		// Apply texture specified transformation option for 3D mapping
		IdentityMapping3D *imap = (IdentityMapping3D*) map;
		imap->Apply3DTextureMappingOptions(tp);
		return new Checkerboard3D<SWCSpectrum>(map, tex1, tex2);
	}
}

static DynamicLoader::RegisterFloatTexture<Checkerboard> r1("checkerboard");
static DynamicLoader::RegisterSWCSpectrumTexture<Checkerboard> r2("checkerboard");
