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

// color.cpp*
#include "color.h"

// RGBColor Method Definitions
ostream &operator<<(ostream &os, const RGBColor &s) {
	for (int i = 0; i < 3; ++i) {
		os << s.c[i];
		if (i != 3-1)
			os << ", ";
	}
	return os;
}

RGBColor::RGBColor(XYZColor xyz) {
	ToXYZ(xyz.c[0], xyz.c[1], xyz.c[2]);
}

void RGBColor::ToXYZ(XYZColor xyz) const {
	ToXYZ(xyz.c[0], xyz.c[1], xyz.c[2]);
}
XYZColor RGBColor::ToXYZ() const {
	XYZColor xyz;
	ToXYZ(xyz.c[0], xyz.c[1], xyz.c[2]);
	return xyz;
} 

void RGBColor::FromXYZ(XYZColor xyz) {
	FromXYZ(xyz.c[0], xyz.c[1], xyz.c[2]);
} 


// XYZColor Method Definitions
ostream &operator<<(ostream &os, const XYZColor &s) {
	for (int i = 0; i < 3; ++i) {
		os << s.c[i];
		if (i != 3-1)
			os << ", ";
	}
	return os;
}

XYZColor::XYZColor(RGBColor rgb) {
	ToRGB(rgb.c[0], rgb.c[1], rgb.c[2]);
}

void XYZColor::ToRGB(RGBColor rgb) const {
	ToRGB(rgb.c[0], rgb.c[1], rgb.c[2]);
}
RGBColor XYZColor::ToRGB() const {
	RGBColor rgb;
	ToRGB(rgb.c[0], rgb.c[1], rgb.c[2]);
	return rgb;
}

void XYZColor::FromRGB(RGBColor rgb) {
	FromRGB(rgb.c[0], rgb.c[1], rgb.c[2]);
}
