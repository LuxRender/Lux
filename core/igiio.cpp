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

// igiio.cpp*
#include "lux.h"
#include "error.h"
#include "color.h"
#include "spectrum.h"

#include "igiio.h"

#include <algorithm>
#include <fstream>
#include <cstring>
using std::memset;

using namespace lux;
// Indigo Image File Format IO - Based on code from GPL Violet Tonemapper.

namespace lux {
// IGI Function Definitions
RGBColor *ReadIgiImage(const string &name, u_int *width, u_int *height)
{
	// radiance - NOTE - unimplemented yet
		printf("IGI file format input not implemented yet");
		return NULL;
}

void WriteIgiImage(const string &name, vector<RGBColor> &pixels,
	vector<float> &alpha, u_int xRes, u_int yRes,
	u_int totalXRes, u_int totalYRes,
	u_int xOffset, u_int yOffset)
{
	IndigoImageHeader header;

	// create XYZ float buffer
	u_int xyzSize = xRes * yRes;
	float *xyz = new float[3 * xyzSize];
	for (u_int i = 0; i < xyzSize; ++i) {
		// convert RGB values to XYZ colour space.
		xyz[3 * i] = 0.436052025f * pixels[i].c[0] + 0.385081593f * pixels[i].c[1] + 0.143087414f * pixels[i].c[2];
		xyz[3 * i + 1] = 0.222491598f * pixels[i].c[0] + 0.71688606f * pixels[i].c[1] + 0.060621486f * pixels[i].c[2];
		xyz[3 * i + 2] = 0.013929122f * pixels[i].c[0] + 0.097097002f * pixels[i].c[1] + 0.71418547f  * pixels[i].c[2];
	}

	std::ofstream file(name.c_str(), std::ios::binary);
	if (!file) {
		LOG( LUX_SEVERE,LUX_SYSTEM)<< "Cannot open file '"<<name<<"' for output";
		return;
	}

	// set header
	memset(&header, 0, sizeof(header));
	header.magic_number = INDIGO_IMAGE_MAGIC_NUMBER;
	header.format_version = 1;

	header.num_samples = 1.; // TODO add samples from film
	header.width = xRes;
	header.height = yRes;
	header.supersample_factor = 1;
	header.zipped = false;
	header.image_data_size = static_cast<int>(xyzSize * 12);
	header.colour_space = 0;

	// write header
	file.write(reinterpret_cast<char *>(&header), sizeof(header));
	// write xyz data
	file.write(reinterpret_cast<char *>(&xyz[0]), header.image_data_size);

	if (!file.good()) {
		LOG( LUX_SEVERE,LUX_SYSTEM)<< "Error writing IGI output file '"<<name<<"'";
		return;
	}

	file.close();
	delete xyz;
}

}
