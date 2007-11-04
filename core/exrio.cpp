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
//#endif
#include <algorithm>

#define cimg_display_type  0
#define cimg_use_png 1
//#define cimg_use_tiff 1
//#define cimg_use_jpeg 1
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
		//Error("Unable to read EXR image file \"%s\": %s", name.c_str(),
		//	e.what());
		std::stringstream ss;
		ss<<"Unable to read EXR image file '"<<name<<"' : "<<e.what();
		luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
		return NULL;
	}
}


 Spectrum *ReadCimgImage(const string &name, int *width, int *height)
 {
	printf("Loading Cimg Texture: '%s'...\n", name.c_str());
	CImg<float> image(name.c_str());
	//CImg<float> imagexyz=image.RGBtoXYZ ();
 	*width  = image.dimx();
 	*height = image.dimy();
 	int pixels=*width * *height;
 	
 	Spectrum *ret = new Spectrum[*width * *height];
	//printf("%f %f %f\n",image(6,6,0,0),image(6,6,0,1),image(6,6,0,2) );
 	
 	// XXX should do real RGB -> Spectrum conversion here
	for (int i = 0; i < *width * *height; ++i) {
		float c[3] = { image[i]/255.0, image[i+pixels]/255.0, image[i+pixels*2]/255.0 };
		//if(i<10) std::cout<<i<<":"<<c[0]<<','<<c[1]<<','<<c[2]<<std::endl;
		ret[i] = Spectrum(c);
	}
 	
    printf("Done.\n");
 	return ret;
 }

 Spectrum *ReadImage(const string &name, int *width, int *height)
 {
	int p = name.find_last_of('.',name.size());
	//string fileName = name.substr(0, p);
	std::string extension = name.substr(p+1, name.size()-p-1);
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
    
 	
 	std::string errorMessage="Cannot recognise file format for file : "+name;
 	luxError(LUX_ERROR,LUX_BADFILE,errorMessage.c_str());
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

	OutputFile file(name.c_str(), header);
	file.setFrameBuffer(fb);
	try {
		file.writePixels(yRes);
	}
	catch (const std::exception &e) {
		//Error("Unable to write image file \"%s\": %s", name.c_str(),
		//	e.what());
		std::stringstream ss;
		ss<<"Unable to write image file '"<<name<<"' : "<<e.what();
		luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
	}

	delete[] (hrgb + 3 * (xOffset + yOffset * xRes));
	delete[] (ha + (xOffset + yOffset * xRes));
}
