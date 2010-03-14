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

#ifndef LUX_MIPMAP_H
#define LUX_MIPMAP_H

#include "lux.h"
#include "texture.h"
#include "texturecolor.h"
#include "spectrum.h"
#include "memory.h"
#include "error.h"

namespace lux
{

// Dade - type of image filters
enum ImageTextureFilterType {
	NEAREST,
	BILINEAR,
	MIPMAP_TRILINEAR,
	MIPMAP_EWA
};

// MIPMap Declarations
typedef enum {
	TEXTURE_REPEAT,
	TEXTURE_BLACK,
	TEXTURE_CLAMP
} ImageWrap;

class MIPMap {
public:
	// MIPMap Public Methods
	virtual ~MIPMap() { };
	virtual float LookupFloat(Channel channel, float s, float t,
		float width = 0.f) const = 0;
	virtual SWCSpectrum LookupSpectrum(const TsPack *tspack,
		float s, float t, float width = 0.f) const = 0;
	virtual float LookupFloat(Channel channel, float s, float t,
		float ds0, float dt0, float ds1, float dt1) const = 0;
	virtual SWCSpectrum LookupSpectrum(const TsPack *tspack,
		float s, float t, float ds0, float dt0, float ds1, float dt1) const = 0;
	virtual void GetDifferentials(Channel channel, float s, float t,
		float *ds, float *dt) const = 0;
	virtual void GetDifferentials(const TsPack *tspack, float s, float t,
		float *ds, float *dt) const = 0;

