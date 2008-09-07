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

// imagemap.cpp*
#include "lux.h"
#include "texture.h"
#include "mipmap.h"
#include "imagereader.h"
#include "paramset.h"
#include "error.h"
#include <map>
using std::map;

namespace lux
{

// ImageMapTexture Declarations
template <class T>
class ImageTexture : public Texture<T> {
public:
	// ImageTexture Public Methods
	ImageTexture(
			TextureMapping2D *m,
			ImageTextureFilterType type,
			const string &filename,
			float maxAniso,
			ImageWrap wrapMode,
			float gain,
			float gamma);
	T Evaluate(const DifferentialGeometry &) const;
	~ImageTexture();
	
	u_int getMemoryUsed() const {
		if (mipmap)
			return mipmap->getMemoryUsed();
		else
			return 0;
	}

	void discardMipmaps(int n) {
		if (mipmap)
			mipmap->discardMipmaps(n);
	}
	
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<RGBColor> * CreateRGBColorTexture(const Transform &tex2world, const TextureParams &tp);

private:
	// ImageTexture Private Methods
	static MIPMap<T> *GetTexture(
		ImageTextureFilterType filterType,
		const string &filename,
		float maxAniso,
		ImageWrap wrap,
		float gain,
		float gamma);
	static void convert(const RGBColor &from, RGBColor *to) {
		*to = from;
	}
	static void convert(const RGBColor &from, float *to) {
		*to = from.y();
	}
		
	// ImageTexture Private Data

	ImageTextureFilterType filterType;
	MIPMap<T> *mipmap;
	TextureMapping2D *mapping;
};

template <class T> inline Texture<float> *ImageTexture<T>::CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
	// Initialize 2D texture mapping _map_ from _tp_
	TextureMapping2D *map = NULL;
	
	string sFilterType = tp.FindString("filtertype");
	ImageTextureFilterType filterType = BILINEAR;
	if ((sFilterType == "") || (sFilterType == "bilinear")) {
		filterType = BILINEAR;
	} else if (sFilterType == "mipmap_trilinear") {
		filterType = MIPMAP_TRILINEAR;
	} else if (sFilterType == "mipmap_ewa") {
		filterType = MIPMAP_EWA;
	} else if (sFilterType == "nearest") {
		filterType = NEAREST;
	}

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
		ss<<"2D texture mapping  '"<<type<<"' unknown";
		luxError(LUX_BADTOKEN,LUX_ERROR,ss.str().c_str());
		map = new UVMapping2D;
	}

	// Initialize _ImageTexture_ parameters
	float maxAniso = tp.FindFloat("maxanisotropy", 8.f);
	string wrap = tp.FindString("wrap");
	ImageWrap wrapMode = TEXTURE_REPEAT;
	if (wrap == "" || wrap == "repeat") wrapMode = TEXTURE_REPEAT;
	else if (wrap == "black") wrapMode = TEXTURE_BLACK;
	else if (wrap == "clamp") wrapMode = TEXTURE_CLAMP;

	float gain = tp.FindFloat("gain", 1.0f);
	float gamma = tp.FindFloat("gamma", 1.0f);

	string filename = tp.FindString("filename");
	int discardmm = tp.FindInt("discardmipmaps", 0);

	ImageTexture<float> *tex = new ImageTexture<float>(map, filterType,
			filename, maxAniso, wrapMode, gain, gamma);

	if ((discardmm > 0) &&
			((filterType == MIPMAP_TRILINEAR) || (filterType == MIPMAP_EWA))) {
		tex->discardMipmaps(discardmm);

		std::stringstream ss;
		ss<<"Discarded " << discardmm << " mipmap levels";
		luxError(LUX_NOERROR,LUX_INFO,ss.str().c_str());
	}

	std::stringstream ss;
	ss<<"Memory used for imagemap '" << filename << "': " <<
		(tex->getMemoryUsed() / 1024) << "KBytes";
	luxError(LUX_NOERROR,LUX_INFO,ss.str().c_str());

	return tex;
}

