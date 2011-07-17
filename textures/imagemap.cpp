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
#include "geometry/matrix3x3.h"

using namespace lux;

void NormalMapTexture::GetDuv(const SpectrumWavelengths &sw,
	const DifferentialGeometry &dg, float delta, float *du, float *dv) const {
	
	float s, t, dsdu, dtdu, dsdv, dtdv;
	mapping->MapDuv(dg, &s, &t, &dsdu, &dtdu, &dsdv, &dtdv);
	float ds, dt;

	// normal from normal map
	Vector n(mipmap->LookupRGBAColor(s, t).c);

	// blender uses full range for Z (blue)
	// TODO - implement different methods for decoding normal
	n.x = 2.f * n.x - 1.f;
	n.y = 2.f * n.y - 1.f;

	// transform dpdu, dpdv to dpds, dpdt

	// invert Jacobian
	const float invJdet = 1.f / (dsdu * dtdv - dsdv * dtdu);
		
	if (fabsf(invJdet) < 1e-5f) {
		*du = *dv = 0.f;
		return;
	}

	const float duds = invJdet * dtdv;
	const float dudt = invJdet * -dsdv;
	const float dvds = invJdet * -dtdu;
	const float dvdt = invJdet * dsdu;

	const Vector dpds = dg.dpdu * duds + dg.dpdv * dvds;
	const Vector dpdt = dg.dpdu * dudt + dg.dpdv * dvdt;
	const Vector k = Normalize(Cross(dpds, dpdt));

	// transform n from tangent to object space
	const Normal t1 = Normalize(Normal(dpds));
	const Normal t2 = Normalize(Normal(dpdt));

	// tangent -> object
	const float mat[3][3] = {
		{ t1.x, t2.x, k.x },
		{ t1.y, t2.y, k.y },
		{ t1.z, t2.z, k.z }};

	n = Normalize(Transform3x3(mat, n));

	// Solve 3x3 system to get dhds/lambda, dhdt/lambda and 1/lambda
	const float A[3][3] = {
		{
			( dpdt.z * k.y - dpdt.y * k.z ),
			( dpds.y * k.z - dpds.z * k.y ),
			( dpds.y * dpdt.z - dpds.z * dpdt.y )
		},
		{
			( dpdt.x * k.z - dpdt.z * k.x ),
			( dpds.z * k.x - dpds.x * k.z ),
			( dpds.z * dpdt.x - dpds.x * dpdt.z )
		},
		{
			( dpdt.y * k.x - dpdt.x * k.y ),
			( dpds.x * k.y - dpds.y * k.x ),
			( dpds.x * dpdt.y - dpds.y * dpdt.x )
		}};

	const float b[3] = { n.x, n.y, n.z };

	float dh[3];

	SolveLinearSystem3x3(A, b, dh);
		
	// recover dhds and dhdt by dividing by 1/lambda
	ds = dh[0] / dh[2];
	dt = dh[1] / dh[2];

	//const Normal nn = Normal(Normalize(Cross(dpds + ds * k, dpdt + dt * k)));

	*du = ds * dsdu + dt * dtdu;
	*dv = ds * dsdv + dt * dtdv;
}


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

	FileData::decode(tp, "filename");
	string filename = tp.FindOneString("filename", "");
	int discardmm = tp.FindOneInt("discardmipmaps", 0);

	ImageSpectrumTexture *tex = new ImageSpectrumTexture(map, filterType,
		filename, discardmm, maxAniso, wrapMode, gain, gamma);

	return tex;
}

Texture<float> *NormalMapTexture::CreateFloatTexture(const Transform &tex2world,
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

	FileData::decode(tp, "filename");
	string filename = tp.FindOneString("filename", "");
	int discardmm = tp.FindOneInt("discardmipmaps", 0);

	NormalMapTexture *tex = new NormalMapTexture(map, filterType,
		filename, discardmm, maxAniso, wrapMode, gain, gamma);

	return tex;
}

map<ImageTexture::TexInfo, boost::shared_ptr<MIPMap> > ImageTexture::textures;

static DynamicLoader::RegisterFloatTexture<ImageFloatTexture> r1("imagemap");
static DynamicLoader::RegisterSWCSpectrumTexture<ImageSpectrumTexture> r2("imagemap");
static DynamicLoader::RegisterFloatTexture<NormalMapTexture> r3("normalmap");
