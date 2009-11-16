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

#ifndef LUX_COLOR_H
#define LUX_COLOR_H
// color.h*
#include "lux.h"

#include <boost/serialization/access.hpp>

namespace lux
{

// Color Declarations
class  Color {
    // Dade - serialization here is required by network rendering
	friend class boost::serialization::access;

public:
	// Color Public Methods
	Color() {};

	// !!! TODO !!! - Radiance - rewrite these to loops so icc vectorizer can vectorize them.

	friend ostream &operator<<(ostream &, const Color &);
	Color &operator+=(const Color &s2) {
			c[0] += s2.c[0]; c[1] += s2.c[1]; c[2] += s2.c[2];
		return *this;
	}
	Color &operator+=(float f) {
			c[0] += f; c[1] += f; c[2] += f;
		return *this;
	}
	Color operator+(const Color &s2) const {
		Color ret = *this;
			ret.c[0] += s2.c[0]; ret.c[1] += s2.c[1]; ret.c[2] += s2.c[2];
		return ret;
	}
	Color &operator-=(const Color &s2) {
			c[0] -= s2.c[0]; c[1] -= s2.c[1]; c[2] -= s2.c[2];
		return *this;
	}
	Color &operator-=(float f) {
			c[0] -= f; c[1] -= f; c[2] -= f;
		return *this;
	}
	Color operator-(const Color &s2) const {
		Color ret = *this;
			ret.c[0] -= s2.c[0]; ret.c[1] -= s2.c[1]; ret.c[2] -= s2.c[2];
		return ret;
	}
	Color operator/(const Color &s2) const {
		Color ret = *this;
			ret.c[0] /= s2.c[0]; ret.c[1] /= s2.c[1]; ret.c[2] /= s2.c[2];
		return ret;
	}
	Color operator*(const Color &sp) const {
		Color ret = *this;
			ret.c[0] *= sp.c[0]; ret.c[1] *= sp.c[1]; ret.c[2] *= sp.c[2];
		return ret;
	}
	Color &operator*=(const Color &sp) {
			c[0] *= sp.c[0]; c[1] *= sp.c[1]; c[2] *= sp.c[2];
		return *this;
	}
	Color operator*(float a) const {
		Color ret = *this;
			ret.c[0] *= a; ret.c[1] *= a; ret.c[2] *= a;
		return ret;
	}
	Color &operator*=(float a) {
			c[0] *= a; c[1] *= a; c[2] *= a;
		return *this;
	}
	friend inline
	Color operator*(float a, const Color &s) {
		return s * a;
	}
	Color operator/(float a) const {
		return *this * (1.f / a);
	}
	Color &operator/=(float a) {
		float inv = 1.f / a;
			c[0] *= inv; c[1] *= inv; c[2] *= inv;
		return *this;
	}
	void AddWeighted(float w, const Color &s) {
			c[0] += w * s.c[0]; c[1] += w * s.c[1]; c[2] += w * s.c[2];
	}
	bool operator==(const Color &sp) const {
			if (c[0] != sp.c[0]) return false;
			if (c[1] != sp.c[1]) return false;
			if (c[2] != sp.c[2]) return false;
		return true;
	}
	bool operator!=(const Color &sp) const {
		return !(*this == sp);
	}
	bool Black() const {
			if (c[0] != 0.) return false;
			if (c[1] != 0.) return false;
			if (c[2] != 0.) return false;
		return true;
	}
	Color Sqrt() const {
		Color ret;
			ret.c[0] = sqrtf(c[0]);
			ret.c[1] = sqrtf(c[1]);
			ret.c[2] = sqrtf(c[2]);
		return ret;
	}
	Color Pow(const Color &e) const {
		Color ret;
			ret.c[0] = c[0] > 0 ? powf(c[0], e.c[0]) : 0.f;
			ret.c[1] = c[1] > 0 ? powf(c[1], e.c[1]) : 0.f;
			ret.c[2] = c[2] > 0 ? powf(c[2], e.c[2]) : 0.f;
		return ret;
	}
	Color Pow(float f) const {
		Color ret;
		ret.c[0] = c[0] > 0 ? powf(c[0], f) : 0.f;
		ret.c[1] = c[1] > 0 ? powf(c[1], f) : 0.f;
		ret.c[2] = c[2] > 0 ? powf(c[2], f) : 0.f;
		return ret;
	}
	Color operator-() const {
		Color ret;
			ret.c[0] = -c[0];
			ret.c[1] = -c[1];
			ret.c[2] = -c[2];
		return ret;
	}
	friend Color Exp(const Color &s) {
		Color ret;
			ret.c[0] = expf(s.c[0]);
			ret.c[1] = expf(s.c[1]);
			ret.c[2] = expf(s.c[2]);
		return ret;
	}
	Color Clamp(float low = 0.f,
	               float high = INFINITY) const {
		Color ret;
			ret.c[0] = ::Clamp(c[0], low, high);
			ret.c[1] = ::Clamp(c[1], low, high);
			ret.c[2] = ::Clamp(c[2], low, high);
		return ret;
	}
	bool IsNaN() const {
			if (isnan(c[0])) return true;
			if (isnan(c[1])) return true;
			if (isnan(c[2])) return true;
		return false;
	}
	void Print(FILE *f) const {
		for (int i = 0; i < 3; ++i)
			fprintf(f, "%f ", c[i]);
	}