template <class T> inline Texture<RGBColor> *ImageTexture<T>::CreateRGBColorTexture(const Transform &tex2world,
		const TextureParams &tp) {
	// Initialize 2D texture mapping _map_ from _tp_
	TextureMapping2D *map = NULL;
	
	string sFilterType = tp.FindString("filtertype");
	ImageTextureFilterType filterType = BILINEAR;
	if ((sFilterType == "") || (sFilterType == "bilinear")) {
		filterType = BILINEAR;
	} else if (sFilterType == "mipmap_trilinear") {
		filterType = MIPMAP_TRILINEAR;
	} else if (sFilterType == "mipmap_ewa") {
		filterType = MIPMAP_EWA;
	} else if (sFilterType == "nearest") {
		filterType = NEAREST;
	}

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
		ss<<"2D texture mapping  '"<<type<<"' unknown";
		luxError(LUX_BADTOKEN,LUX_ERROR,ss.str().c_str());
		map = new UVMapping2D;
	}

	// Initialize _ImageTexture_ parameters
	float maxAniso = tp.FindFloat("maxanisotropy", 8.f);
	string wrap = tp.FindString("wrap");
	ImageWrap wrapMode = TEXTURE_REPEAT;
	if (wrap == "" || wrap == "repeat") wrapMode = TEXTURE_REPEAT;
	else if (wrap == "black") wrapMode = TEXTURE_BLACK;
	else if (wrap == "clamp") wrapMode = TEXTURE_CLAMP;

	float gain = tp.FindFloat("gain", 1.0f);
	float gamma = tp.FindFloat("gamma", 1.0f);

	string filename = tp.FindString("filename");
	int discardmm = tp.FindInt("discardmipmaps", 0);

	ImageTexture<RGBColor> *tex = new ImageTexture<RGBColor>(map, filterType,
			filename, maxAniso, wrapMode, gain, gamma);

	if ((discardmm > 0) &&
			((filterType == MIPMAP_TRILINEAR) || (filterType == MIPMAP_EWA))) {
		tex->discardMipmaps(discardmm);

		std::stringstream ss;
		ss<<"Discarded " << discardmm << " mipmap levels";
		luxError(LUX_NOERROR,LUX_INFO,ss.str().c_str());
	}

	std::stringstream ss;
	ss<<"Memory used for imagemap '" << filename << "': " <<
		(tex->getMemoryUsed() / 1024) << "KBytes";
	luxError(LUX_NOERROR,LUX_INFO,ss.str().c_str());

	return tex;
}

// ImageMapTexture Method Definitions
template <class T> inline 
ImageTexture<T>::ImageTexture(
		TextureMapping2D *m,
		ImageTextureFilterType type,
		const string &filename,
		float maxAniso,
		ImageWrap wrapMode,
		float gain,
		float gamma) {
	filterType = type;
	mapping = m;
	mipmap = GetTexture(filterType, filename, maxAniso, wrapMode, gain, gamma);
}

template <class T> inline ImageTexture<T>::~ImageTexture() {
	delete mapping;
}

struct TexInfo {
	TexInfo(ImageTextureFilterType type, const string &f, float ma,
			ImageWrap wm, float ga, float gam)
		: filterType(type), filename(f), maxAniso(ma),
			wrapMode(wm), gain(ga), gamma(gam) { }

	ImageTextureFilterType filterType;
	string filename;
	float maxAniso;
	ImageWrap wrapMode;
	float gain;
	float gamma;

	bool operator<(const TexInfo &t2) const {
		if (filterType != t2.filterType) return filterType < t2.filterType;
		if (filename != t2.filename) return filename < t2.filename;
		if (maxAniso != t2.maxAniso) return maxAniso < t2.maxAniso;
		if (wrapMode != t2.wrapMode) return wrapMode < t2.wrapMode;
		if (gain != t2.gain) return gain < t2.gain;

		return gamma < t2.gamma;
	}
};

template <class T> inline MIPMap<T> *ImageTexture<T>::
	GetTexture(
		ImageTextureFilterType filterType,
		const string &filename,
		float maxAniso,
		ImageWrap wrap,
		float gain,
		float gamma) {
	// Look for texture in texture cache
	static map<TexInfo, MIPMap<T> *> textures;
	TexInfo texInfo(filterType, filename, maxAniso, wrap, gain, gamma);
	if (textures.find(texInfo) != textures.end())
		return textures[texInfo];

	// radiance - disabled for threading // static StatsCounter texLoaded("Texture", "Number of image maps loaded"); // NOBOOK
	// radiance - disabled for threading // ++texLoaded; // NOBOOK
	int width, height;
	auto_ptr<ImageData> imgdata(ReadImage(filename));
	MIPMap<T> *ret = NULL;
	if (imgdata.get() != NULL) {
		width=imgdata->getWidth();
		height=imgdata->getHeight();
		ret = imgdata->createMIPMap<T>(filterType, maxAniso, wrap, gain, gamma);
	} else {
		// Create one-valued _MIPMap_
		T *oneVal = new T[1];
		oneVal[0] = 1.;

		ret = new MIPMapFastImpl<T,T>(filterType, 1, 1, oneVal);

		delete[] oneVal;
	}
	textures[texInfo] = ret;
	
	return ret;
}

template <class T> inline 
T ImageTexture<T>::Evaluate(
		const DifferentialGeometry &dg) const {
	float s, t, dsdx, dtdx, dsdy, dtdy;
	mapping->Map(dg, &s, &t, &dsdx, &dtdx, &dsdy, &dtdy);
	return mipmap->Lookup(s, t, dsdx, dtdx, dsdy, dtdy);
}

}//namespace lux