	virtual u_int GetMemoryUsed() const = 0;
	virtual void DiscardMipmaps(u_int n) { }
};

template <class T> class MIPMapFastImpl : public MIPMap {
public:
	// MIPMapFastImpl Public Methods
	MIPMapFastImpl(ImageTextureFilterType type, u_int xres, u_int yres,
		const T *data, float maxAniso = 8.f,
		ImageWrap wrapMode = TEXTURE_REPEAT);
	virtual ~MIPMapFastImpl();

	virtual float LookupFloat(Channel channel, float s, float t,
		float width = 0.f) const;
	virtual float LookupFloat(Channel channel, float s, float t,
		float ds0, float dt0, float ds1, float dt1) const;
	virtual SWCSpectrum LookupSpectrum(const TsPack *tspack,
		float s, float t, float width = 0.f) const;
	virtual SWCSpectrum LookupSpectrum(const TsPack *tspack,
		float s, float t, float ds0, float dt0, float ds1, float dt1) const;
	virtual void GetDifferentials(Channel channel, float s, float t,
		float *ds, float *dt) const {
		switch (filterType) {
			case MIPMAP_TRILINEAR:
			case MIPMAP_EWA: {
				s *= uSize(0);
				const int is = Floor2Int(s);
				const float as = s - is;
				t *= vSize(0);
				const int it = Floor2Int(t);
				const float at = t - it;
				int s0, s1;
				if (as < .5f) {
					s0 = is - 1;
					s1 = is;
				} else {
					s0 = is;
					s1 = is + 1;
				}
				int t0, t1;
				if (at < .5f) {
					t0 = it - 1;
					t1 = it;
				} else {
					t0 = it;
					t1 = it + 1;
				}
				*ds = Lerp(at, Texel(channel, 0, s1, it) -
					Texel(channel, 0, s0, it),
					Texel(channel, 0, s1, it + 1) -
					Texel(channel, 0, s0, it + 1)) *
					uSize(0);
				*dt = Lerp(as, Texel(channel, 0, is, t1) -
					Texel(channel, 0, is, t1),
					Texel(channel, 0, is + 1, t1) -
					Texel(channel, 0, is + 1, t0)) *
					vSize(0);
				break;
			}
			case BILINEAR:
			case NEAREST: {
				s *= singleMap->uSize();
				const int is = Floor2Int(s);
				const float as = s - is;
				t *= singleMap->vSize();
				const int it = Floor2Int(t);
				const float at = t - it;
				int s0, s1;
				if (as < .5f) {
					s0 = is - 1;
					s1 = is;
				} else {
					s0 = is;
					s1 = is + 1;
				}
				int t0, t1;
				if (at < .5f) {
					t0 = it - 1;
					t1 = it;
				} else {
					t0 = it;
					t1 = it + 1;
				}
				*ds = Lerp(at, Texel(channel, s1, it) -
					Texel(channel, s0, it),
					Texel(channel, s1, it + 1) -
					Texel(channel, s0, it + 1)) *
					singleMap->uSize();
				*dt = Lerp(as, Texel(channel, is, t1) -
					Texel(channel, is, t1),
					Texel(channel, is + 1, t1) -
					Texel(channel, is + 1, t0)) *
					singleMap->vSize();
				break;
			}
		}
	}
	virtual void GetDifferentials(const TsPack *tspack, float s, float t,
		float *ds, float *dt) const {
		switch (filterType) {
			case MIPMAP_TRILINEAR:
			case MIPMAP_EWA: {
				s *= uSize(0);
				const int is = Floor2Int(s);
				const float as = s - is;
				t *= vSize(0);
				const int it = Floor2Int(t);
				const float at = t - it;
				int s0, s1;
				if (as < .5f) {
					s0 = is - 1;
					s1 = is;
				} else {
					s0 = is;
					s1 = is + 1;
				}
				int t0, t1;
				if (at < .5f) {
					t0 = it - 1;
					t1 = it;
				} else {
					t0 = it;
					t1 = it + 1;
				}
				*ds = Lerp(at, Texel(tspack, 0, s1, it).Filter(tspack) -
					Texel(tspack, 0, s0, it).Filter(tspack),
					Texel(tspack, 0, s1, it + 1).Filter(tspack) -
					Texel(tspack, 0, s0, it + 1).Filter(tspack)) *
					uSize(0);
				*dt = Lerp(as, Texel(tspack, 0, is, t1).Filter(tspack) -
					Texel(tspack, 0, is, t1).Filter(tspack),
					Texel(tspack, 0, is + 1, t1).Filter(tspack) -
					Texel(tspack, 0, is + 1, t0).Filter(tspack)) *
					vSize(0);
				break;
			}
			case BILINEAR:
			case NEAREST: {
				s *= singleMap->uSize();
				const int is = Floor2Int(s);
				const float as = s - is;
				t *= singleMap->vSize();
				const int it = Floor2Int(t);
				const float at = t - it;
				int s0, s1;
				if (as < .5f) {
					s0 = is - 1;
					s1 = is;
				} else {
					s0 = is;
					s1 = is + 1;
				}
				int t0, t1;
				if (at < .5f) {
					t0 = it - 1;
					t1 = it;
				} else {
					t0 = it;
					t1 = it + 1;
				}
				*ds = Lerp(at, Texel(tspack, 0, s1, it).Filter(tspack) -
					Texel(tspack, 0, s0, it).Filter(tspack),
					Texel(tspack, 0, s1, it + 1).Filter(tspack) -
					Texel(tspack, 0, s0, it + 1).Filter(tspack)) *
					singleMap->uSize();
				*dt = Lerp(as, Texel(tspack, 0, is, t1).Filter(tspack) -
					Texel(tspack, 0, is, t1).Filter(tspack),
					Texel(tspack, 0, is + 1, t1).Filter(tspack) -
					Texel(tspack, 0, is + 1, t0).Filter(tspack)) *
					singleMap->vSize();
				break;
			}
		}
	}

	virtual u_int GetMemoryUsed() const {
		switch (filterType) {
			case MIPMAP_EWA:
			case MIPMAP_TRILINEAR: {
				u_int size = 0;

				for (u_int i = 0; i < nLevels; ++i)
					size += pyramid[i]->uSize() *
						pyramid[i]->vSize() * sizeof(T);
				return size;
			}
			case NEAREST:
			case BILINEAR:
				return singleMap->uSize() *
					singleMap->vSize() * sizeof(T);
		}
		LOG(LUX_ERROR, LUX_SYSTEM) << "Internal error in MIPMapFastImpl::~MIPMapFastImpl(), unknown filter type";
		return 0;
	}

	virtual void DiscardMipmaps(u_int n) {
		for (u_int i = 0; i < n; ++i) {
			if (nLevels <= 1)
				return;

			delete pyramid[0];

			nLevels--;
			BlockedArray<T> **newPyramid = new BlockedArray<T> *[nLevels];
			for (u_int j = 0; j < nLevels; ++j)
				newPyramid[j] = pyramid[j + 1];

			delete[] pyramid;
			pyramid = newPyramid;
		}
	}

protected:
	// Dade - used by MIPMAP_EWA, MIPMAP_TRILINEAR
	float Texel(Channel channel, u_int level, int s, int t) const;
	SWCSpectrum Texel(const TsPack *tspack, u_int level,
		int s, int t) const;

	// Dade - used by NEAREST, BILINEAR
	float Texel(Channel channel, int s, int t) const;
	SWCSpectrum Texel(const TsPack *tspack, int s, int t) const;

private:
	// MIPMAPImpl Private data types
	struct ResampleWeight {
		int firstTexel;
		float weight[4];
	};
	// MIPMapFastImpl Private Methods
	ResampleWeight *ResampleWeights(u_int oldres, u_int newres) {
		BOOST_ASSERT(newres >= oldres);
		ResampleWeight *wt = new ResampleWeight[newres];
		const float filterwidth = 2.f;
		for (u_int i = 0; i < newres; ++i) {
			// Compute image resampling weights for _i_th texel
			const float center = (i + .5f) * oldres / newres;
			wt[i].firstTexel = Floor2Int(center - filterwidth + 0.5f);
			for (int j = 0; j < 4; ++j) {
				const float pos = wt[i].firstTexel + j + .5f;
				wt[i].weight[j] = Lanczos((pos - center) /
					filterwidth);
			}
			// Normalize filter weights for texel resampling
			const float invSumWts = 1.f / (wt[i].weight[0] +
				wt[i].weight[1] + wt[i].weight[2] +
				wt[i].weight[3]);
			for (int j = 0; j < 4; ++j)
				wt[i].weight[j] *= invSumWts;
		}
		return wt;
	}

	inline u_int uSize(u_int level) const { return pyramid[level]->uSize(); }
	inline u_int vSize(u_int level) const { return pyramid[level]->vSize(); }

	float Triangle(Channel channel, u_int level, float s, float t) const;
	SWCSpectrum Triangle(const TsPack *tspack, u_int level,
		float s, float t) const;
	float Triangle(Channel channel, float s, float t) const;
	SWCSpectrum Triangle(const TsPack *tspack, float s, float t) const;
	float Nearest(Channel channel, float s, float t) const;
	SWCSpectrum Nearest(const TsPack *tspack, float s, float t) const;
	float EWA(Channel channel, float s, float t,
		float ds0, float dt0, float ds1, float dt1, u_int level) const;
	SWCSpectrum EWA(const TsPack *tspack, float s, float t,
		float ds0, float dt0, float ds1, float dt1, u_int level) const;

	// MIPMap Private Data
	ImageTextureFilterType filterType;
	float maxAnisotropy;
	float gain;
	float gamma;
	ImageWrap wrapMode;
	u_int nLevels;
	union {
		BlockedArray<T> **pyramid;
		BlockedArray<T> *singleMap;
	};

#define WEIGHT_LUT_SIZE 128
	static float *weightLut;
};

template <class T> float *MIPMapFastImpl<T>::weightLut = NULL;

// MIPMapFastImpl Method Definitions
template <class T>
float MIPMapFastImpl<T>::LookupFloat(Channel channel, float s, float t,
	float width) const
{
	switch (filterType) {
		case MIPMAP_TRILINEAR:
		case MIPMAP_EWA: {
			// Compute MIPMap level for trilinear filtering
			const float level = nLevels - 1 +
				Log2(max(width, 1e-8f));
			// Perform trilinear interpolation at appropriate level
			if (level < 0)
				return Triangle(channel, 0, s, t);
			else if (level >= nLevels - 1)
				return Texel(channel, nLevels - 1,
					Floor2Int(s * uSize(nLevels - 1)),
					Floor2Int(t * vSize(nLevels - 1)));
			else {
				const u_int iLevel = Floor2UInt(level);
				const float delta = level - iLevel;
				return Lerp(delta,
					Triangle(channel, iLevel, s, t),
					Triangle(channel, iLevel + 1, s, t));
			}
		}
		case BILINEAR:
			return Triangle(channel, s, t);
		case NEAREST:
			return Nearest(channel, s, t);
	}
	LOG(LUX_ERROR, LUX_SYSTEM) << "Internal error in MIPMapFastImpl::Lookup()";
	return 1.f;
}
template <class T>
SWCSpectrum MIPMapFastImpl<T>::LookupSpectrum(const TsPack *tspack,
	float s, float t, float width) const
{
	switch (filterType) {
		case MIPMAP_TRILINEAR:
		case MIPMAP_EWA: {
			// Compute MIPMap level for trilinear filtering
			const float level = nLevels - 1 + Log2(width);
			// Perform trilinear interpolation at appropriate level
			if (level < 0)
				return Triangle(tspack, 0, s, t);
			else if (level >= nLevels - 1)
				return Texel(tspack, nLevels - 1,
					Floor2Int(s * uSize(nLevels - 1)),
					Floor2Int(t * vSize(nLevels - 1)));
			else {
				const u_int iLevel = Floor2UInt(level);
				const float delta = level - iLevel;
				return Lerp(delta,
					Triangle(tspack, iLevel, s, t),
					Triangle(tspack, iLevel + 1, s, t));
			}
		}
		case BILINEAR:
			return Triangle(tspack, s, t);
		case NEAREST:
			return Nearest(tspack, s, t);
	}
	LOG(LUX_ERROR, LUX_SYSTEM) << "Internal error in MIPMapFastImpl::Lookup()";
	return SWCSpectrum(1.f);
}

template <class T>
float MIPMapFastImpl<T>::Triangle(Channel channel, u_int level,
	float s, float t) const
{
	level = Clamp(level, 0U, nLevels - 1);
	s *= uSize(level);
	t *= vSize(level);
	const int s0 = Floor2Int(s), t0 = Floor2Int(t);
	const float ds = s - s0, dt = t - t0;
	return Lerp(ds,
		Lerp(dt, Texel(channel, level, s0, t0),
		Texel(channel, level, s0, t0 + 1)),
		Lerp(dt, Texel(channel, level, s0 + 1, t0),
		Texel(channel, level, s0 + 1, t0 + 1)));
}
template <class T>
SWCSpectrum MIPMapFastImpl<T>::Triangle(const TsPack *tspack, u_int level,
	float s, float t) const
{
	level = Clamp(level, 0U, nLevels - 1);
	s *= uSize(level);
	t *= vSize(level);
	const int s0 = Floor2Int(s), t0 = Floor2Int(t);
	const float ds = s - s0, dt = t - t0;
	return Lerp(ds,
		Lerp(dt, Texel(tspack, level, s0, t0),
		Texel(tspack, level, s0, t0 + 1)),
		Lerp(dt, Texel(tspack, level, s0 + 1, t0),
		Texel(tspack, level, s0 + 1, t0 + 1)));
}

template <class T>
float MIPMapFastImpl<T>::Triangle(Channel channel, float s, float t) const
{
	s *= singleMap->uSize();
	t *= singleMap->vSize();
	const int s0 = Floor2Int(s), t0 = Floor2Int(t);
	const float ds = s - s0, dt = t - t0;
	return Lerp(ds,
		Lerp(dt, Texel(channel, s0, t0), Texel(channel, s0, t0 + 1)),
		Lerp(dt, Texel(channel, s0 + 1, t0),
		Texel(channel, s0 + 1, t0 + 1)));
}
template <class T>
SWCSpectrum MIPMapFastImpl<T>::Triangle(const TsPack *tspack,
	float s, float t) const
{
	s *= singleMap->uSize();
	t *= singleMap->vSize();
	const int s0 = Floor2Int(s), t0 = Floor2Int(t);
	const float ds = s - s0, dt = t - t0;
	return Lerp(ds,
		Lerp(dt, Texel(tspack, s0, t0), Texel(tspack, s0, t0 + 1)),
		Lerp(dt, Texel(tspack, s0 + 1, t0),
		Texel(tspack, s0 + 1, t0 + 1)));
}

template <class T>
float MIPMapFastImpl<T>::Nearest(Channel channel, float s, float t) const
{
	s *= singleMap->uSize();
	t *= singleMap->vSize();
	const int s0 = Floor2Int(s), t0 = Floor2Int(t);
	return Texel(channel, s0, t0);
}
template <class T>
SWCSpectrum MIPMapFastImpl<T>::Nearest(const TsPack *tspack, float s, float t) const
{
	s *= singleMap->uSize();
	t *= singleMap->vSize();
	const int s0 = Floor2Int(s), t0 = Floor2Int(t);
	return Texel(tspack, s0, t0);
}

template <class T>
float MIPMapFastImpl<T>::LookupFloat(Channel channel, float s, float t,
	float ds0, float dt0, float ds1, float dt1) const
{
	switch (filterType) {
		case MIPMAP_TRILINEAR:
			return LookupFloat(channel, s, t,
				2.f * max(max(fabsf(ds0), fabsf(dt0)),
				max(fabsf(ds1), fabsf(dt1))));
		case MIPMAP_EWA: {
			// Compute ellipse minor and major axes
			if (ds0 * ds0 + dt0 * dt0 < ds1 * ds1 + dt1 * dt1) {
				swap(ds0, ds1);
				swap(dt0, dt1);
			}
			const float majorLength = sqrtf(ds0 * ds0 + dt0 * dt0);
			float minorLength = sqrtf(ds1 * ds1 + dt1 * dt1);

			// Clamp ellipse eccentricity if too large
			if (minorLength * maxAnisotropy < majorLength) {
				const float scale = majorLength /
					(minorLength * maxAnisotropy);
				ds1 *= scale;
				dt1 *= scale;
				minorLength *= scale;
			}

			// Choose level of detail for EWA lookup and perform EWA filtering
			const float lod = nLevels - 1 + Log2(minorLength);
			if (lod <= 0.f)
				return Triangle(channel, 0, s, t);
			else if (lod >= nLevels - 1)
				return Texel(channel, nLevels - 1,
					Floor2Int(s * uSize(nLevels - 1)),
					Floor2Int(t * vSize(nLevels - 1)));
			else {
				const u_int ilod = Floor2UInt(lod);
				const float d = lod - ilod;
				return Lerp(d, EWA(channel, s, t,
					ds0, dt0, ds1, dt1, ilod),
					EWA(channel, s, t,
					ds0, dt0, ds1, dt1, ilod + 1));
			}
		}
		case BILINEAR:
			return Triangle(channel, s, t);
		case NEAREST:
			return Nearest(channel, s, t);
	}
	LOG(LUX_ERROR, LUX_SYSTEM) << "Internal error in MIPMapFastImpl::Lookup()";
	return 1.f;
}
template <class T>
SWCSpectrum MIPMapFastImpl<T>::LookupSpectrum(const TsPack *tspack,
	float s, float t, float ds0, float dt0, float ds1, float dt1) const
{
	switch (filterType) {
		case MIPMAP_TRILINEAR:
			return LookupSpectrum(tspack, s, t,
				2.f * max(max(fabsf(ds0), fabsf(dt0)),
				max(fabsf(ds1), fabsf(dt1))));
		case MIPMAP_EWA: {
			// Compute ellipse minor and major axes
			if (ds0 * ds0 + dt0 * dt0 < ds1 * ds1 + dt1 * dt1) {
				swap(ds0, ds1);
				swap(dt0, dt1);
			}
			const float majorLength = sqrtf(ds0 * ds0 + dt0 * dt0);
			float minorLength = sqrtf(ds1 * ds1 + dt1 * dt1);

			// Clamp ellipse eccentricity if too large
			if (minorLength * maxAnisotropy < majorLength) {
				const float scale = majorLength /
					(minorLength * maxAnisotropy);
				ds1 *= scale;
				dt1 *= scale;
				minorLength *= scale;
			}

			// Choose level of detail for EWA lookup and perform EWA filtering
			const float lod = nLevels - 1 + Log2(minorLength);
			if (lod <= 0.f)
				return Triangle(tspack, 0, s, t);
			else if (lod >= nLevels - 1)
				return Texel(tspack, nLevels - 1,
					Floor2Int(s * uSize(nLevels - 1)),
					Floor2Int(t * vSize(nLevels - 1)));
			else {
				const u_int ilod = Floor2UInt(lod);
				const float d = lod - ilod;
				return Lerp(d, EWA(tspack, s, t,
					ds0, dt0, ds1, dt1, ilod),
					EWA(tspack, s, t,
					ds0, dt0, ds1, dt1, ilod + 1));
			}
		}
		case BILINEAR:
			return Triangle(tspack, s, t);
		case NEAREST:
			return Nearest(tspack, s, t);
	}
	LOG(LUX_ERROR, LUX_SYSTEM) << "Internal error in MIPMapFastImpl::Lookup()";
	return SWCSpectrum(1.f);
}

template <class T>
float MIPMapFastImpl<T>::EWA(Channel channel, float s, float t,
	float ds0, float dt0, float ds1, float dt1, u_int level) const
{
	s = s * uSize(level);
	t = t * vSize(level);
	if (level >= nLevels)
		return Texel(channel, nLevels - 1, Floor2Int(s), Floor2Int(t));
	// Convert EWA coordinates to appropriate scale for level
	ds0 *= uSize(level);
	dt0 *= vSize(level);
	ds1 *= uSize(level);
	dt1 *= vSize(level);
	// Compute ellipse coefficients to bound EWA filter region
	float A = dt0 * dt0 + dt1 * dt1 + 1.f;
	float B = -2.f * (ds0 * dt0 + ds1 * dt1);
	float C = ds0 * ds0 + ds1 * ds1 + 1.f;
	const float F = A * C - B * B * 0.25f;
	// Compute the ellipse's $(s,t)$ bounding box in texture space
	const float du = sqrtf(C), dv = sqrtf(A);
	const int s0 = Ceil2Int(s - du);
	const int s1 = Floor2Int(s + du);
	const int t0 = Ceil2Int(t - dv);
	const int t1 = Floor2Int(t + dv);

	const float invF = 1.f / F;
	A *= invF;
	B *= invF;
	C *= invF;
	// Scan over ellipse bound and compute quadratic equation
	float num = 0.f;
	float den = 0.f;
	for (int it = t0; it <= t1; ++it) {
		const float tt = it - t;
		for (int is = s0; is <= s1; ++is) {
			const float ss = is - s;
			// Compute squared radius and filter texel if inside ellipse
			const float r2 = A * ss * ss + B * ss * tt + C * tt * tt;
			if (r2 < 1.f) {
				const float weight =
					weightLut[min(Float2Int(r2 *
					WEIGHT_LUT_SIZE), WEIGHT_LUT_SIZE - 1)];
				num += Texel(channel, level, is, it) * weight;
				den += weight;
			}
		}
	}

	return num / den;
}
template <class T>
SWCSpectrum MIPMapFastImpl<T>::EWA(const TsPack *tspack, float s, float t,
	float ds0, float dt0, float ds1, float dt1, u_int level) const
{
	s = s * uSize(level);
	t = t * vSize(level);
	if (level >= nLevels)
		return Texel(tspack, nLevels - 1, Floor2Int(s), Floor2Int(t));
	// Convert EWA coordinates to appropriate scale for level
	ds0 *= uSize(level);
	dt0 *= vSize(level);
	ds1 *= uSize(level);
	dt1 *= vSize(level);
	// Compute ellipse coefficients to bound EWA filter region
	float A = dt0 * dt0 + dt1 * dt1 + 1.f;
	float B = -2.f * (ds0 * dt0 + ds1 * dt1);
	float C = ds0 * ds0 + ds1 * ds1 + 1.f;
	const float F = A * C - B * B * 0.25f;
	// Compute the ellipse's $(s,t)$ bounding box in texture space
	const float du = sqrtf(C), dv = sqrtf(A);
	const int s0 = Ceil2Int(s - du);
	const int s1 = Floor2Int(s + du);
	const int t0 = Ceil2Int(t - dv);
	const int t1 = Floor2Int(t + dv);

	const float invF = 1.f / F;
	A *= invF;
	B *= invF;
	C *= invF;
	// Scan over ellipse bound and compute quadratic equation
	SWCSpectrum num(0.f);
	float den = 0.f;
	for (int it = t0; it <= t1; ++it) {
		const float tt = it - t;
		for (int is = s0; is <= s1; ++is) {
			const float ss = is - s;
			// Compute squared radius and filter texel if inside ellipse
			const float r2 = A * ss * ss + B * ss * tt + C * tt * tt;
			if (r2 < 1.f) {
				const float weight =
					weightLut[min(Float2Int(r2 *
					WEIGHT_LUT_SIZE), WEIGHT_LUT_SIZE - 1)];
				num += Texel(tspack, level, is, it) * weight;
				den += weight;
			}
		}
	}

	return num / den;
}

template <class T>
MIPMapFastImpl<T>::~MIPMapFastImpl()
{
	switch (filterType) {
		case MIPMAP_TRILINEAR:
		case MIPMAP_EWA:
			for (u_int i = 0; i < nLevels; ++i)
				delete pyramid[i];
			delete[] pyramid;
			break;
		case BILINEAR:
		case NEAREST:
			delete singleMap;
			break;
		default:
			LOG(LUX_ERROR, LUX_SYSTEM) << "Internal error in MIPMapFastImpl::~MIPMapFastImpl(), unknown filter type";
	}
}

template <class T>
MIPMapFastImpl<T>::MIPMapFastImpl(ImageTextureFilterType type, u_int sres, u_int tres,
	const T *img, float maxAniso, ImageWrap wm)
{
	filterType = type;
	maxAnisotropy = maxAniso;
	wrapMode = wm;

	switch (filterType) {
	case MIPMAP_TRILINEAR:
	case MIPMAP_EWA: {
		T *resampledImage = NULL;
		if (!IsPowerOf2(sres) || !IsPowerOf2(tres)) {
			// Resample image to power-of-two resolution
			const u_int sPow2 = RoundUpPow2(sres), tPow2 = RoundUpPow2(tres);
			LOG(LUX_INFO, LUX_NOERROR) <<
				"Resampling image from " << sres << "x" <<
				tres << " to " << sPow2 << "x" << tPow2;

			// Resample image in $s$ direction
			struct ResampleWeight *sWeights = ResampleWeights(sres, sPow2);
			resampledImage = new T[sPow2 * tPow2];

			// Apply _sWeights_ to zoom in $s$ direction
			for (u_int t = 0; t < tres; ++t) {
				for (u_int s = 0; s < sPow2; ++s) {
					// Compute texel $(s,t)$ in $s$-zoomed image
					resampledImage[t * sPow2 + s] = T();
					// NOTE - Ratow - Offsetting weights to minimize possible over/underflows
					for (int jo = 2; jo < 6; ++jo) {
						const int j = jo % 4;

						int origS = sWeights[s].firstTexel + j;
						switch (wrapMode) {
						case TEXTURE_REPEAT:
							origS = Mod(origS, static_cast<int>(sres));
							break;
						case TEXTURE_CLAMP:
							origS = Clamp(origS, 0, static_cast<int>(sres - 1));
							break;
						case TEXTURE_BLACK:
							break;
						}

						if (origS >= 0 && origS < static_cast<int>(sres)) {
							if (sWeights[s].weight[j] > 0.f)
								resampledImage[t * sPow2 + s] += sWeights[s].weight[j] * img[t * sres + origS];
							else /* TextureColor cannot be negative so we invert and subtract */
								resampledImage[t * sPow2 + s] -= (-sWeights[s].weight[j]) * img[t * sres + origS];
						}
					}
				}
			}
			delete[] sWeights;

			// Resample image in $t$ direction
			struct ResampleWeight *tWeights = ResampleWeights(tres, tPow2);
			T *workData = new T[tPow2];
			for (u_int s = 0; s < sPow2; ++s) {
				for (u_int t = 0; t < tPow2; ++t) {
					workData[t] = T();
					// NOTE - Ratow - Offsetting weights to minimize possible over/underflows
					for (int jo = 2; jo < 6; ++jo) {
						const int j = jo % 4;

						int origT = tWeights[t].firstTexel + j;
						switch (wrapMode) {
						case TEXTURE_REPEAT:
							origT = Mod(origT, static_cast<int>(tres));
							break;
						case TEXTURE_CLAMP:
							origT = Clamp(origT, 0, static_cast<int>(tres - 1));
							break;
						case TEXTURE_BLACK:
							break;
						}

						if (origT >= 0 && origT < static_cast<int>(tres)) {
							if(tWeights[t].weight[j] > 0.f)
								workData[t] += tWeights[t].weight[j] * resampledImage[origT * sPow2 + s];
							else /* TextureColor cannot be negative so we invert and subtract */
								workData[t] -= (-tWeights[t].weight[j]) * resampledImage[origT * sPow2 + s];
						}
					}
				}
				for (u_int t = 0; t < tPow2; ++t)
					resampledImage[t * sPow2 + s] = workData[t].Clamp(0.f, INFINITY);
			}
			delete[] workData;
			delete[] tWeights;
			img = resampledImage;

			sres = sPow2;
			tres = tPow2;
		}

		// Initialize levels of MIPMap from image
		nLevels = 1 + static_cast<u_int>(max(0, Log2Int(static_cast<float>(max(sres, tres)))));

		LOG(LUX_INFO, LUX_NOERROR) << "Generating " << nLevels <<
			" mipmap levels";

		pyramid = new BlockedArray<T> *[this->nLevels];
		// Initialize most detailed level of MIPMap
		pyramid[0] = new BlockedArray<T>(sres, tres, img);
		for (u_int i = 1; i < nLevels; ++i) {
			// Initialize $i$th MIPMap level from $i-1$st level
			const u_int sRes = max<u_int>(1,
				pyramid[i - 1]->uSize() / 2);
			const u_int tRes = max<u_int>(1,
				pyramid[i - 1]->vSize() / 2);
			pyramid[i] = new BlockedArray<T>(sRes, tRes);
			// Filter four texels from finer level of pyramid
			for (u_int t = 0; t < tRes; ++t) {
				for (u_int s = 0; s < sRes; ++s) {
					/* NOTE - Ratow - multiplying before summing all TextureColors because they can overflow */
					(*pyramid[i])(s, t) =
						0.25f * (*pyramid[i - 1])(2 * s, 2 * t) +
						0.25f * (*pyramid[i - 1])(2 * s + 1, 2 * t) +
						0.25f * (*pyramid[i - 1])(2 * s, 2 * t + 1) +
						0.25f * (*pyramid[i - 1])(2 * s + 1, 2 * t + 1);
				}
			}
		}
		if (resampledImage)
			delete[] resampledImage;

		// Initialize EWA filter weights if needed
		if (!weightLut) {
			weightLut = AllocAligned<float>(WEIGHT_LUT_SIZE);
			for (u_int i = 0; i < WEIGHT_LUT_SIZE; ++i) {
				const float alpha = 2.f;
				const float r2 = static_cast<float>(i) / static_cast<float>(WEIGHT_LUT_SIZE - 1);
				weightLut[i] = expf(-alpha * r2) - expf(-alpha);
			}
		}
		break;
	}
	case BILINEAR:
	case NEAREST:
		singleMap = new BlockedArray<T>(sres, tres, img);
		nLevels = 0;
		break;
	default:
		LOG(LUX_ERROR, LUX_SYSTEM) << "Internal error in MIPMapFastImpl::MIPMapFastImpl(), unknown filter type";
	}
}

template <class T>
float MIPMapFastImpl<T>::Texel(Channel channel, u_int level, int s, int t) const
{
	const BlockedArray<T> &l = *pyramid[level];
	// Compute texel $(s,t)$ accounting for boundary conditions
	switch (wrapMode) {
		case TEXTURE_REPEAT:
			s = Mod(s, static_cast<int>(l.uSize()));
			t = Mod(t, static_cast<int>(l.vSize()));
			break;
		case TEXTURE_CLAMP:
			s = Clamp(s, 0, static_cast<int>(l.uSize()) - 1);
			t = Clamp(t, 0, static_cast<int>(l.vSize()) - 1);
			break;
		case TEXTURE_BLACK:
			if (s < 0 || s >= static_cast<int>(l.uSize()) ||
				t < 0 || t >= static_cast<int>(l.vSize()))
			return 0.f;
	}

	return l(s, t).GetFloat(channel);
}
template <class T>
SWCSpectrum MIPMapFastImpl<T>::Texel(const TsPack *tspack, u_int level,
	int s, int t) const
{
	const BlockedArray<T> &l = *pyramid[level];
	// Compute texel $(s,t)$ accounting for boundary conditions
	switch (wrapMode) {
		case TEXTURE_REPEAT:
			s = Mod(s, static_cast<int>(l.uSize()));
			t = Mod(t, static_cast<int>(l.vSize()));
			break;
		case TEXTURE_CLAMP:
			s = Clamp(s, 0, static_cast<int>(l.uSize()) - 1);
			t = Clamp(t, 0, static_cast<int>(l.vSize()) - 1);
			break;
		case TEXTURE_BLACK:
			if (s < 0 || s >= static_cast<int>(l.uSize()) ||
				t < 0 || t >= static_cast<int>(l.vSize()))
			return SWCSpectrum(0.f);
	}

	return l(s, t).GetSpectrum(tspack);
}

template <class T>
float MIPMapFastImpl<T>::Texel(Channel channel, int s, int t) const
{
	const BlockedArray<T> &l = *singleMap;
	// Compute texel $(s,t)$ accounting for boundary conditions
	switch (wrapMode) {
		case TEXTURE_REPEAT:
			s = Mod(s, static_cast<int>(l.uSize()));
			t = Mod(t, static_cast<int>(l.vSize()));
			break;
		case TEXTURE_CLAMP:
			s = Clamp(s, 0, static_cast<int>(l.uSize()) - 1);
			t = Clamp(t, 0, static_cast<int>(l.vSize()) - 1);
			break;
		case TEXTURE_BLACK:
			if (s < 0 || s >= static_cast<int>(l.uSize()) ||
				t < 0 || t >= static_cast<int>(l.vSize()))
			return 0.f;
	}

	return l(s, t).GetFloat(channel);
}
template <class T>
SWCSpectrum MIPMapFastImpl<T>::Texel(const TsPack *tspack, int s, int t) const
{
	const BlockedArray<T> &l = *singleMap;
	// Compute texel $(s,t)$ accounting for boundary conditions
	switch (wrapMode) {
		case TEXTURE_REPEAT:
			s = Mod(s, static_cast<int>(l.uSize()));
			t = Mod(t, static_cast<int>(l.vSize()));
			break;
		case TEXTURE_CLAMP:
			s = Clamp(s, 0, static_cast<int>(l.uSize()) - 1);
			t = Clamp(t, 0, static_cast<int>(l.vSize()) - 1);
			break;
		case TEXTURE_BLACK:
			if (s < 0 || s >= static_cast<int>(l.uSize()) ||
				t < 0 || t >= static_cast<int>(l.vSize()))
			return SWCSpectrum(0.f);
	}

	return l(s, t).GetSpectrum(tspack);
}

template <class T> class MIPMapImpl : public MIPMapFastImpl<T> {
public:
	// MIPMapFastImpl Public Methods
	MIPMapImpl(ImageTextureFilterType type, u_int xres, u_int yres,
		const T *data, float maxAniso = 8.f,
		ImageWrap wrapMode = TEXTURE_REPEAT,
		float s = 1.f, float g = 1.f) :
		MIPMapFastImpl<T>(type, xres, yres, data, maxAniso, wrapMode),
		gain(s), gamma(g) { };
	virtual ~MIPMapImpl() { }
	virtual float LookupFloat(Channel channel, float s, float t,
		float width = 0.f) const {
		return powf(gain * MIPMapFastImpl<T>::LookupFloat(channel, s, t,
			width), gamma);
	}
	virtual SWCSpectrum LookupSpectrum(const TsPack *tspack,
		float s, float t, float width = 0.f) const {
		return (gain * MIPMapFastImpl<T>::LookupSpectrum(tspack, s, t,
			width)).Pow(gamma);
	}
	virtual float LookupFloat(Channel channel, float s, float t,
		float ds0, float dt0, float ds1, float dt1) const {
		return powf(gain * MIPMapFastImpl<T>::LookupFloat(channel, s, t,
			ds0, dt0, ds1, dt1), gamma);
	}
	virtual SWCSpectrum LookupSpectrum(const TsPack *tspack,
		float s, float t, float ds0, float dt0, float ds1, float dt1) const {
		return (gain * MIPMapFastImpl<T>::LookupSpectrum(tspack, s, t,
			ds0, dt0, ds1, dt1)).Pow(gamma);
	}
private:
	float gain, gamma;
};

}//namespace lux

#endif // LUX_MIPMAP_H
