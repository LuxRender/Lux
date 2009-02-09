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

// color.cpp*
#include "color.h"

using namespace lux;

namespace lux
{

static float determinent(const float matrix[3][3])
{
	float temp;
	temp  = matrix[0][0] * matrix[1][1] * matrix[2][2];
	temp -= matrix[0][0] * matrix[1][2] * matrix[2][1];
	temp -= matrix[1][0] * matrix[0][1] * matrix[2][2];
	temp += matrix[1][0] * matrix[0][2] * matrix[2][1];
	temp += matrix[2][0] * matrix[0][1] * matrix[1][2];
	temp -= matrix[2][0] * matrix[0][2] * matrix[1][1];
	return temp;
}
static void inverse(const float matrix[3][3], float result[3][3])
{
	float det = determinent(matrix);
	if (det == 0.f)
		return;
	float a = matrix[0][0], b = matrix[0][1], c = matrix[0][2],
		d = matrix[1][0], e = matrix[1][1], f = matrix[1][2],
		g = matrix[2][0], h = matrix[2][1], i = matrix[2][2];

	result[0][0] = (e * i - f * h) / det;
	result[0][1] = (c * h - b * i) / det;
	result[0][2] = (b * f - c * e) / det;
	result[1][0] = (f * g - d * i) / det;
	result[1][1] = (a * i - c * g) / det;
	result[1][2] = (c * d - a * f) / det;
	result[2][0] = (d * h - e * g) / det;
	result[2][1] = (b * g - a * h) / det;
	result[2][2] = (a * e - b * d) / det;
}
static void multiply(const float matrix[3][3], const float vector[3], float result[3])
{
	result[0] = matrix[0][0] * vector[0] + matrix[0][1] * vector[1] + matrix[0][2] * vector[2];
	result[1] = matrix[1][0] * vector[0] + matrix[1][1] * vector[1] + matrix[1][2] * vector[2];
	result[2] = matrix[2][0] * vector[0] + matrix[2][1] * vector[1] + matrix[2][2] * vector[2];
}
static void multiply(const float a[3][3], const float b[3][3], float result[3][3])
{
	result[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0];
	result[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1];
	result[0][2] = a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2];
	result[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0];
	result[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1];
	result[1][2] = a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2];
	result[2][0] = a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0];
	result[2][1] = a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1];
	result[2][2] = a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2];
}
static float dot(const float a[3], const float b[3])
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

//!
//! \param[in] xR x value of red in xyY space
//! \param[in] yR y value of red in xyY space
//! \param[in] xG x value of green in xyY space
//! \param[in] yG y value of green in xyY space
//! \param[in] xB x value of blue in xyY space
//! \param[in] yB y value of blue in xyY space
//! \param[in] xW x value of white in xyY space
//! \param[in] yW y value of white in xyY space
//! \param[in] lum Y (intensity) value of white in xyY space
//!
//! Initialize a colorspace conversion instance by providing reference values
//! of red, green, blue and white point.
//! This functions computes the corresponding convertion matrix from XYZ
//! space to RGB space.
//!
ColorSystem::ColorSystem(float xR, float yR, float xG, float yG, float xB, float yB,
	float xW, float yW, float lum)
	: xRed(xR), yRed(yR), xGreen(xG), yGreen(yG), xBlue(xB), yBlue(yB),
	xWhite(xW), yWhite(yW), luminance(lum)
{
	float red[3] = {xRed / yRed, 1.f, (1.f - xRed - yRed) / yRed};
	float green[3] = {xGreen / yGreen, 1.f, (1.f - xGreen - yGreen) / yGreen};
	float blue[3] = {xBlue / yBlue, 1.f, (1.f - xBlue - yBlue) / yBlue};
	float white[3] = {xWhite / yWhite, 1.f, (1.f - xWhite - yWhite) / yWhite};
	float rgb[3][3];
	rgb[0][0] = red[0]; rgb[1][0] = red[1]; rgb[2][0] = red[2];
	rgb[0][1] = green[0]; rgb[1][1] = green[1]; rgb[2][1] = green[2];
	rgb[0][2] = blue[0]; rgb[1][2] = blue[1]; rgb[2][2] = blue[2];
	inverse(rgb, rgb);
	float y[3];
	multiply(rgb, white, y);
	float x[3] = {y[0] * red[0], y[1] * green[0], y[2] * blue[0]};
	float z[3] = {y[0] * red[2], y[1] * green[2], y[2] * blue[2]};
	rgb[0][0] = x[0] + white[0]; rgb[1][0] = x[1] + white[0]; rgb[2][0] = x[2] + white[0];
	rgb[0][1] = y[0] + white[1]; rgb[1][1] = y[1] + white[1]; rgb[2][1] = y[2] + white[1];
	rgb[0][2] = z[0] + white[2]; rgb[1][2] = z[1] + white[2]; rgb[2][2] = z[2] + white[2];
	float matrix[3][3];
	matrix[0][0] = (dot(x, x) + white[0] * white[0]) * luminance;
	matrix[1][0] = (dot(x, y) + white[1] * white[0]) * luminance;
	matrix[2][0] = (dot(x, z) + white[2] * white[0]) * luminance;
	matrix[0][1] = (dot(y, x) + white[0] * white[1]) * luminance;
	matrix[1][1] = (dot(y, y) + white[1] * white[1]) * luminance;
	matrix[2][1] = (dot(y, z) + white[2] * white[1]) * luminance;
	matrix[0][2] = (dot(z, x) + white[0] * white[2]) * luminance;
	matrix[1][2] = (dot(z, y) + white[1] * white[2]) * luminance;
	matrix[2][2] = (dot(z, z) + white[2] * white[2]) * luminance;
	inverse(matrix, matrix);
	//C=R*Tt*(T*Tt)^-1
	multiply(rgb, matrix, XYZToRGB);
	inverse(XYZToRGB, RGBToXYZ);
}

