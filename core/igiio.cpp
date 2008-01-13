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

// igiio.cpp*
#include <algorithm>
#include <fstream>

#include "lux.h"
#include "error.h"
#include "color.h"
#include "spectrum.h"

#include "igiio.h"

// Indigo Image File Format IO - Based on code from GPL Violet Tonemapper.

// IGI Function Definitions
 Spectrum *ReadIgiImage(const string &name, int *width, int *height) {
	// radiance - NOTE - unimplemented yet
		printf("IGI file format input not implemented yet");
		return NULL;
 }

 void WriteIgiImage(const string &name, float *pixels,
		float *alpha, int xRes, int yRes,
		int totalXRes, int totalYRes,
		int xOffset, int yOffset) {

    IndigoImageHeader header;

	// create XYZ float buffer
    u_int xyzSize = xRes * yRes * 3;
	float *xyz = new float[xyzSize];
	for (u_int i = 0; i < xyzSize; i+=3) {
		// convert RGB values to XYZ colour space.
		xyz[i] = 0.436052025f*pixels[i] + 0.385081593f*pixels[i+1] + 0.143087414f *pixels[i+2];
		xyz[i+1] = 0.222491598f*pixels[i] + 0.71688606f *pixels[i+1] + 0.060621486f *pixels[i+2];
		xyz[i+2] = 0.013929122f*pixels[i] + 0.097097002f*pixels[i+1] + 0.71418547f  *pixels[i+2];
    }

	std::ofstream file(name.c_str(), std::ios::binary);
	if(!file) {
		std::stringstream ss;
	 	ss<< "Cannot open file '"<<name<<"' for output";
		luxError(LUX_SYSTEM, LUX_SEVERE, ss.str().c_str());
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
	header.image_data_size = xyzSize * 4;

	// write header
	file.write((const char*)&header, sizeof(header));
	// write xyz data
	file.write((const char*)&xyz[0], header.image_data_size);

	if(!file.good()) {
		std::stringstream ss;
	 	ss<< "Error writing IGI output file '"<<name<<"'";
		luxError(LUX_SYSTEM, LUX_SEVERE, ss.str().c_str());
		return;
	}

	file.close();
	delete xyz;
}
