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
	ImageTexture(TextureMapping2D *m,
			const string &filename,
			bool doTri,
			float maxAniso,
			ImageWrap wm,
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
	static Texture<Spectrum> * CreateSpectrumTexture(const Transform &tex2world, const TextureParams &tp);

private:
	// ImageTexture Private Methods
	static MIPMap<T> *GetTexture(const string &filename,
	    bool doTrilinear, float maxAniso, ImageWrap wm, float gain, float gamma);
	static void convert(const Spectrum &from, Spectrum *to) {
		*to = from;
	}
	static void convert(const Spectrum &from, float *to) {
		*to = from.y();
	}

	// ImageTexture Private Data
	MIPMap<T> *mipmap;
	TextureMapping2D *mapping;
};

template <class T> inline Texture<float> *ImageTexture<T>::CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
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
		ss<<"2D texture mapping  '"<<type<<"' unknown";
		luxError(LUX_BADTOKEN,LUX_ERROR,ss.str().c_str());
		map = new UVMapping2D;
	}

	// Initialize _ImageTexture_ parameters
	float maxAniso = tp.FindFloat("maxanisotropy", 8.f);
	bool trilerp = tp.FindBool("trilinear", false);
	string wrap = tp.FindString("wrap");
	ImageWrap wrapMode = TEXTURE_REPEAT;
	if (wrap == "" || wrap == "repeat") wrapMode = TEXTURE_REPEAT;
	else if (wrap == "black") wrapMode = TEXTURE_BLACK;
	else if (wrap == "clamp") wrapMode = TEXTURE_CLAMP;

	float gain = tp.FindFloat("gain", 1.0f);
	float gamma = tp.FindFloat("gamma", 1.0f);

	string filename = tp.FindString("filename");

	int discardmm = tp.FindInt("discardmipmaps", 0);

	ImageTexture<float> *tex = new ImageTexture<float>(map, filename,
		trilerp, maxAniso, wrapMode, gain, gamma);

	if (discardmm > 0) {
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

template <class T> inline Texture<Spectrum> *ImageTexture<T>::CreateSpectrumTexture(const Transform &tex2world,
		const TextureParams &tp) {
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
		ss<<"2D texture mapping  '"<<type<<"' unknown";
		luxError(LUX_BADTOKEN,LUX_ERROR,ss.str().c_str());
		map = new UVMapping2D;
	}

	// Initialize _ImageTexture_ parameters
	float maxAniso = tp.FindFloat("maxanisotropy", 8.f);
	bool trilerp = tp.FindBool("trilinear", false);
	string wrap = tp.FindString("wrap");
	ImageWrap wrapMode = TEXTURE_REPEAT;
	if (wrap == "" || wrap == "repeat") wrapMode = TEXTURE_REPEAT;
	else if (wrap == "black") wrapMode = TEXTURE_BLACK;
	else if (wrap == "clamp") wrapMode = TEXTURE_CLAMP;

	float gain = tp.FindFloat("gain", 1.0f);
	float gamma = tp.FindFloat("gamma", 1.0f);

	string filename = tp.FindString("filename");

	int discardmm = tp.FindInt("discardmipmaps", 0);

	ImageTexture<Spectrum> *tex = new ImageTexture<Spectrum>(map, filename,
		trilerp, maxAniso, wrapMode, gain, gamma);

	if (discardmm > 0) {
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
ImageTexture<T>::ImageTexture(TextureMapping2D *m,
		const string &filename,
		bool doTrilinear,
		float maxAniso,
		ImageWrap wrapMode,
		float gain,
		float gamma) {
	mapping = m;
	mipmap = GetTexture(filename, doTrilinear,
		maxAniso, wrapMode, gain, gamma);
}

template <class T> inline ImageTexture<T>::~ImageTexture() {
	delete mapping;
}

struct TexInfo {
	TexInfo(const string &f, bool dt, float ma, ImageWrap wm)
		: filename(f), doTrilinear(dt), maxAniso(ma), wrapMode(wm) { }
	string filename;
	bool doTrilinear;
	float maxAniso;
	ImageWrap wrapMode;
	bool operator<(const TexInfo &t2) const {
		if (filename != t2.filename) return filename < t2.filename;
		if (doTrilinear != t2.doTrilinear) return doTrilinear < t2.doTrilinear;
		if (maxAniso != t2.maxAniso) return maxAniso < t2.maxAniso;
		return wrapMode < t2.wrapMode;
	}
};

template <class T> inline MIPMap<T> *ImageTexture<T>::
	GetTexture(const string &filename,
		bool doTrilinear,
		float maxAniso,
		ImageWrap wrap,
		float gain,
		float gamma) {
	// Look for texture in texture cache
	static map<TexInfo, MIPMap<T> *> textures;
	TexInfo texInfo(filename, doTrilinear, maxAniso, wrap);
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
		ret = imgdata->createMIPMap<T>(doTrilinear, maxAniso, wrap, gain, gamma);
	} else {
		// Create one-valued _MIPMap_
		T *oneVal = new T[1];
		oneVal[0] = 1.;

		ret = new MIPMapFastImpl<T,T>(1, 1, oneVal);

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