//!
//! \param[in] color  A RGB color possibly unrepresentable
//!
//! Test whether a requested colour is within the gamut achievable with
//! the primaries of the current colour system. This amounts simply to
//! testing whether all the primary weights are non-negative.
//!
static inline bool LowGamut(const RGBColor &color)
{
    return color.c[0] < 0.f || color.c[1] < 0.f || color.c[2] < 0.f;
}

//!
//! \param[in] color  A RGB color possibly unrepresentable
//!
//! Test whether a requested colour is within the gamut achievable with
//! the primaries of the current colour system. This amounts simply to
//! testing whether all the primary weights are at most 1.
//!
static inline bool HighGamut(const RGBColor &color)
{
    return color.c[0] > 1.f || color.c[1] > 1.f || color.c[2] > 1.f;
}

//!
//! \param[in] xyz The color in XYZ space
//! \param[in,out] rgb The same color in RGB space
//! \return Whether the RGB representation was modified or not
//! \retval true The color has been modified
//! \retval false The color was inside the representable gamut:
//! no modification occured
//!
//! If the requested RGB shade contains a negative weight for one of
//! the primaries, it lies outside the colour gamut accessible from the
//! given triple of primaries. Desaturate it by mixing with the white
//! point of the colour system so as to reduce the primary with the
//! negative weight to zero. This is equivalent to finding the
//! intersection on the CIE diagram of a line drawn between the white
//! point and the requested colour with the edge of the Maxwell
//! triangle formed by the three primaries.
//! This function tries not to change the overall intensity, only the tint
//! is shifted to be inside the representable gamut.
//!
bool ColorSystem::Constrain(float lum, RGBColor &rgb) const
{
	bool constrain = false;
	// Is the contribution of one of the primaries negative ?
	if (LowGamut(rgb)) {
		if (lum < 0.f) {
			rgb = 0.f;
			return true;
		}

		// Find the primary with most negative weight and calculate the
		// parameter of the point on the vector from the white point
		// to the original requested colour in RGB space.
		float l = lum / luminance;
		float parameter;
		if (rgb.c[0] < rgb.c[1] && rgb.c[0] < rgb.c[2]) {
			parameter = l / (l - rgb.c[0]);
		} else if (rgb.c[1] < rgb.c[2]) {
			parameter = l / (l - rgb.c[1]);
		} else {
			parameter = l / (l - rgb.c[2]);
		}

		// Now finally compute the gamut-constrained RGB weights.
		rgb = Lerp(parameter, RGBColor(l), rgb);
		constrain = true;	// Colour modified to fit RGB gamut
	}
	// The following is disabled to better preserve color instead of luminance
	// The is most noticeable with HDR outputs that don't need this clipping at all
/*	if (HighGamut(rgb)) {
		if (lum > luminance) {
			rgb = RGBColor(1.f);
			return true;
		}

		// Find the primary with greater weight and calculate the
		// parameter of the point on the vector from the white point
		// to the original requested colour in RGB space.
		float l = lum / luminance;
		float parameter;
		if (rgb.c[0] > rgb.c[1] && rgb.c[0] > rgb.c[2]) {
			parameter = (1.f - l) / (rgb.c[0] - l);
		} else if (rgb.c[1] > rgb.c[2]) {
			parameter = (1.f - l) / (rgb.c[1] - l);
		} else {
			parameter = (1.f - l) / (rgb.c[2] - l);
		}

		// Now finally compute the gamut-constrained RGB weights.
		rgb = Lerp(parameter, RGBColor(l), rgb);
		constrain = true;	// Colour modified to fit RGB gamut
	}*/

	return constrain;
}

}

