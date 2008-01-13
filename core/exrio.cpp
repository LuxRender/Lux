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

// exrio.cpp*
//#ifdef __APPLE__
#include "lux.h"
#include "error.h"
#include "color.h"
#include "spectrum.h"
//#endif
#include <algorithm>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>

#define cimg_display_type  0

#ifdef LUX_USE_CONFIG_H
#include "config.h"

#ifdef PNG_FOUND
#define cimg_use_png 1
#endif

#ifdef JPEG_FOUND
#define cimg_use_jpeg 1
#endif

#ifdef TIFF_FOUND
#define cimg_use_tiff 1
#endif


#else //LUX_USE_CONFIG_H
#define cimg_use_png 1
//#define cimg_use_tiff 1
#define cimg_use_jpeg 1
#endif //LUX_USE_CONFIG_H


#define cimg_debug 0     // Disable modal window in CImg exceptions.
#include "cimg.h"
using namespace cimg_library;

#ifdef WIN32
#define hypotf hypot // For the OpenEXR headers
#endif

#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <half.h>
/*#ifndef __APPLE__
#include "lux.h"
#include "error.h"
#include "color.h"
#include "spectrum.h"
#endif*/
using namespace Imf;
using namespace Imath;
// EXR Function Definitions
 Spectrum *ReadExrImage(const string &name, int *width, int *height) {
	try {
    printf("Loading OpenEXR Texture: '%s'...\n", name.c_str());
	InputFile file(name.c_str());
	Box2i dw = file.header().dataWindow();
	*width  = dw.max.x - dw.min.x + 1;
	*height = dw.max.y - dw.min.y + 1;

	half *rgb = new half[3 * *width * *height];

	FrameBuffer frameBuffer;
	frameBuffer.insert("R", Slice(HALF, (char *)rgb,
		3*sizeof(half), *width * 3 * sizeof(half), 1, 1, 0.0));
	frameBuffer.insert("G", Slice(HALF, (char *)rgb+sizeof(half),
		3*sizeof(half), *width * 3 * sizeof(half), 1, 1, 0.0));
	frameBuffer.insert("B", Slice(HALF, (char *)rgb+2*sizeof(half),
		3*sizeof(half), *width * 3 * sizeof(half), 1, 1, 0.0));

	file.setFrameBuffer(frameBuffer);
	file.readPixels(dw.min.y, dw.max.y);

	Spectrum *ret = new Spectrum[*width * *height];
	// XXX should do real RGB -> Spectrum conversion here
	for (int i = 0; i < *width * *height; ++i) {
		float c[3] = { rgb[3*i], rgb[3*i+1], rgb[3*i+2] };
		//if(i<10) std::cout<<i<<":"<<c[0]<<','<<c[1]<<','<<c[2]<<std::endl;
		ret[i] = Spectrum(c);
	}

	delete[] rgb;
	printf("Done.\n");
	return ret;
	} catch (const std::exception &e) {
		std::stringstream ss;
		ss<<"Unable to read EXR image file '"<<name<<"' : "<<e.what();
		luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
		return NULL;
	}
}


 Spectrum *ReadCimgImage(const string &name, int *width, int *height)
 {
	try {
		printf("Loading Cimg Texture: '%s'...\n", name.c_str());
		CImg<float> image(name.c_str());

		*width  = image.dimx();
 		*height = image.dimy();
 		int pixels=*width * *height;

 		Spectrum *ret = new Spectrum[*width * *height];

 		// XXX should do real RGB -> Spectrum conversion here
		for (int i = 0; i < *width * *height; ++i) {
			float c[3] = { image[i]/255.0, image[i+pixels]/255.0, image[i+pixels*2]/255.0 };
			ret[i] = Spectrum(c);
		}

		printf("Done.\n");
 		return ret;
	} catch (CImgIOException &e) {
		std::stringstream ss;
		ss<<"Unable to read Cimg image file '"<<name<<"' : "<<e.message;
		luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
		return NULL;
	}
 }

 Spectrum *ReadImage(const string &name, int *width, int *height)
 {
	boost::filesystem::path imagePath( name );
	if(!boost::filesystem::exists(imagePath)) {
		std::stringstream ss;
		ss<<"Unable to open image file '"<<imagePath.string()<<"'";
		luxError(LUX_NOFILE, LUX_ERROR, ss.str().c_str());
		return NULL;
	}

	std::string extension = boost::filesystem::extension(imagePath).substr(1);
	//transform extension to lowercase
	#ifdef WIN32
	std::transform ( extension.begin(), extension.end(), extension.begin(), (int(*)(int)) tolower );
	#else
	std::transform ( extension.begin(), extension.end(), extension.begin(), (int(*)(int)) std::tolower );
 	#endif

 	if(extension=="exr") return ReadExrImage(name, width, height);

 	/*
 	The CImg Library can NATIVELY handle the following file formats :
    * RAW : consists in a very simple header (in ascii), then the image data.
    * ASC (Ascii)
    * HDR (Analyze 7.5)
    * INR (Inrimage)
    * PPM/PGM (Portable Pixmap)
    * BMP (uncompressed)
    * PAN (Pandore-5)
    * DLM (Matlab ASCII)*/
    if(extension=="raw") return ReadCimgImage(name, width, height);
    if(extension=="asc") return ReadCimgImage(name, width, height);
    if(extension=="hdr") return ReadCimgImage(name, width, height);
    if(extension=="inr") return ReadCimgImage(name, width, height);
    if(extension=="ppm") return ReadCimgImage(name, width, height);
    if(extension=="pgm") return ReadCimgImage(name, width, height);
    if(extension=="bmp") return ReadCimgImage(name, width, height);
    if(extension=="pan") return ReadCimgImage(name, width, height);
    if(extension=="dlm") return ReadCimgImage(name, width, height);

	// linked formats
    if(extension=="png") return ReadCimgImage(name, width, height);
    if(extension=="tif") return ReadCimgImage(name, width, height);
    if(extension=="tiff") return ReadCimgImage(name, width, height);
    if(extension=="jpg") return ReadCimgImage(name, width, height);
    if(extension=="jpeg") return ReadCimgImage(name, width, height);
    if(extension=="tga") return ReadCimgImage(name, width, height);


	std::stringstream ss;
 	ss<<"Cannot recognise file format for image '"<<name<<"'";
	luxError(LUX_BADFILE, LUX_ERROR, ss.str ().c_str());
 	return NULL;
 }

 void WriteRGBAImage(const string &name, float *pixels,
		float *alpha, int xRes, int yRes,
		int totalXRes, int totalYRes,
		int xOffset, int yOffset) {
	Header header(totalXRes, totalYRes);
	Box2i dataWindow(V2i(xOffset, yOffset), V2i(xOffset + xRes - 1, yOffset + yRes - 1));
	header.dataWindow() = dataWindow;
	header.channels().insert("R", Channel (HALF));
	header.channels().insert("G", Channel (HALF));
	header.channels().insert("B", Channel (HALF));
	header.channels().insert("A", Channel (HALF));

	half *hrgb = new half[3 * xRes * yRes];
	for (int i = 0; i < 3 * xRes * yRes; ++i)
		hrgb[i] = pixels[i];
	half *ha = new half[xRes * yRes];
	for (int i = 0; i < xRes * yRes; ++i)
		ha[i] = alpha[i];

	hrgb -= 3 * (xOffset + yOffset * xRes);
	ha -= (xOffset + yOffset * xRes);

	FrameBuffer fb;
	fb.insert("R", Slice(HALF, (char *)hrgb, 3*sizeof(half),
		3*xRes*sizeof(half)));
	fb.insert("G", Slice(HALF, (char *)hrgb+sizeof(half), 3*sizeof(half),
		3*xRes*sizeof(half)));
	fb.insert("B", Slice(HALF, (char *)hrgb+2*sizeof(half), 3*sizeof(half),
		3*xRes*sizeof(half)));
	fb.insert("A", Slice(HALF, (char *)ha, sizeof(half), xRes*sizeof(half)));

	try {
		OutputFile file(name.c_str(), header);
		file.setFrameBuffer(fb);
		file.writePixels(yRes);
	}
	catch (const std::exception &e) {
		//Error("Unable to write image file \"%s\": %s", name.c_str(),
		//	e.what());
		std::stringstream ss;
		ss<<"Unable to write image file '"<<name<<"' : "<<e.what();
		luxError(LUX_BUG,LUX_SEVERE,ss.str().c_str());
	}

	delete[] (hrgb + 3 * (xOffset + yOffset * xRes));
	delete[] (ha + (xOffset + yOffset * xRes));
}
