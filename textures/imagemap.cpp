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
#include "filedata.h"
#include "geometry/raydifferential.h"

using namespace lux;

void NormalMapTexture::GetDuv(const SpectrumWavelengths &sw,
	const DifferentialGeometry &dg, float delta, float *du, float *dv) const {
	
	float s, t;
	mapping->Map(dg, &s, &t);

	// normal from normal map
	Vector n(mipmap->LookupRGBAColor(s, t).c);

	// TODO - implement different methods for decoding normal
	n = 2.f * n - Vector(1.f, 1.f, 1.f);

	// recover dhdu,dhdv in uv space directly
	const Vector dpdu = dg.dpdu;
	const Vector dpdv = dg.dpdv;
	const Vector k = Vector(dg.nn);

	// transform n from tangent to object space
	const Vector t1 = dg.tangent;
	const Vector t2 = dg.bitangent;
	// magnitude of dg.tsign is the magnitude of the interpolated normal
	const Vector kk = k * fabsf(dg.btsign);
	const float btsign = dg.btsign > 0.f ? 1.f : -1.f;

	// tangent -> object
	n = Normalize(n.x * t1 + n.y * btsign * t2 + n.z * kk);	

	// Since n is stored normalized in the normal map
	// we need to recover the original length (lambda).
	// We do this by solving 
	//   lambda*n = dp/du x dp/dv
	// where 
	//   p(u,v) = base(u,v) + h(u,v) * k
	// and
	//   k = dbase/du x dbase/dv
	//
	// We recover lambda by dotting the above with k
	//   k . lambda*n = k . (dp/du x dp/dv)
	//   lambda = (k . k) / (k . n)
	// 
	// We then recover dh/du by dotting the first eq by dp/du
	//   dp/du . lambda*n = dp/du . (dp/du x dp/dv)
	//   dp/du . lambda*n = dh/du * [dbase/du . (k x dbase/dv)]
	//
	// The term "dbase/du . (k x dbase/dv)" reduces to "-(k . k)", so we get
	//   dp/du . lambda*n = dh/du * -(k . k)
	//   dp/du . [(k . k) / (k . n)*n] = dh/du * -(k . k)
	//   dp/du . [-n / (k . n)] = dh/du
	// and similar for dh/dv
	// 
	// Since the recovered dh/du will be in units of ||k||, we must divide
	// by ||k|| to get normalized results. Using dg.nn as k in the last eq 
	// yields the same result.
	const Vector nn = (-1.f / Dot(k, n)) * n;

	*du = Dot(dpdu, nn);
	*dv = Dot(dpdv, nn);
}


Texture<float> *ImageFloatTexture::CreateFloatTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	// Initialize 2D texture mapping _map_ from _tp_
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

	FileData::decode(tp, "filename");
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

	ImageFloatTexture *tex = new ImageFloatTexture(TextureMapping2D::Create(tex2world, tp), filterType,
		filename, discardmm, maxAniso, wrapMode, ch, gain, gamma);

	return tex;
}

RGBIllumSPD ImageSpectrumTexture::whiteRGBIllum;

Texture<SWCSpectrum> *ImageSpectrumTexture::CreateSWCSpectrumTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	// Initialize 2D texture mapping _map_ from _tp_
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

	FileData::decode(tp, "filename");
	string filename = tp.FindOneString("filename", "");
	int discardmm = tp.FindOneInt("discardmipmaps", 0);

	ImageSpectrumTexture *tex = new ImageSpectrumTexture(TextureMapping2D::Create(tex2world, tp), filterType,
		filename, discardmm, maxAniso, wrapMode, gain, gamma);

	return tex;
}

Texture<float> *NormalMapTexture::CreateFloatTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	// Initialize 2D texture mapping _map_ from _tp_
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

	FileData::decode(tp, "filename");
	string filename = tp.FindOneString("filename", "");
	int discardmm = tp.FindOneInt("discardmipmaps", 0);

	NormalMapTexture *tex = new NormalMapTexture(TextureMapping2D::Create(tex2world, tp), filterType,
		filename, discardmm, maxAniso, wrapMode, gain, gamma);

	return tex;
}

map<ImageTexture::TexInfo, boost::shared_ptr<MIPMap> > ImageTexture::textures;

static DynamicLoader::RegisterFloatTexture<ImageFloatTexture> r1("imagemap");
static DynamicLoader::RegisterSWCSpectrumTexture<ImageSpectrumTexture> r2("imagemap");
static DynamicLoader::RegisterFloatTexture<NormalMapTexture> r3("normalmap");
