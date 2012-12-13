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
#include "spectrum.h"
#include "texture.h"
#include "mipmap.h"
#include "imagereader.h"
#include "paramset.h"
#include "error.h"
#include "rgbillum.h"
#include <map>
using std::map;

// TODO - radiance - add methods for Power and Illuminant propagation

namespace lux
{

class TexInfo {
public:
	TexInfo(ImageTextureFilterType type, const string &f, int dm,
		float ma, ImageWrap wm, float ga, float gam) :
		filterType(type), filename(f), discardmm(dm),
		maxAniso(ma), wrapMode(wm), gain(ga), gamma(gam) { }

	ImageTextureFilterType filterType;
	string filename;
	int discardmm;
	float maxAniso;
	ImageWrap wrapMode;
	float gain;
	float gamma;

	bool operator<(const TexInfo &t2) const {
		if (filterType != t2.filterType)
			return filterType < t2.filterType;
		if (filename != t2.filename)
			return filename < t2.filename;
		if (discardmm != t2.discardmm)
			return discardmm < t2.discardmm;
		if (maxAniso != t2.maxAniso)
			return maxAniso < t2.maxAniso;
		if (wrapMode != t2.wrapMode)
			return wrapMode < t2.wrapMode;
		if (gain != t2.gain)
			return gain < t2.gain;
		return gamma < t2.gamma;
	}
};

class ImageTexture {
public:
	// ImageTexture Public Methods
	ImageTexture(const TexInfo &texInfo, TextureMapping2D *m) : info(texInfo) {
		mipmap = GetTexture(info);
	}
	virtual ~ImageTexture() {
		// If the map isn't used anymore, remove it from the cache
		// The last user still has 2 references:
		// 1 from the texture and 1 from the dictionary
		for (map<TexInfo, boost::shared_ptr<MIPMap> >::iterator t = textures.begin(); t != textures.end(); ++t) {
			if ((*t).second.get() == mipmap.get() &&
				(*t).second.use_count() == 2) {
				textures.erase(t);
				break;
			}
		}
		delete mapping;
	}

	const MIPMap *GetMIPMap() const { return mipmap.get(); }
	const TexInfo &GetInfo() const { return info; }

private:
	static map<TexInfo, boost::shared_ptr<MIPMap> > textures;

	// ImageTexture Private Methods
	static boost::shared_ptr<MIPMap> GetTexture(const TexInfo &texInfo);

protected:
	// ImageTexture Protected Data
	boost::shared_ptr<MIPMap> mipmap;
	TextureMapping2D *mapping;
	TexInfo info;
};

// ImageTexture Declarations
class ImageFloatTexture : public Texture<float>, public ImageTexture {
public:
	// ImageFloatTexture Public Methods
	ImageFloatTexture(const TexInfo &texInfo, TextureMapping2D *m, Channel ch) :
		Texture("ImageFloatTexture-" + boost::lexical_cast<string>(this)),
		ImageTexture(texInfo, m) { channel = ch; }

	virtual ~ImageFloatTexture() { }

	virtual float Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		float s, t;
		mapping->Map(dg, &s, &t);
		return mipmap->LookupFloat(channel, s, t);
	}
	virtual float Y() const {
		return mipmap->LookupFloat(channel, .5f, .5f, .5f);
	}
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const {
		float s, t, dsdu, dtdu, dsdv, dtdv;
		mapping->MapDuv(dg, &s, &t, &dsdu, &dtdu, &dsdv, &dtdv);
		float ds, dt;
		mipmap->GetDifferentials(channel, s, t, &ds, &dt);
		*du = ds * dsdu + dt * dtdu;
		*dv = ds * dsdv + dt * dtdv;
	}

	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		mipmap->GetMinMaxFloat(channel, minValue, maxValue);
	}

	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);

private:
	// ImageFloatTexture Private Data
	Channel channel;
};

class ImageSpectrumTexture : public Texture<SWCSpectrum>, public ImageTexture {
public:
	// ImageSpectrumTexture Public Methods
	ImageSpectrumTexture(const TexInfo &texInfo, TextureMapping2D *m) :
		Texture("ImageSpectrumTexture-" + boost::lexical_cast<string>(this)),
		ImageTexture(texInfo, m), isIlluminant(false) { }

