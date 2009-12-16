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
        vector<float> &alpha, u_int xPixelCount, u_int yPixelCount,
        u_int xResolution, u_int yResolution,
        u_int xPixelStart, u_int yPixelStart) {
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

	// No cropping data: use directly the cropped size
	header[13] = static_cast<char>((xPixelCount >> 8) & 0xFF);
	header[12] = static_cast<char>(xPixelCount & 0xFF);
	header[15] = static_cast<char>((yPixelCount >> 8) & 0xFF);
	header[14] = static_cast<char>(yPixelCount & 0xFF);
	if(channeltype == 0)
		header[16] = 8;						// bitdepth for BW
	else if(channeltype == 2)
		header[16] = 24;						// bitdepth for RGB
	else
		header[16] = 32;						// bitdepth for RGBA

	// put the header data into the file
	for (u_int i = 0; i < 18; ++i)
		fputc(header[i], tgaFile);

	// write the bytes of data out
	for (u_int i = 0;  i < yPixelCount ; ++i) {
		const u_int line = yPixelCount - i - 1;
		for (u_int j = 0; j < xPixelCount; ++j) {
			const u_int offset = line * xPixelCount + j;
			if (channeltype == 0) {
				// BW
				float c = pixels[offset].Y();
				fputc(static_cast<unsigned char>(Clamp(256.f * c, 0.f, 255.f)), tgaFile);
			} else {
				// RGB(A)
				fputc(static_cast<unsigned char>(Clamp(256.f * pixels[offset].c[2], 0.f, 255.f)), tgaFile);
				fputc(static_cast<unsigned char>(Clamp(256.f * pixels[offset].c[1], 0.f, 255.f)), tgaFile);
				fputc(static_cast<unsigned char>(Clamp(256.f * pixels[offset].c[0], 0.f, 255.f)), tgaFile);
				if(channeltype == 3) //  Alpha
					fputc(static_cast<unsigned char>(Clamp(256.f * alpha[offset], 0.f, 255.f)), tgaFile);
			}
		}
	}

	fclose(tgaFile);
}

}


