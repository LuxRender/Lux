/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_COLORBASE_H
#define LUX_COLORBASE_H
// color.h*
#include "lux.h"
#include "color.h"
#include "spectrum.h"

#include <boost/serialization/access.hpp>

#ifdef WIN32
#undef max
#undef min
#endif // WIN32
#include <limits>

namespace lux
{

typedef enum {
	CHANNEL_RED,
	CHANNEL_GREEN,
	CHANNEL_BLUE,
	CHANNEL_ALPHA,
	CHANNEL_MEAN,
	CHANNEL_WMEAN
} Channel;

template <class T, u_int colorSamples> class TextureColor
{
	friend class boost::serialization::access;
public:
	TextureColor(T v = 0) {
		for (u_int i = 0; i < colorSamples; ++i)
			c[i] = v;
	}
	TextureColor(T cs[colorSamples]) {
		for (u_int i = 0; i < colorSamples; ++i)
			c[i] = cs[i];
	}
	~TextureColor() { }

	SWCSpectrum GetSpectrum(const SpectrumWavelengths &sw) const;
	float GetFloat(Channel channel) const;
	RGBAColor GetRGBAColor() const;

	TextureColor<T, colorSamples> &operator+=(const TextureColor<T, colorSamples> &s2) {
		for (u_int i = 0; i < colorSamples; ++i)
			c[i] += min<T>(s2.c[i],
				std::numeric_limits<T>::max() - c[i]);
		return *this;
	}
	TextureColor<T, colorSamples> &operator-=(const TextureColor<T, colorSamples> &s2) {
		for (u_int i = 0; i < colorSamples; ++i)
			c[i] -= min<T>(s2.c[i],
				c[i] - std::numeric_limits<T>::min());
		return *this;
	}
	TextureColor<T, colorSamples> operator+(const TextureColor<T, colorSamples> &s2) const {
		TextureColor<T, colorSamples> ret = *this;
		ret += s2;
		return ret;
	}
	TextureColor<T, colorSamples> operator-(const TextureColor<T, colorSamples> &s2) const {
		TextureColor<T, colorSamples> ret = *this;
		ret -= s2;
		return ret;
	}
	TextureColor<T, colorSamples> operator/(const TextureColor<T, colorSamples> &s2) const {
		TextureColor<T, colorSamples> ret = *this;
		for (u_int i = 0; i < colorSamples; ++i)
			ret.c[i] /= s2.c[i];
		return ret;
	}
	TextureColor<T, colorSamples> operator*(const TextureColor<T, colorSamples> &sp) const {
		TextureColor<T, colorSamples> ret = *this;
		ret *= sp;
		return ret;
	}
	TextureColor<T, colorSamples> &operator*=(const TextureColor<T, colorSamples> &sp) {
		for (u_int i = 0; i < colorSamples; ++i)
			if (c[i] != 0 && sp.c[i] > std::numeric_limits<T>::max() / c[i])
				c[i] = std::numeric_limits<T>::max();
			else
				c[i] *= sp.c[i];
		return *this;
	}
	TextureColor<T, colorSamples> operator*(float a) const {
		TextureColor<T, colorSamples> ret = *this;
		ret *= a;
		return ret;
	}
	TextureColor<T, colorSamples> &operator*=(float a) {
		for (u_int i = 0; i < colorSamples; ++i)
			if (c[i] != 0 && a > std::numeric_limits<T>::max() / static_cast<float>(c[i]))
				c[i] = std::numeric_limits<T>::max();
			else
				c[i] *= a;
		return *this;
	}
	friend inline TextureColor<T, colorSamples> operator*(float a, const TextureColor<T, colorSamples> &s) {
		return s * a;
	}
	TextureColor<T,colorSamples> operator/(float a) const {
		return *this * (1.f / a);
	}
	TextureColor<T,colorSamples> &operator/=(float a) {
		return *this *= 1.f / a;
	}
	void AddWeighted(float w, const TextureColor<T, colorSamples> &s) {
		*this += s * w;
	}
	bool operator==(const TextureColor<T, colorSamples> &sp) const {
		for (u_int i = 0; i < colorSamples; ++i)
			if (c[i] != sp.c[i])
				return false;
		return true;
	}
	bool operator!=(const TextureColor<T,colorSamples> &sp) const {
		for (u_int i = 0; i < colorSamples; ++i)
			if (c[i] == sp.c[i])
				return false;
		return true;
	}
	TextureColor<T, colorSamples> operator-() const {
		TextureColor<T, colorSamples> ret;
		for (u_int i = 0; i < colorSamples; ++i)
			ret.c[i] = -c[i];
		return ret;
	}
	TextureColor<T, colorSamples> Clamp(float low = 0.f,
		float high = INFINITY) const {
		TextureColor<T, colorSamples> ret;
		for (u_int i = 0; i < colorSamples; ++i)
			ret.c[i] = ::Clamp<float>(c[i], low, high);
		return ret;
	}
	// Color Public Data
	T c[colorSamples];
};

// Specializations
// unsigned char
template<> inline SWCSpectrum TextureColor<unsigned char, 1>::GetSpectrum(const SpectrumWavelengths &sw) const {
	const float norm = 1.f / std::numeric_limits<unsigned char>::max();
	return SWCSpectrum(c[0] * norm);
}
template<> inline float TextureColor<unsigned char, 1>::GetFloat(Channel channel) const {
	const float norm = 1.f / std::numeric_limits<unsigned char>::max();
	return c[0] * norm;
}
template<> inline RGBAColor TextureColor<unsigned char, 1>::GetRGBAColor() const {
	const float norm = 1.f / std::numeric_limits<unsigned char>::max();
	return RGBAColor(c[0] * norm);
}
template<> inline SWCSpectrum TextureColor<unsigned char, 3>::GetSpectrum(const SpectrumWavelengths &sw) const {
	const float norm = 1.f / std::numeric_limits<unsigned char>::max();
	return SWCSpectrum(sw, RGBColor(c[0], c[1], c[2]) * norm);
}
template<> inline float TextureColor<unsigned char, 3>::GetFloat(Channel channel) const {
	const float norm = 1.f / std::numeric_limits<unsigned char>::max();
	switch (channel) {
		case CHANNEL_RED:
			return c[0] * norm;
		case CHANNEL_GREEN:
			return c[1] * norm;
		case CHANNEL_BLUE:
			return c[2] * norm;
		case CHANNEL_ALPHA:
			return 1.f;
		case CHANNEL_MEAN:
			return RGBColor(c[0], c[1], c[2]).Filter() * norm;
		case CHANNEL_WMEAN:
			return RGBColor(c[0], c[1], c[2]).Y() * norm;
	}
	return 1.f;
}
template<> inline RGBAColor TextureColor<unsigned char, 3>::GetRGBAColor() const {
	const float norm = 1.f / std::numeric_limits<unsigned char>::max();
	return RGBAColor(c[0], c[1], c[2]) * norm;
}
template<> inline SWCSpectrum TextureColor<unsigned char, 4>::GetSpectrum(const SpectrumWavelengths &sw) const {
	const float norm = 1.f / std::numeric_limits<unsigned char>::max();
	return SWCSpectrum(sw, RGBColor(c[0], c[1], c[2]) * norm);
}
template<> inline float TextureColor<unsigned char, 4>::GetFloat(Channel channel) const {
	const float norm = 1.f / std::numeric_limits<unsigned char>::max();
	switch (channel) {
		case CHANNEL_RED:
			return c[0] * norm;
		case CHANNEL_GREEN:
			return c[1] * norm;
		case CHANNEL_BLUE:
			return c[2] * norm;
		case CHANNEL_ALPHA:
			return c[3] * norm;
		case CHANNEL_MEAN:
			return RGBColor(c[0], c[1], c[2]).Filter() * norm;
		case CHANNEL_WMEAN:
			return RGBColor(c[0], c[1], c[2]).Y() * norm;
	}
	return 1.f;
}
template<> inline RGBAColor TextureColor<unsigned char, 4>::GetRGBAColor() const {
	const float norm = 1.f / std::numeric_limits<unsigned char>::max();
	return RGBAColor(c[0], c[1], c[2], c[3]) * norm;
}

// unsigned short
template<> inline SWCSpectrum TextureColor<unsigned short, 1>::GetSpectrum(const SpectrumWavelengths &sw) const {
	const float norm = 1.f / std::numeric_limits<unsigned short>::max();
	return SWCSpectrum(c[0] * norm);
}
template<> inline float TextureColor<unsigned short, 1>::GetFloat(Channel channel) const {
	const float norm = 1.f / std::numeric_limits<unsigned short>::max();
	return c[0] * norm;
}
template<> inline RGBAColor TextureColor<unsigned short, 1>::GetRGBAColor() const {
	const float norm = 1.f / std::numeric_limits<unsigned short>::max();
	return RGBAColor(c[0]) * norm;
}
template<> inline SWCSpectrum TextureColor<unsigned short, 3>::GetSpectrum(const SpectrumWavelengths &sw) const {
	const float norm = 1.f / std::numeric_limits<unsigned short>::max();
	return SWCSpectrum(sw, RGBColor(c[0], c[1], c[2]) * norm);
}
template<> inline float TextureColor<unsigned short, 3>::GetFloat(Channel channel) const {
	const float norm = 1.f / std::numeric_limits<unsigned short>::max();
	switch (channel) {
		case CHANNEL_RED:
			return c[0] * norm;
		case CHANNEL_GREEN:
			return c[1] * norm;
		case CHANNEL_BLUE:
			return c[2] * norm;
		case CHANNEL_ALPHA:
			return 1.f;
		case CHANNEL_MEAN:
			return RGBColor(c[0], c[1], c[2]).Filter() * norm;
		case CHANNEL_WMEAN:
			return RGBColor(c[0], c[1], c[2]).Y() * norm;
	}
	return 1.f;
}
template<> inline RGBAColor TextureColor<unsigned short, 3>::GetRGBAColor() const {
	const float norm = 1.f / std::numeric_limits<unsigned short>::max();
	return RGBAColor(c[0], c[1], c[2]) * norm;
}
template<> inline SWCSpectrum TextureColor<unsigned short, 4>::GetSpectrum(const SpectrumWavelengths &sw) const {
	const float norm = 1.f / std::numeric_limits<unsigned short>::max();
	return SWCSpectrum(sw, RGBColor(c[0], c[1], c[2]) * norm);
}
template<> inline float TextureColor<unsigned short, 4>::GetFloat(Channel channel) const {
	const float norm = 1.f / std::numeric_limits<unsigned short>::max();
	switch (channel) {
		case CHANNEL_RED:
			return c[0] * norm;
		case CHANNEL_GREEN:
			return c[1] * norm;
		case CHANNEL_BLUE:
			return c[2] * norm;
		case CHANNEL_ALPHA:
			return c[3] * norm;
		case CHANNEL_MEAN:
			return RGBColor(c[0], c[1], c[2]).Filter() * norm;
		case CHANNEL_WMEAN:
			return RGBColor(c[0], c[1], c[2]).Y() * norm;
	}
	return 1.f;
}
template<> inline RGBAColor TextureColor<unsigned short, 4>::GetRGBAColor() const {
	const float norm = 1.f / std::numeric_limits<unsigned short>::max();
	return RGBAColor(c[0], c[1], c[2], c[3]) * norm;
}

// float
template<> inline SWCSpectrum TextureColor<float, 1>::GetSpectrum(const SpectrumWavelengths &sw) const {
	return SWCSpectrum(c[0]);
}
template<> inline float TextureColor<float, 1>::GetFloat(Channel channel) const {
	return c[0];
}
template<> inline RGBAColor TextureColor<float, 1>::GetRGBAColor() const {
	return RGBAColor(c[0]);
}
template<> inline SWCSpectrum TextureColor<float, 3>::GetSpectrum(const SpectrumWavelengths &sw) const {
	return SWCSpectrum(sw, RGBColor(c[0], c[1], c[2]));
}
template<> inline float TextureColor<float, 3>::GetFloat(Channel channel) const {
	switch (channel) {
		case CHANNEL_RED:
			return c[0];
		case CHANNEL_GREEN:
			return c[1];
		case CHANNEL_BLUE:
			return c[2];
		case CHANNEL_ALPHA:
			return 1.f;
		case CHANNEL_MEAN:
			return RGBColor(c[0], c[1], c[2]).Filter();
		case CHANNEL_WMEAN:
			return RGBColor(c[0], c[1], c[2]).Y();
	}
	return 1.f;
}
template<> inline RGBAColor TextureColor<float, 3>::GetRGBAColor() const {
	return RGBAColor(c[0], c[1], c[2]);
}
template<> inline SWCSpectrum TextureColor<float, 4>::GetSpectrum(const SpectrumWavelengths &sw) const {
	return SWCSpectrum(sw, RGBColor(c[0], c[1], c[2]));
}
template<> inline float TextureColor<float, 4>::GetFloat(Channel channel) const {
	switch (channel) {
		case CHANNEL_RED:
			return c[0];
		case CHANNEL_GREEN:
			return c[1];
		case CHANNEL_BLUE:
			return c[2];
		case CHANNEL_ALPHA:
			return c[3];
		case CHANNEL_MEAN:
			return RGBColor(c[0], c[1], c[2]).Filter();
		case CHANNEL_WMEAN:
			return RGBColor(c[0], c[1], c[2]).Y();
	}
	return 1.f;
}
template<> inline RGBAColor TextureColor<float, 4>::GetRGBAColor() const {
	return RGBAColor(c[0], c[1], c[2], c[3]);
}

}
#endif // LUX_COLORBASE_H
