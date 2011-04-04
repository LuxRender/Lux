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

// imagemap.cpp*
#include "imagemap.h"
#include "dynload.h"

using namespace lux;

Texture<float> *ImageFloatTexture::CreateFloatTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	// Initialize 2D texture mapping _map_ from _tp_
	TextureMapping2D *map = NULL;
	
	string sFilterType = tp.FindOneString("filtertype", "bilinear");
	ImageTextureFilterType filterType = BILINEAR;
	if (sFilterType == "bilinear")
		filterType = BILINEAR;
	else if (sFilterType == "mipmap_trilinear")
		filterType = MIPMAP_TRILINEAR;
	else if (sFilterType == "mipmap_ewa")
		filterType = MIPMAP_EWA;
	else if (sFilterType == "nearest")
		filterType = NEAREST;

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
		LOG(LUX_ERROR, LUX_BADTOKEN) << "2D texture mapping  '" <<
			type << "' unknown";
		map = new UVMapping2D;
	}

	// Initialize _ImageTexture_ parameters
	float maxAniso = tp.FindOneFloat("maxanisotropy", 8.f);
	string wrap = tp.FindOneString("wrap", "repeat");
	ImageWrap wrapMode = TEXTURE_REPEAT;
	if (wrap == "repeat")
		wrapMode = TEXTURE_REPEAT;
	else if (wrap == "black")
		wrapMode = TEXTURE_BLACK;
	else if (wrap == "white")
		wrapMode = TEXTURE_WHITE;
	else if (wrap == "clamp")
		wrapMode = TEXTURE_CLAMP;

	float gain = tp.FindOneFloat("gain", 1.0f);
	float gamma = tp.FindOneFloat("gamma", 1.0f);

	string filename = tp.FindOneString("filename", "");
	int discardmm = tp.FindOneInt("discardmipmaps", 0);

	string channel = tp.FindOneString("channel", "mean");
	Channel ch;
	if (channel == "red")
		ch = CHANNEL_RED;
	else if (channel == "green")
		ch = CHANNEL_GREEN;
	else if (channel == "blue")
		ch = CHANNEL_BLUE;
	else if (channel == "alpha")
		ch = CHANNEL_ALPHA;
	else if (channel == "mean")
		ch = CHANNEL_MEAN;
	else if (channel == "colored_mean")
		ch = CHANNEL_WMEAN;
	else {
		LOG(LUX_WARNING, LUX_BADTOKEN) << "Unknown image channel '" <<
			channel << "' using 'mean' instead";
		ch = CHANNEL_MEAN;
	}

	ImageFloatTexture *tex = new ImageFloatTexture(map, filterType,
		filename, discardmm, maxAniso, wrapMode, ch, gain, gamma);

	return tex;
}

RGBIllumSPD ImageSpectrumTexture::whiteRGBIllum;

Texture<SWCSpectrum> *ImageSpectrumTexture::CreateSWCSpectrumTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	// Initialize 2D texture mapping _map_ from _tp_
	TextureMapping2D *map = NULL;
	
	string sFilterType = tp.FindOneString("filtertype", "bilinear");
	ImageTextureFilterType filterType = BILINEAR;
	if (sFilterType == "bilinear")
		filterType = BILINEAR;
	else if (sFilterType == "mipmap_trilinear")
		filterType = MIPMAP_TRILINEAR;
	else if (sFilterType == "mipmap_ewa")
		filterType = MIPMAP_EWA;
	else if (sFilterType == "nearest")
		filterType = NEAREST;

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
		LOG(LUX_ERROR, LUX_BADTOKEN) << "2D texture mapping  '" <<
			type << "' unknown";
		map = new UVMapping2D;
	}

	// Initialize _ImageTexture_ parameters
	float maxAniso = tp.FindOneFloat("maxanisotropy", 8.f);
	string wrap = tp.FindOneString("wrap", "repeat");
	ImageWrap wrapMode = TEXTURE_REPEAT;
	if (wrap == "repeat")
		wrapMode = TEXTURE_REPEAT;
	else if (wrap == "black")
		wrapMode = TEXTURE_BLACK;
	else if (wrap == "white")
		wrapMode = TEXTURE_WHITE;
	else if (wrap == "clamp")
		wrapMode = TEXTURE_CLAMP;

	float gain = tp.FindOneFloat("gain", 1.0f);
	float gamma = tp.FindOneFloat("gamma", 1.0f);

	string filename = tp.FindOneString("filename", "");
	int discardmm = tp.FindOneInt("discardmipmaps", 0);

	ImageSpectrumTexture *tex = new ImageSpectrumTexture(map, filterType,
		filename, discardmm, maxAniso, wrapMode, gain, gamma);

	return tex;
}

map<ImageTexture::TexInfo, boost::shared_ptr<MIPMap> > ImageTexture::textures;

static DynamicLoader::RegisterFloatTexture<ImageFloatTexture> r1("imagemap");
static DynamicLoader::RegisterSWCSpectrumTexture<ImageSpectrumTexture> r2("imagemap");
