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
#include <map>
using std::map;

// TODO - radiance - add methods for Power and Illuminant propagation

namespace lux
{

class ImageTexture {
public:
	// ImageTexture Public Methods
	ImageTexture(TextureMapping2D *m, ImageTextureFilterType type,
		const string &filename, float maxAniso, ImageWrap wrapMode,
		float gain, float gamma) {
		filterType = type;
		mapping = m;
		mipmap = GetTexture(filterType, filename, maxAniso, wrapMode,
			gain, gamma);
	}
	virtual ~ImageTexture() {
		delete mapping;
	}

	u_int GetMemoryUsed() const {
		if (mipmap)
			return mipmap->GetMemoryUsed();
		else
			return 0;
	}
	void DiscardMipmaps(int n) {
		if (mipmap)
			mipmap->DiscardMipmaps(n);
	}

private:
	class TexInfo {
	public:
		TexInfo(ImageTextureFilterType type, const string &f, float ma,
			ImageWrap wm, float ga, float gam) :
			filterType(type), filename(f), maxAniso(ma),
			wrapMode(wm), gain(ga), gamma(gam) { }

		ImageTextureFilterType filterType;
		string filename;
		float maxAniso;
		ImageWrap wrapMode;
		float gain;
		float gamma;

		bool operator<(const TexInfo &t2) const {
			if (filterType != t2.filterType)
				return filterType < t2.filterType;
			if (filename != t2.filename)
				return filename < t2.filename;
			if (maxAniso != t2.maxAniso)
				return maxAniso < t2.maxAniso;
			if (wrapMode != t2.wrapMode)
				return wrapMode < t2.wrapMode;
			if (gain != t2.gain)
				return gain < t2.gain;
			return gamma < t2.gamma;
		}
	};

	// ImageTexture Private Methods
	static boost::shared_ptr<MIPMap> GetTexture(ImageTextureFilterType filterType,
		const string &filename, float maxAniso, ImageWrap wrap,
		float gain, float gamma);

protected:
	// ImageTexture Protected Data
	ImageTextureFilterType filterType;
	boost::shared_ptr<MIPMap> mipmap;
	TextureMapping2D *mapping;
};

// ImageTexture Declarations
class ImageFloatTexture : public Texture<float>, public ImageTexture {
public:
	// ImageFloatTexture Public Methods
	ImageFloatTexture(TextureMapping2D *m, ImageTextureFilterType type,
		const string &filename, float maxAniso, ImageWrap wrapMode,
		Channel ch, float gain, float gamma) :
		ImageTexture(m, type, filename, maxAniso, wrapMode,
			gain, gamma) { channel = ch; }

	virtual ~ImageFloatTexture() { }

	virtual float Evaluate(const TsPack *tspack,
		const DifferentialGeometry &dg) const {
		float s, t, dsdx, dtdx, dsdy, dtdy;
		mapping->MapDxy(dg, &s, &t, &dsdx, &dtdx, &dsdy, &dtdy);
		return mipmap->LookupFloat(channel, s, t,
			dsdx, dtdx, dsdy, dtdy);
	}
	virtual float Y() const {
		return mipmap->LookupFloat(channel, .5f, .5f, .5f);
	}
	virtual void GetDuv(const TsPack *tspack,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const {
		float s, t, dsdu, dtdu, dsdv, dtdv;
		mapping->MapDuv(dg, &s, &t, &dsdu, &dtdu, &dsdv, &dtdv);
		float ds, dt;
		mipmap->GetDifferentials(channel, s, t, &ds, &dt);
		*du = ds * dsdu + dt * dtdu;
		*dv = ds * dsdv + dt * dtdv;
	}

	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);

private:
	// ImageFloatTexture Private Data
	Channel channel;
};

class ImageSpectrumTexture : public Texture<SWCSpectrum>, public ImageTexture {
public:
	// ImageSpectrumTexture Public Methods
	ImageSpectrumTexture(TextureMapping2D *m, ImageTextureFilterType type,
		const string &filename, float maxAniso, ImageWrap wrapMode,
		float gain, float gamma) :
		ImageTexture(m, type, filename, maxAniso, wrapMode,
			gain, gamma) { }

	virtual ~ImageSpectrumTexture() { }

	virtual SWCSpectrum Evaluate(const TsPack *tspack,
		const DifferentialGeometry &dg) const {
		float s, t, dsdx, dtdx, dsdy, dtdy;
		mapping->MapDxy(dg, &s, &t, &dsdx, &dtdx, &dsdy, &dtdy);
		return mipmap->LookupSpectrum(tspack, s, t,
			dsdx, dtdx, dsdy, dtdy);
	}
	virtual float Y() const {
		return mipmap->LookupFloat(CHANNEL_WMEAN, .5f, .5f, .5f);
	}
	virtual float Filter() const {
		return mipmap->LookupFloat(CHANNEL_MEAN, .5f, .5f, .5f);
	}
	virtual void GetDuv(const TsPack *tspack,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const {
		float s, t, dsdu, dtdu, dsdv, dtdv;
		mapping->MapDuv(dg, &s, &t, &dsdu, &dtdu, &dsdv, &dtdv);
		float ds, dt;
		mipmap->GetDifferentials(tspack, s, t, &ds, &dt);
		*du = ds * dsdu + dt * dtdu;
		*dv = ds * dsdv + dt * dtdv;
	}

	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);
};

// ImageTexture Method Definitions
inline boost::shared_ptr<MIPMap> ImageTexture::GetTexture(ImageTextureFilterType filterType,
	const string &filename, float maxAniso, ImageWrap wrap, float gain,
	float gamma)
{
	// Look for texture in texture cache
	static map<TexInfo, boost::shared_ptr<MIPMap> > textures;
	TexInfo texInfo(filterType, filename, maxAniso, wrap, gain, gamma);
	if (textures.find(texInfo) != textures.end())
		return textures[texInfo];
	int width, height;
	auto_ptr<ImageData> imgdata(ReadImage(filename));
	boost::shared_ptr<MIPMap> ret;
	if (imgdata.get() != NULL) {
		width=imgdata->getWidth();
		height=imgdata->getHeight();
		ret = boost::shared_ptr<MIPMap>(imgdata->createMIPMap(filterType, maxAniso, wrap, gain, gamma));
	} else {
		// Create one-valued _MIPMap_
		TextureColor<float, 1> oneVal(1.f);

		ret = boost::shared_ptr<MIPMap>(new MIPMapFastImpl<TextureColor<float, 1> >(filterType, 1, 1, &oneVal));
	}
	if (ret) {
		textures[texInfo] = ret;
		return textures[texInfo];
	}

	return boost::shared_ptr<MIPMap>();
}

}//namespace lux
