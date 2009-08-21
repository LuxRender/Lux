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
#include "lux.h"
#include "texture.h"
#include "mipmap.h"
#include "imagereader.h"
#include "paramset.h"
#include "error.h"
#include <map>
using std::map;

// TODO - radiance - add methods for Power and Illuminant propagation

namespace lux
{

// ImageTexture Declarations
class ImageFloatTexture : public Texture<float> {
public:
	// ImageFloatTexture Public Methods
	ImageFloatTexture(
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
	};

	virtual ~ImageFloatTexture() { delete mapping; };

	virtual float Evaluate(const TsPack *tspack, const DifferentialGeometry &dg) const {
		float s, t, dsdx, dtdx, dsdy, dtdy;
		mapping->Map(dg, &s, &t, &dsdx, &dtdx, &dsdy, &dtdy);
		return mipmap->Lookup(s, t, dsdx, dtdx, dsdy, dtdy);
	};
	
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
	//static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);

private:
	// ImageFloatTexture Private Methods
	static MIPMap<float> *GetTexture(
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
		
	// ImageFloatTexture Private Data
	ImageTextureFilterType filterType;
	MIPMap<float> *mipmap;
	TextureMapping2D *mapping;
};

class ImageSpectrumTexture : public Texture<SWCSpectrum> {
public:
	// ImageSpectrumTexture Public Methods
	ImageSpectrumTexture(
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
	};

	virtual ~ImageSpectrumTexture() { delete mapping; };

	virtual SWCSpectrum Evaluate(const TsPack *tspack, const DifferentialGeometry &dg) const {
		float s, t, dsdx, dtdx, dsdy, dtdy;
		mapping->Map(dg, &s, &t, &dsdx, &dtdx, &dsdy, &dtdy);
		return SWCSpectrum(tspack, mipmap->Lookup(s, t, dsdx, dtdx, dsdy, dtdy));
	};
	
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
	
	//static Texture<float> * CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);

private:
	// ImageSpectrumTexture Private Methods
	static MIPMap<RGBColor> *GetTexture(
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
		
	// ImageSpectrumTexture Private Data
	ImageTextureFilterType filterType;
	MIPMap<RGBColor> *mipmap;
	TextureMapping2D *mapping;
};

// ImageMapTexture Method Definitions

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

inline MIPMap<float> *ImageFloatTexture::
	GetTexture(
		ImageTextureFilterType filterType,
		const string &filename,
		float maxAniso,
		ImageWrap wrap,
		float gain,
		float gamma) {
	// Look for texture in texture cache
	static map<TexInfo, MIPMap<float> *> textures;
	TexInfo texInfo(filterType, filename, maxAniso, wrap, gain, gamma);
	if (textures.find(texInfo) != textures.end())
		return textures[texInfo];
	int width, height;
	auto_ptr<ImageData> imgdata(ReadImage(filename));
	MIPMap<float> *ret = NULL;
	if (imgdata.get() != NULL) {
		width=imgdata->getWidth();
		height=imgdata->getHeight();
		ret = imgdata->createMIPMap<float>(filterType, maxAniso, wrap, gain, gamma);
	} else {
		// Create one-valued _MIPMap_
		float *oneVal = new float[1];
		oneVal[0] = 1.;

		ret = new MIPMapFastImpl<float,float>(filterType, 1, 1, oneVal);

		delete[] oneVal;
	}
	textures[texInfo] = ret;
	
	return ret;
}

inline MIPMap<RGBColor> *ImageSpectrumTexture::
	GetTexture(
		ImageTextureFilterType filterType,
		const string &filename,
		float maxAniso,
		ImageWrap wrap,
		float gain,
		float gamma) {
	// Look for texture in texture cache
	static map<TexInfo, MIPMap<RGBColor> *> textures;
	TexInfo texInfo(filterType, filename, maxAniso, wrap, gain, gamma);
	if (textures.find(texInfo) != textures.end())
		return textures[texInfo];
	int width, height;
	auto_ptr<ImageData> imgdata(ReadImage(filename));
	MIPMap<RGBColor> *ret = NULL;
	if (imgdata.get() != NULL) {
		width=imgdata->getWidth();
		height=imgdata->getHeight();
		ret = imgdata->createMIPMap<RGBColor>(filterType, maxAniso, wrap, gain, gamma);
	} else {
		// Create one-valued _MIPMap_
		RGBColor *oneVal = new RGBColor[1];
		oneVal[0] = 1.;

		ret = new MIPMapFastImpl<RGBColor,RGBColor>(filterType, 1, 1, oneVal);

		delete[] oneVal;
	}
	textures[texInfo] = ret;
	
	return ret;
}

}//namespace lux
