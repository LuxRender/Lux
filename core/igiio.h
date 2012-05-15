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

#ifndef LUX_IGIIO_H
#define LUX_IGIIO_H
// igiio.h*

#include "lux.h"

namespace lux
{

// Based on code from GPL Violet Tonemapper.
static const int INDIGO_IMAGE_MAGIC_NUMBER = 66613373;

class IndigoImageHeader
{
public:
	IndigoImageHeader() {};
	~IndigoImageHeader() {};

	//all integers and unsigned integers are 32 bit
	//Byte order should be little endian (Intel byte order)
	int magic_number;//should be 66613373
	int format_version;//1
	double num_samples;//total num samples taken over entire image
	unsigned int width;//width of supersampled (large) image
	unsigned int height;//height of supersampled (large) image
	unsigned int supersample_factor;// >= 1
	int zipped;//boolean

	int image_data_size;//size of image data in bytes
	//should be equal to width*height*12, if data is uncompressed.

	unsigned int colour_space;//0 = XYZ
	unsigned char padding[5000];//padding in case i want more stuff in the header in future

	//image data follows:
	//top row, then next-to-top row, etc...
	//left to right across the row.
	//3 32 bit floats per pixel.
};

void WriteIgiImage(const string &name,
	vector<RGBColor> &pixels, vector<float> &alpha, u_int XRes, u_int YRes,
	u_int totalXRes, u_int totalYRes, u_int xOffset, u_int yOffset);

}

#endif // LUX_IGIIO_H

