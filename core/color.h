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

#ifndef LUX_COLOR_H
#define LUX_COLOR_H
// color.h*
#include "lux.h"


namespace lux
{

// Color Declarations
class  Color {
    // Dade - serialization here is required by network rendering
	friend class boost::serialization::access;

public:
	// Color Public Methods
	Color() {};

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

	float y() const {
		return 0.;
	}

	bool operator<(const Color &s2) const {
		return y() < s2.y();
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
	RGBColor(float cs[3]) {
			c[0] = cs[0]; c[1] = cs[1]; c[2] = cs[2];
	}
	RGBColor(XYZColor xyz);
	RGBColor(const Color &color) { // so that operators work
		c[0] = color.c[0]; c[1] = color.c[1]; c[2] = color.c[2];
	}

	// XYZ Color output methods
	void ToXYZ(float &x, float &y, float &z) const {
		x = 0.412453f * c[0] + 0.357580f * c[1] + 0.180423f * c[2];
		y = 0.212671f * c[0] + 0.715160f * c[1] + 0.072169f * c[2];
		z = 0.019334f * c[0] + 0.119193f * c[1] + 0.950227f * c[2];
	}
	void ToXYZ(float xyz[3]) const {
		ToXYZ(xyz[0], xyz[1], xyz[2]);
	}
	void ToXYZ(XYZColor xyz) const;
	XYZColor ToXYZ() const;

	// XYZ Color input methods
	void FromXYZ(float x, float y, float z) {
		c[0] =  3.240479f * x + -1.537150f * y + -0.498535f * z;
		c[1] = -0.969256f * x +  1.875991f * y +  0.041556f * z;
		c[2] =  0.055648f * x + -0.204043f * y +  1.057311f * z;
	}
	void FromXYZ(float xyz[3]) {
		FromXYZ(xyz[0], xyz[1], xyz[2]);
	}
	void FromXYZ(XYZColor xyz); 

	float y() const {
		return 0.212671f * c[0] + 0.715160f * c[1] + 0.072169f * c[2];
	}
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
	XYZColor(RGBColor rgb);

	// RGB Color output methods
	void ToRGB(float &r, float &g, float &b) const {
		r =  3.240479f * c[0] + -1.537150f * c[1] + -0.498535f * c[2];
		g = -0.969256f * c[0] +  1.875991f * c[1] +  0.041556f * c[2];
		b =  0.055648f * c[0] + -0.204043f * c[1] +  1.057311f * c[2];
	}
	void ToRGB(float rgb[3]) const {
		ToRGB(rgb[0], rgb[1], rgb[2]);
	}
	void ToRGB(RGBColor rgb) const;
	RGBColor ToRGB() const;

	// RGB Color input methods
	void FromRGB(float r, float g, float b) {
		c[0] = 0.412453f * r + 0.357580f * g + 0.180423f * b;
		c[1] = 0.212671f * r + 0.715160f * g + 0.072169f * b;
		c[2] = 0.019334f * r + 0.119193f * g + 0.950227f * b;
	}
	void FromRGB(float rgb[3]) {
		FromRGB(rgb[0], rgb[1], rgb[2]);
	}
	void FromRGB(RGBColor rgb);

	float y() const {
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
		const float lum = color.y();
		float c[3];
		c[0] = conversion[0][0] * color.c[0] + conversion[0][1] * color.c[1] + conversion[0][2] * color.c[2];
		c[1] = conversion[1][0] * color.c[0] + conversion[1][1] * color.c[1] + conversion[1][2] * color.c[2];
		c[2] = conversion[2][0] * color.c[0] + conversion[2][1] * color.c[1] + conversion[2][2] * color.c[2];
		RGBColor rgb(c);
		Constrain(lum, rgb);
		return rgb;
	}
protected:
	bool Constrain(float lum, RGBColor &rgb) const;
	float xRed, yRed; //!<Red coordinates
	float xGreen, yGreen; //!<Green coordinates
	float xBlue, yBlue; //!<Blue coordinates
	float xWhite, yWhite; //!<White coordinates
	float luminance; //!<White intensity
	float conversion[3][3]; //!<Corresponding conversion matrix from XYZ to RGB
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

inline RGBColor::RGBColor(XYZColor xyz) {
	ToXYZ(xyz.c[0], xyz.c[1], xyz.c[2]);
}

inline void RGBColor::ToXYZ(XYZColor xyz) const {
	ToXYZ(xyz.c[0], xyz.c[1], xyz.c[2]);
}
inline XYZColor RGBColor::ToXYZ() const {
	XYZColor xyz;
	ToXYZ(xyz.c[0], xyz.c[1], xyz.c[2]);
	return xyz;
} 

inline void RGBColor::FromXYZ(XYZColor xyz) {
	FromXYZ(xyz.c[0], xyz.c[1], xyz.c[2]);
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

inline XYZColor::XYZColor(RGBColor rgb) {
	ToRGB(rgb.c[0], rgb.c[1], rgb.c[2]);
}

inline void XYZColor::ToRGB(RGBColor rgb) const {
	ToRGB(rgb.c[0], rgb.c[1], rgb.c[2]);
}
inline RGBColor XYZColor::ToRGB() const {
	RGBColor rgb;
	ToRGB(rgb.c[0], rgb.c[1], rgb.c[2]);
	return rgb;
}

inline void XYZColor::FromRGB(RGBColor rgb) {
	FromRGB(rgb.c[0], rgb.c[1], rgb.c[2]);
}

}//namespace lux




#endif // LUX_COLOR_H