	virtual ~ImageSpectrumTexture() { }

	virtual SWCSpectrum Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		float s, t;
		mapping->Map(dg, &s, &t);
		if (isIlluminant)
			return SWCSpectrum(sw, whiteRGBIllum) * mipmap->LookupSpectrum(sw, s, t);
		else
			return mipmap->LookupSpectrum(sw, s, t);
	}
	virtual float Y() const {
		return (isIlluminant ? whiteRGBIllum.Y() : 1.f) * 
			mipmap->LookupFloat(CHANNEL_WMEAN, .5f, .5f, .5f);
	}
	virtual float Filter() const {
		return (isIlluminant ? whiteRGBIllum.Filter() : 1.f) *
			mipmap->LookupFloat(CHANNEL_MEAN, .5f, .5f, .5f);
	}
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const {
		float s, t, dsdu, dtdu, dsdv, dtdv;
		mapping->MapDuv(dg, &s, &t, &dsdu, &dtdu, &dsdv, &dtdv);
		float ds, dt;
		mipmap->GetDifferentials(sw, s, t, &ds, &dt);
		*du = ds * dsdu + dt * dtdu;
		*dv = ds * dsdv + dt * dtdv;
	}

	virtual void SetIlluminant() {
		isIlluminant = true;
	}

	static RGBIllumSPD whiteRGBIllum;
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);
private:
	bool isIlluminant;
};


class NormalMapTexture : public Texture<float>, public ImageTexture {
public:
	// NormalMapTexture Public Methods
	NormalMapTexture(const TexInfo &texInfo, TextureMapping2D *m) :
		Texture("NormalMapTexture-" + boost::lexical_cast<string>(this)),
		ImageTexture(texInfo, m) { }

	virtual ~NormalMapTexture() { }

	virtual float Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return 0.f;
	}
	virtual float Y() const {
		return 0.f;
	}
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const;

	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		*minValue = 0.f;
		*maxValue = 0.f;
	}

	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);

private:
	// NormalMapTexture Private Data
};

// ImageTexture Method Definitions
inline boost::shared_ptr<MIPMap> ImageTexture::GetTexture(const TexInfo &texInfo) {
	// Look for texture in texture cache
	if (textures.find(texInfo) != textures.end()) {
		LOG(LUX_INFO, LUX_NOERROR) << "Reusing data for imagemap '" <<
			texInfo.filename << "'";
		return textures[texInfo];
	}
	std::auto_ptr<ImageData> imgdata(ReadImage(texInfo.filename));
	boost::shared_ptr<MIPMap> ret;
	if (imgdata.get() != NULL) {
		ret = boost::shared_ptr<MIPMap>(imgdata->createMIPMap(
				texInfo.filterType, texInfo.maxAniso, texInfo.wrapMode, texInfo.gain, texInfo.gamma));
	} else {
		// Create one-valued _MIPMap_
		TextureColor<float, 1> oneVal(1.f);

		ret = boost::shared_ptr<MIPMap>(new MIPMapFastImpl<TextureColor<float, 1> >(
				texInfo.filterType, 1, 1, &oneVal));
	}
	if (ret) {
		if (texInfo.discardmm > 0 && (texInfo.filterType == MIPMAP_TRILINEAR ||
			texInfo.filterType == MIPMAP_EWA)) {
			ret->DiscardMipmaps(texInfo.discardmm);

			LOG(LUX_INFO, LUX_NOERROR) << "Discarded " <<
				texInfo.discardmm << " mipmap levels";
		}

		LOG(LUX_INFO, LUX_NOERROR) << "Memory used for imagemap '" <<
			texInfo.filename << "': " << (ret->GetMemoryUsed() / 1024) <<
			"KBytes";

		textures[texInfo] = ret;
		return textures[texInfo];
	}
	LOG(LUX_ERROR, LUX_SYSTEM) << "Creation of imagemap '" << texInfo.filename <<
		"' failed";

	return boost::shared_ptr<MIPMap>();
}

}//namespace lux
