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

/*
   g++ -g -Wall exravg.cpp -I../../src/tangled_code/OpenEXR/include -ltiff -L../../src/tangled_code/OpenEXR/lib-linux -lIlmImf -lImath -lIex -lHalf  

 */

#include <stdio.h>
#include <stdlib.h>
#include <tiffio.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <half.h>

using namespace Imf;
using namespace Imath;

static bool ReadEXR(const char *name, float *&rgba, int &xRes, int &yRes, bool &hasAlpha);

int main(int argc, char *argv[]) 
{
	if (argc < 2) {
		fprintf(stderr, "usage: exravg [file1.exr] <file2.exr> ...\n");
		return 1;
	}

	float *rgba, *orig_rgba;
	int xRes, yRes;
	bool hasAlpha;
	float a = 0;
	int file;

	for (file = 1 ; file < argc ; file++)
	{
		if (ReadEXR(argv[file], rgba, xRes, yRes, hasAlpha)) {
			orig_rgba = rgba;
			a = 0;
			for (int i = 0; i < xRes*yRes; ++i) {
				for (int j = 0; j < 3; ++j)
					a += rgba[j];
				rgba += hasAlpha ? 4 : 3;
			}
		}
		printf("%s: Average value %f\n", argv[file], a / (3.f * xRes * yRes));
		delete [] orig_rgba;
	}
	return 0;
}

static bool ReadEXR(const char *name, float *&rgba, int &xRes, int &yRes, bool &hasAlpha)
{
	InputFile file(name);
	Box2i dw = file.header().dataWindow();
	xRes = dw.max.x - dw.min.x + 1;
	yRes = dw.max.y - dw.min.y + 1;

	half *hrgba = new half[4 * xRes * yRes];

	// for now...
	hasAlpha = true;
	int nChannels = 4;

	FrameBuffer frameBuffer;
	frameBuffer.insert("R", Slice(HALF, (char *)hrgba,
				4*sizeof(half), xRes * 4 * sizeof(half), 1, 1, 0.0));
	frameBuffer.insert("G", Slice(HALF, (char *)hrgba+sizeof(half),
				4*sizeof(half), xRes * 4 * sizeof(half), 1, 1, 0.0));
	frameBuffer.insert("B", Slice(HALF, (char *)hrgba+2*sizeof(half),
				4*sizeof(half), xRes * 4 * sizeof(half), 1, 1, 0.0));
	frameBuffer.insert("A", Slice(HALF, (char *)hrgba+3*sizeof(half),
				4*sizeof(half), xRes * 4 * sizeof(half), 1, 1, 1.0));

	file.setFrameBuffer(frameBuffer);
	file.readPixels(dw.min.y, dw.max.y);

	rgba = new float[nChannels * xRes * yRes];
	for (int i = 0; i < nChannels * xRes * yRes; ++i)
		rgba[i] = hrgba[i];
	delete[] hrgba;

	return rgba;
}
