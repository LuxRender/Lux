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

// tgaio.cpp*
#include "tgaio.h"
#include "error.h"
#include "color.h"

using namespace lux;

namespace lux
{

void WriteTargaImage(int channeltype, bool savezbuf, const string &name, vector<RGBColor> &pixels,
        vector<float> &alpha, int xPixelCount, int yPixelCount,
        int xResolution, int yResolution,
        int xPixelStart, int yPixelStart) {
	// Open file
	FILE* tgaFile = fopen(name.c_str(),"wb");
	if (!tgaFile) {
		std::stringstream ss;
		ss<< "Cannot open file '"<<name<<"' for output";
		luxError(LUX_SYSTEM, LUX_SEVERE, ss.str().c_str());
		return;
	}

	// write the header
	// make sure its platform independent of little endian and big endian
	char header[18];
	memset(header, 0, sizeof(char) * 18);

	if(channeltype > 0)
		header[2] = 2;							// set the data type of the targa (2 = RGB uncompressed)
	else
		header[2] = 3;							// set the data type of the targa (3 = GREYSCALE uncompressed)

	short xResShort = xPixelCount;			// set the resolution and make sure the bytes are in the right order
	header[13] = (char) (xResShort >> 8);
	header[12] = xResShort & 0xFF;
	short yResShort = yPixelCount;
	header[15] = (char) (yResShort >> 8);
	header[14] = yResShort & 0xFF;
	if(channeltype == 0)
		header[16] = 8;						// bitdepth for BW
	else if(channeltype == 1)
		header[16] = 24;						// bitdepth for RGB
	else
		header[16] = 32;						// bitdepth for RGBA

	// put the header data into the file
	for (int i = 0; i < 18; ++i)
		fputc(header[i], tgaFile);

	// write the bytes of data out
	for (int i = yPixelCount - 1;  i >= 0 ; --i) {
		for (int j = 0; j < xPixelCount; ++j) {
			if(channeltype == 0) {
				// BW
				float c = (0.3f * pixels[i * xPixelCount + j].c[0]) + 
					(0.59f * pixels[i * xPixelCount + j].c[1]) + 
					(0.11f * pixels[i * xPixelCount + j].c[2]);
				fputc(static_cast<unsigned char>(Clamp(256 * c, 0.f, 255.f)), tgaFile);
			} else {
				// RGB(A)
				fputc(static_cast<unsigned char>(Clamp(256 * pixels[i * xPixelCount + j].c[2], 0.f, 255.f)), tgaFile);
				fputc(static_cast<unsigned char>(Clamp(256 * pixels[i * xPixelCount + j].c[1], 0.f, 255.f)), tgaFile);
				fputc(static_cast<unsigned char>(Clamp(256 * pixels[i * xPixelCount + j].c[0], 0.f, 255.f)), tgaFile);
				if(channeltype == 2) //  Alpha
					fputc(static_cast<unsigned char>(Clamp(256 * alpha[(i * xPixelCount + j)], 0.f, 255.f)), tgaFile);
			}
		}
	}

	fclose(tgaFile);
}

}


