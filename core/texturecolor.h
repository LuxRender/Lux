/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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

#ifdef WIN32
#undef max
#undef min
#include <limits>
#else
#include <limits.h>
#endif // WIN32

namespace lux
{
	class TextureColorBase
	{
	public:
		TextureColorBase(){};
	};

	template <class T,int colorSamples> class TextureColor : public TextureColorBase {
			friend class boost::serialization::access;
		public:
			TextureColor(T v = 0) {
				for (int i = 0; i < colorSamples; ++i)
					c[i] = v;
			}
			TextureColor(T cs[colorSamples]) {
			for (int i = 0; i < colorSamples; ++i)
				c[i] = cs[i];
			}

			TextureColor<T,colorSamples> &operator+=(const TextureColor<T,colorSamples> &s2) {
			for (int i = 0; i < colorSamples; ++i)
				if (c[i] > std::numeric_limits<T>::max() - s2.c[i])
					c[i] = std::numeric_limits<T>::max();
				else
					c[i] += s2.c[i];
			return *this;
			}
			TextureColor<T,colorSamples> operator+(const TextureColor<T,colorSamples>  &s2) const {
				TextureColor<T,colorSamples> ret = *this;
				for (int i = 0; i < colorSamples; ++i)
					if (ret.c[i] > std::numeric_limits<T>::max() - s2.c[i])
						ret.c[i] = std::numeric_limits<T>::max();
					else
						ret.c[i] += s2.c[i];
				return ret;
			}
			TextureColor<T,colorSamples> operator-(const TextureColor<T,colorSamples>  &s2) const {
				TextureColor<T,colorSamples> ret = *this;
				for (int i = 0; i < colorSamples; ++i)
					if (c[i] < s2.c[i])
						ret.c[i] = 0;
					else
						ret.c[i] -= s2.c[i];
				return ret;
			}
			TextureColor<T,colorSamples> operator/(const TextureColor<T,colorSamples>  &s2) const {
				TextureColor<T,colorSamples> ret = *this;
				for (int i = 0; i < colorSamples; ++i)
					ret.c[i] /= s2.c[i];
				return ret;
			}
			TextureColor<T,colorSamples> operator*(const TextureColor<T,colorSamples> &sp) const {
				TextureColor<T,colorSamples> ret = *this;
				for (int i = 0; i < colorSamples; ++i)
					if ((float)ret.c[i]*sp.c[i] > std::numeric_limits<T>::max())
						ret.c[i] = std::numeric_limits<T>::max();
					else
						ret.c[i] *= sp.c[i];
				return ret;
			}
			TextureColor<T,colorSamples> &operator*=(const TextureColor<T,colorSamples> &sp) {
				for (int i = 0; i < colorSamples; ++i)
				if ((float)c[i]*sp.c[i] > std::numeric_limits<T>::max())
					c[i] = std::numeric_limits<T>::max();
				else
					c[i] *= sp.c[i];
				return *this;
			}
			TextureColor<T,colorSamples> operator*(float a) const {
				TextureColor<T,colorSamples> ret = *this;
				for (int i = 0; i < colorSamples; ++i)
					if (a*ret.c[i] > std::numeric_limits<T>::max())
						ret.c[i] = std::numeric_limits<T>::max();
					else
						ret.c[i] = (T)(a*ret.c[i]);
				return ret;
			}
			TextureColor<T, colorSamples> &operator*=(float a) {
				for (int i = 0; i < colorSamples; ++i)
					if (a*c[i] > std::numeric_limits<T>::max())
						c[i] = std::numeric_limits<T>::max();
					else
						c[i] *= a;
				return *this;
			}
			friend inline
			TextureColor<T,colorSamples> operator*(float a, const TextureColor<T,colorSamples> &s) {
				return s * a;
			}
			TextureColor<T,colorSamples> operator/(float a) const {
				return *this * (1.f / a);
			}
			TextureColor<T,colorSamples> &operator/=(float a) {
				float inv = 1.f / a;
				for (int i = 0; i < colorSamples; ++i)
					c[i] *= inv;
				return *this;
			}
			void AddWeighted(float w, const TextureColor<T,colorSamples> &s) {
				for (int i = 0; i < colorSamples; ++i)
					c[i] += (T)(w * s.c[i]);
			}
			bool operator==(const TextureColor<T,colorSamples> &sp) const {
				for (int i = 0; i < colorSamples; ++i)
					if (c[i] != sp.c[i]) return false;
				return true;
			}
			bool operator!=(const TextureColor<T,colorSamples> &sp) const {
				return !(*this == sp);
			}
			TextureColor<T,colorSamples> operator-() const {
				TextureColor<T,colorSamples> ret;
				for (int i = 0; i < colorSamples; ++i)
					ret.c[i] = -c[i];
				return ret;
			}
			TextureColor<T,colorSamples> Clamp(float low = 0.f,
						   float high = INFINITY) const {
				TextureColor<T,colorSamples> ret;
				for (int i = 0; i < colorSamples; ++i)
					ret.c[i] = (T)::Clamp((float)c[i], low, high);
				return ret;
			}
			// Color Public Data
			T c[colorSamples];
		};
}
#endif // LUX_COLORBASE_H