	friend class lux::ParamSet;
	
	// Color Public Data
	float c[3];
	
private:
	template<class Archive>
			void serialize(Archive & ar, const unsigned int version)
			{
				for (int i = 0; i < 3; ++i)
					ar & c[i];
			}
};

// RGBColor Declarations
class  RGBColor : public Color {
public:
	// RGBColor Public Methods
	RGBColor(float v = 0.f) {
			c[0] = v; c[1] = v; c[2] = v;
	}
	RGBColor(const float cs[3]) {
			c[0] = cs[0]; c[1] = cs[1]; c[2] = cs[2];
	}
	RGBColor(const Color &color) { // so that operators work
		c[0] = color.c[0]; c[1] = color.c[1]; c[2] = color.c[2];
	}

	float Y() const {
		return 0.212671f * c[0] + 0.715160f * c[1] + 0.072169f * c[2];
	}
	float Filter() const { return (c[0] + c[1] + c[2]) / 3.f; }
};

// XYZColor Declarations
class  XYZColor : public Color {
    // Dade - serialization here is required by network rendering
    friend class boost::serialization::access;

public:
	// XYZColor Public Methods
	XYZColor(float v = 0.f) {
			c[0] = v; c[1] = v; c[2] = v;
	}
	XYZColor(float cs[3]) {
			c[0] = cs[0]; c[1] = cs[1]; c[2] = cs[2];
	}
	XYZColor(const Color &color) { // so that operators work
		c[0] = color.c[0]; c[1] = color.c[1]; c[2] = color.c[2];
	}

	float Y() const {
		return c[1];
	}
};

//!
//! A colour system is defined by the CIE x and y coordinates of its
//! three primary illuminants and the x and y coordinates of the white
//! point.
//! The additional definition of the white point intensity allow for
//! intensity adaptation
//!
class ColorSystem
{
public:
	ColorSystem(float xR, float yR, float xG, float yG, float xB, float yB,
		float xW, float yW, float lum = 1.);
//!
//! \param[in] color A color in XYZ space
//! \return The color converted in RGB space
//!
//! Determine the contribution of each primary in a linear combination
//! which sums to the desired chromaticity. If the requested
//! chromaticity falls outside the Maxwell triangle (colour gamut) formed
//! by the three primaries, one of the R, G, or B weights will be
//! negative. Use Constrain() to desaturate an outside-gamut colour to
//! the closest representation within the available gamut.
//! \sa Constrain
//!
	RGBColor ToRGBConstrained(const XYZColor &color) const {
		const float lum = color.Y();
		float c[3];
		c[0] = XYZToRGB[0][0] * color.c[0] + XYZToRGB[0][1] * color.c[1] + XYZToRGB[0][2] * color.c[2];
		c[1] = XYZToRGB[1][0] * color.c[0] + XYZToRGB[1][1] * color.c[1] + XYZToRGB[1][2] * color.c[2];
		c[2] = XYZToRGB[2][0] * color.c[0] + XYZToRGB[2][1] * color.c[1] + XYZToRGB[2][2] * color.c[2];
		RGBColor rgb(c);
		Constrain(lum, rgb);
		return rgb;
	}
//!
//! \param[in] color A color in RGB space
//! \return The color converted in XYZ space
//!
	XYZColor ToXYZ(const RGBColor &color) const {
		float c[3];
		c[0] = RGBToXYZ[0][0] * color.c[0] + RGBToXYZ[0][1] * color.c[1] + RGBToXYZ[0][2] * color.c[2];
		c[1] = RGBToXYZ[1][0] * color.c[0] + RGBToXYZ[1][1] * color.c[1] + RGBToXYZ[1][2] * color.c[2];
		c[2] = RGBToXYZ[2][0] * color.c[0] + RGBToXYZ[2][1] * color.c[1] + RGBToXYZ[2][2] * color.c[2];
		return XYZColor(c);
	}
//protected:
	bool Constrain(float lum, RGBColor &rgb) const;
	RGBColor Limit(const RGBColor &rgb, int method) const;
	float xRed, yRed; //!<Red coordinates
	float xGreen, yGreen; //!<Green coordinates
	float xBlue, yBlue; //!<Blue coordinates
	float xWhite, yWhite; //!<White coordinates
	float luminance; //!<White intensity
	float XYZToRGB[3][3]; //!<Corresponding conversion matrix from XYZ to RGB
	float RGBToXYZ[3][3]; //!<Corresponding conversion matrix from RGB to XYZ
};

// RGBColor Method Definitions
inline ostream &operator<<(ostream &os, const RGBColor &s) {
	for (int i = 0; i < 3; ++i) {
		os << s.c[i];
		if (i != 3-1)
			os << ", ";
	}
	return os;
}

// XYZColor Method Definitions
inline ostream &operator<<(ostream &os, const XYZColor &s) {
	for (int i = 0; i < 3; ++i) {
		os << s.c[i];
		if (i != 3-1)
			os << ", ";
	}
	return os;
}

}//namespace lux

#endif // LUX_COLOR_H
