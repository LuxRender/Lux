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
 * exrtoiff.cpp
 *
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <tiffio.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <half.h>

#include "pbrt.h"
#include "film.h"
#include "paramset.h"

using namespace Imf;
using namespace Imath;

static bool ReadEXR(const char *name, float *&rgba, int &xRes, int &yRes, bool &hasAlpha);
static void WriteTIFF(const char *name, float *rgba, int xRes, int yRes, bool hasAlpha);

static void usage() {
  fprintf( stderr, "usage: exrtotiff [options] <input.exr> <output.tiff>\n" );
  fprintf( stderr, "Supported options:\n");
  fprintf( stderr, "\t-scale scale\n" );
  fprintf( stderr, "\t-gamma gamma\n" );
  fprintf( stderr, "\t-tonemap mapname\n" );
  fprintf( stderr, "\t-param name value (set tone map parameter)\n" );
  fprintf( stderr, "\t-bloomRadius radius\n" );
  fprintf( stderr, "\t-bloomWeight weight\n" );
  fprintf( stderr, "\t-bg bg-grey color\n");
  exit(1);
}

int main(int argc, char *argv[]) 
{
    float scale = 1.f, gamma = 2.2f;
    float bloomRadius = 0.f, bloomWeight = .2f;
    float bggray = -1.f;
    char *toneMap = NULL;
    ParamSet toneMapParams;

    int argNum = 1;
    while (argNum < argc && argv[argNum][0] == '-') {
#define ARG(name, var) \
      else if (!strcmp(argv[argNum], "-" name)) { \
	if (argNum+1 == argc) \
	  usage(); \
	var = atof(argv[argNum+1]); \
	++argNum; \
      }

	if (!strcmp(argv[argNum], "-tonemap")) {
	    if (argNum+1 == argc)
		usage();
	    toneMap = argv[argNum+1];
	    ++argNum;
	}
	else if (!strcmp(argv[argNum], "-param")) {
	    if (argNum+2 >= argc)
		usage();
	    float val = atof(argv[argNum+2]);
	    toneMapParams.AddFloat(argv[argNum+1], &val);
	    argNum += 2;
	}
	ARG("scale", scale)
	ARG("gamma", gamma)
	ARG("bg", bggray)
	ARG("bloomRadius", bloomRadius)
	ARG("bloomWeight", bloomWeight)
	else 
	    usage();
      ++argNum;

    }
    if (argNum + 2 > argc) usage();

    char *inFile = argv[argNum], *outFile = argv[argNum+1]; 	  
    float *rgba;
    int xRes, yRes;
    bool hasAlpha;
    pbrtInit();

    if (ReadEXR(inFile, rgba, xRes, yRes, hasAlpha)) {
	float *rgb = new float[xRes*yRes*3];
	for (int i = 0; i < xRes*yRes; ++i) {
	    for (int j = 0; j < 3; ++j) {
		rgb[3*i+j] = scale * rgba[4*i+j];
//CO		if (rgba[4*i+3] != 0.f)
//CO		    rgb[3*i+j] /= rgba[4*i+3];
		if (bggray > 0)
		    rgb[3*i+j] = rgba[4*i+3] * rgb[3*i+j] + (1.f - rgba[4*i+3]) * bggray;
	    }
	    if (bggray > 0)
		rgba[4*i+3] = 1.f;
	}

	ApplyImagingPipeline(rgb, xRes, yRes, NULL, bloomRadius,
			     bloomWeight, toneMap, &toneMapParams,
			     gamma, 0.f, 255);

	for (int i = 0; i < xRes*yRes; ++i) {
	    for (int j = 0; j < 3; ++j) {
		rgba[4*i+j] = rgb[3*i+j];
		if (rgba[4*i+3] != 0.f)
		    rgba[4*i+j] /= rgba[4*i+3];
	    }
	    rgba[4*i+3] *= 255.f;
	}

	WriteTIFF(outFile, rgba, xRes, yRes, hasAlpha);
    }
    pbrtCleanup();
    return 0;
}

void WriteTIFF(const char *name, float *rgba, int XRes, int YRes, bool hasAlpha) 
{
    // Open 8-bit TIFF file for writing
    TIFF *tiff = TIFFOpen(name, "w");
    if (!tiff) {
	fprintf(stderr, "Unable to open TIFF %s for writing", name);
	return;
    }

    int nChannels = hasAlpha ? 4 : 3;
    TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, nChannels);
    if (hasAlpha) {
	short int extra[] = { EXTRASAMPLE_ASSOCALPHA };
	TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, (short)1, extra);
    }
    // Write image resolution information
    TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, XRes);
    TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, YRes);
    TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    // Set Generic TIFF Fields
    TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, 1);
    TIFFSetField(tiff, TIFFTAG_XRESOLUTION, 1.f);
    TIFFSetField(tiff, TIFFTAG_YRESOLUTION, 1.f);
    TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, (short)1);
    TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tiff, TIFFTAG_ORIENTATION, (int)ORIENTATION_TOPLEFT);
    // Write 8-bit scanlines
    u_char *buf = new u_char[nChannels * XRes];
    for (int y = 0; y < YRes; ++y) {
	u_char *bufp = buf;
	for (int x = 0; x < XRes; ++x) {
	    // Pack 8-bit pixels samples into buf
	    for (int s = 0; s < nChannels; ++s)
		*bufp++ = (u_char)*rgba++;
	}
	TIFFWriteScanline(tiff, buf, y, 1);
    }
    // Close 8-bit TIFF file
    delete[] buf;
    TIFFClose(tiff);
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

    half *hp = hrgba - nChannels * (dw.min.x + dw.min.y * xRes);

    FrameBuffer frameBuffer;
    frameBuffer.insert("R", Slice(HALF, (char *)hp,
				  4*sizeof(half), xRes * 4 * sizeof(half), 1, 1, 0.0));
    frameBuffer.insert("G", Slice(HALF, (char *)hp+sizeof(half),
				  4*sizeof(half), xRes * 4 * sizeof(half), 1, 1, 0.0));
    frameBuffer.insert("B", Slice(HALF, (char *)hp+2*sizeof(half),
				  4*sizeof(half), xRes * 4 * sizeof(half), 1, 1, 0.0));
    frameBuffer.insert("A", Slice(HALF, (char *)hp+3*sizeof(half),
				  4*sizeof(half), xRes * 4 * sizeof(half), 1, 1, 1.0));

    file.setFrameBuffer(frameBuffer);
    file.readPixels(dw.min.y, dw.max.y);

    rgba = new float[nChannels * xRes * yRes];
    for (int i = 0; i < nChannels * xRes * yRes; ++i)
	rgba[i] = hrgba[i];
    delete[] hrgba;

    return rgba;
}
