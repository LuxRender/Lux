/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

// exrio.cpp*
#include "lux.h"
#include "error.h"
#include "color.h"
#include "spectrum.h"
#include "imagereader.h"
#include "texturecolor.h"
#include <algorithm>

#include <FreeImage.h>

#include <boost/filesystem.hpp>

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
#define cimg_use_tiff 1
#define cimg_use_jpeg 1
#endif //LUX_USE_CONFIG_H


#define cimg_debug 0     // Disable modal window in CImg exceptions.
// Include the CImg Library, with the GREYCstoration plugin included
#define cimg_plugin "greycstoration.h"
#include "cimg.h"
using namespace cimg_library;

#if defined(WIN32) && !defined(__CYGWIN__)
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
using namespace lux;

namespace lux {

class StandardImageReader : public ImageReader {
public:
	StandardImageReader() { };
	virtual ~StandardImageReader() { }

	virtual ImageData* read(const string &name);
};
	
ImageData* createImageData(const string &name,
	FIBITMAP *image);

template <typename T, int C> void* readImageData(FIBITMAP *image, const u_int *channelMapping) {

	u_int width = FreeImage_GetWidth(image);
	u_int height = FreeImage_GetHeight(image);

	void* ret = new TextureColor<T, C>[width * height];

	u_int bpp = FreeImage_GetBPP(image) / 8;
	for (u_int i = 0; i < height; ++i) {
		// FreeImage stores images bottom-up
		BYTE *bits = FreeImage_GetScanLine(image, height-i-1);
		for (u_int j = 0; j < width; ++j) {
			// bpp/bytesPerChannel may not equal noChannels
			T *src = reinterpret_cast<T*>(&bits[j * bpp]);
			for (u_int k = 0; k < C; ++k)
				((TextureColor<T, C> *)ret)[j+(i*width)].c[k] = src[channelMapping[k]];
		}
	}
	
	return ret;
}

template <typename T> void* readImageData(FIBITMAP *image, const u_int noChannels, const u_int *channelMapping) {
	switch (noChannels) {
		case 1:
			return readImageData<T,1>(image, channelMapping);
		case 3:
			return readImageData<T,3>(image, channelMapping);
		case 4:
			return readImageData<T,4>(image, channelMapping);
		default:
			return NULL;
	}
}

void* readImageData(FIBITMAP *image, const u_int bytesPerChannel, const u_int noChannels, const u_int *channelMapping) {
	switch (bytesPerChannel) {
		case 1:
			return readImageData<unsigned char>(image, noChannels, channelMapping);
		case 2:
			return readImageData<unsigned short>(image, noChannels, channelMapping);
		case 4:
			return readImageData<float>(image, noChannels, channelMapping);
		default:
			return NULL;
	}
}


ImageData *createImageData(const std::string &name, FIBITMAP *image) {

	// convert if we need to
	FREE_IMAGE_TYPE fit = FreeImage_GetImageType(image);
	FREE_IMAGE_COLOR_TYPE fic = FreeImage_GetColorType(image);

	u_int bytesPerChannel = 0;
	u_int noChannels = 0;
	bool useChannelMapping = false;

	// determine bytesPerChannel and noChannels
	// as well as convert image if necessary

	// temporary image used for conversion
	FIBITMAP *timage = NULL;

	switch (fit) {
		case FIT_BITMAP: 
			{
				// Standard image: 1-, 4-, 8-, 16-, 24-, 32-bit
				u_int bpp = FreeImage_GetBPP(image);
				bool trans = FreeImage_IsTransparent(image);

				// standardize formats
				if (fic == FIC_PALETTE || bpp < 24) {
					if (trans) {
						timage = FreeImage_ConvertTo32Bits(image);
						noChannels = 4;
					}
					else {
						timage = FreeImage_ConvertTo24Bits(image);
						noChannels = 3;
					}
					image = timage;

				} else if (fic == FIC_RGB) {
					noChannels = 3;
				} else if (fic == FIC_RGBALPHA) {
					noChannels = 4;
				} else {
					LOG(LUX_ERROR, LUX_BADFILE) <<
						"Unsupported color type (type=" << fic << ")";
					image = NULL; // signal error
				}
				bytesPerChannel = 1;
				useChannelMapping = true;
			}
			break;			

		case FIT_FLOAT:		// Array of float: 32-bit IEEE floating point
			bytesPerChannel = 4;
			noChannels = 1;
			break;

		case FIT_UINT16:	// Array of unsigned short: unsigned 16-bit
			bytesPerChannel = 2;
			noChannels = 1;
			break;

		case FIT_INT16:		// Array of short: signed 16-bit
		case FIT_UINT32:	// Array of unsigned long: unsigned 32-bit
		case FIT_INT32:		// Array of long: signed 32-bit
		case FIT_DOUBLE:	// Array of double: 64-bit IEEE floating point
			{
				// can't handle these directly, convert to float
				timage = FreeImage_ConvertToType(image, FIT_FLOAT);
				image = timage;
				bytesPerChannel = 4;
				noChannels = 1;
			}
			break;

		case FIT_RGB16:		// 48-bit RGB image: 3 x 16-bit
			noChannels = 3;
			bytesPerChannel = 2;
			break;

		case FIT_RGBA16:	// 64-bit RGBA image: 4 x 16-bit
			noChannels = 4;
			bytesPerChannel = 2;
			break;

		case FIT_RGBF:		// 96-bit RGB float image: 3 x 32-bit IEEE floating point
			noChannels = 3;
			bytesPerChannel = 4;
			break;

		case FIT_RGBAF:		// 128-bit RGBA float image: 4 x 32-bit IEEE floating point
			noChannels = 4;
			bytesPerChannel = 4;
			break;

		default:
			// FIT_UNKNOWN Unknown format (returned value only, never use it as input value)
			// FIT_COMPLEX Array of FICOMPLEX: 2 x 64-bit IEEE floating point
			LOG(LUX_ERROR, LUX_BADFILE) << "Image unsupported";
			return NULL;
	}

	ImageData::PixelDataType type;
	switch(bytesPerChannel) {
		case 1:
			type = ImageData::UNSIGNED_CHAR_TYPE;
			break;
		case 2:
			type = ImageData::UNSIGNED_SHORT_TYPE;
			break;
		case 4:
			type = ImageData::FLOAT_TYPE;
			break;
		default:
			LOG(LUX_ERROR, LUX_SYSTEM) <<
				"Unsupported pixel type (size=" << bytesPerChannel << ")";
			image = NULL;
	}	

	// something went wrong above
	if (!image) {	
		if (timage)			
			// already printed error
			FreeImage_Unload(timage);
		else
			LOG(LUX_ERROR, LUX_SYSTEM) << "Unable to convert image data";

		return NULL;
	}
	
	u_int width = FreeImage_GetWidth(image);
	u_int height = FreeImage_GetHeight(image);

	void* ret;

	if (useChannelMapping) {
		const u_int bmpChannelMapping[] = {FI_RGBA_RED, FI_RGBA_GREEN, FI_RGBA_BLUE, FI_RGBA_ALPHA};
		ret = readImageData(image, bytesPerChannel, noChannels, bmpChannelMapping);
	} else {
		const u_int stdChannelMapping[] = {0, 1, 2, 3};
		ret = readImageData(image, bytesPerChannel, noChannels, stdChannelMapping);
	}

	if (timage)
		FreeImage_Unload(timage);
	
	if (!ret) {
		LOG(LUX_ERROR, LUX_SYSTEM) << "Unable to read image data";
		return NULL;
	}
	
	ImageData* data = new ImageData(width, height, type, noChannels, ret);

	return data;
}

ImageData *StandardImageReader::read(const string &name)
{
	LOG(LUX_INFO, LUX_NOERROR) << "Loading texture: '" <<
		name << "'...";

	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	// check the file signature and deduce its format
	// (the second argument is currently not used by FreeImage)
	fif = FreeImage_GetFileType(name.c_str(), 0);
	if (fif == FIF_UNKNOWN) {
		// no signature ?
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(name.c_str());
	}
	// check that the plugin has reading capabilities ...
	if (!((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif))) {
		LOG(LUX_ERROR, LUX_BADFILE) << "Image type unknown or unsupported";
		return NULL;
	}

	int flags;

	switch (fif) {
		case FIF_ICO:
			flags = ICO_MAKEALPHA;
			break;
		case FIF_JPEG:
			flags = JPEG_ACCURATE;
			break;
		case FIF_PNG:
			flags = PNG_IGNOREGAMMA;
			break;
		default:
			flags = 0;
	}

	// ok, let's load the file
	FIBITMAP *image = FreeImage_Load(fif, name.c_str(), flags);
	// unless a bad file format, we are done !
	
	ImageData *data = createImageData(name, image);		
	
	// data may be NULL in case of error

	FreeImage_Unload(image);

	return data;
}

ImageData *ReadImage(const string &name)
{
	try {
		boost::filesystem::path imagePath(AdjustFilename(name));
		// boost::filesystem::exists() can throw an exception under Windows
		// if the drive in imagePath doesn't exist
		if (!boost::filesystem::exists(imagePath)) {
			LOG(LUX_ERROR, LUX_NOFILE) <<
				"Unable to open image file '" <<
				imagePath.string() << "'";
			return NULL;
		}

		StandardImageReader stdImageReader;
		return stdImageReader.read(imagePath.string());

		LOG(LUX_ERROR, LUX_BADFILE) <<
			"Cannot recognise file format for image '" <<
			name << "'";
		return NULL;
	} catch (const std::exception &e) {
		LOG(LUX_ERROR, LUX_BUG) << "Unable to read image file '" <<
			name << "': " << e.what();
		return NULL;
	}
}

/*
 * To convert a standard EXR to Blender MultiLayer format, change the channel names:
 * RenderLayer.Combined.R
 * RenderLayer.Combined.G
 * RenderLayer.Combined.B
 * RenderLayer.Combined.A
 * RenderLayer.Depth.Z
 * (and force RGBA format)
 *
 * and set a header
 * header.insert("BlenderMultiChannel", StringAttribute("Blender V2.43 and newer"));
 *
 * it may also be necessary to flip image data both horizonally and vertically
 */

void WriteOpenEXRImage(int channeltype, bool halftype, bool savezbuf,
	int compressiontype, const string &name, vector<RGBColor> &pixels,
	vector<float> &alpha, u_int xRes, u_int yRes,
	u_int totalXRes, u_int totalYRes, u_int xOffset, u_int yOffset,
	vector<float> &zbuf)
{
	Header header(totalXRes, totalYRes);

	// Set Compression
	switch(compressiontype) {
		case 0:
			header.compression() = RLE_COMPRESSION;
			break;
		case 1:
			header.compression() = PIZ_COMPRESSION;
			break;
		case 2:
			header.compression() = ZIP_COMPRESSION;
			break;
		case 3:
			header.compression() = PXR24_COMPRESSION;
			break;
		case 4:
			header.compression() = NO_COMPRESSION;
			break;
		default:
			header.compression() = RLE_COMPRESSION;
			break; 
	}

	Box2i dataWindow(V2i(xOffset, yOffset),
		V2i(xOffset + xRes - 1, yOffset + yRes - 1));
	header.dataWindow() = dataWindow;

	// Set base channel type
	Imf::PixelType savetype = Imf::FLOAT;
	if (halftype)
		savetype = Imf::HALF;

	// Define channels
	if (channeltype == 0) {
		header.channels().insert("Y", Imf::Channel(savetype));
	} else if(channeltype == 1) {
		header.channels().insert("Y", Imf::Channel(savetype));
		header.channels().insert("A", Imf::Channel(savetype));
	} else if(channeltype == 2) {
		header.channels().insert("R", Imf::Channel(savetype));
		header.channels().insert("G", Imf::Channel(savetype));
		header.channels().insert("B", Imf::Channel(savetype));
	} else {
		header.channels().insert("R", Imf::Channel(savetype));
		header.channels().insert("G", Imf::Channel(savetype));
		header.channels().insert("B", Imf::Channel(savetype));
		header.channels().insert("A", Imf::Channel(savetype));
	}

	// Add 32bit float Zbuf channel if required
	if (savezbuf)
		header.channels().insert("Z", Imf::Channel(Imf::FLOAT));

	FrameBuffer fb;

	// Those buffers will hold image data in case a type
	// conversion is needed.
	// THEY MUST NOT BE DELETED BEFORE DATA IS WRITTEN TO FILE
	float *fy = NULL;
	half *hy = NULL;
	half *hrgb = NULL;
	half *ha = NULL;
	const u_int bufSize = xRes * yRes;
	const u_int bufOffset = xOffset + yOffset * xRes;

	if (!halftype) {
		// Write framebuffer data for 32bit FLOAT type
		if (channeltype <= 1) {
			// Save Y
			fy = new float[bufSize];
			// FIXME use the correct color space
			for (u_int i = 0; i < bufSize; ++i)
				fy[i] = (0.3f * pixels[i].c[0]) +
					(0.59f * pixels[i].c[1]) +
					(0.11f * pixels[i].c[2]);
				fb.insert("Y", Slice(Imf::FLOAT,
					(char *)(fy - bufOffset), sizeof(float),
					xRes * sizeof(float)));
		} else if (channeltype >= 2) {
			// Save RGB
			float *frgb = &pixels[0].c[0];
			fb.insert("R", Slice(Imf::FLOAT,
				(char *)(frgb - 3 * bufOffset),
				sizeof(RGBColor), xRes * sizeof(RGBColor)));
			fb.insert("G", Slice(Imf::FLOAT,
				(char *)(frgb - 3 * bufOffset) + sizeof(float),
				sizeof(RGBColor), xRes * sizeof(RGBColor)));
			fb.insert("B", Slice(Imf::FLOAT,
				(char *)(frgb - 3 * bufOffset) + 2 * sizeof(float),
				sizeof(RGBColor), xRes * sizeof(RGBColor)));
		}
		if (channeltype == 1 || channeltype == 3) {
			// Add alpha
			float *fa = &alpha[0];
			fb.insert("A", Slice(Imf::FLOAT,
				(char *)(fa - bufOffset), sizeof(float),
				xRes * sizeof(float)));
		}
	} else {
		// Write framebuffer data for 16bit HALF type
		if (channeltype <= 1) {
			// Save Y
			hy = new half[bufSize];
			//FIXME use correct color space
			for (u_int i = 0; i < bufSize; ++i)
				hy[i] = (0.3f * pixels[i].c[0]) +
					(0.59f * pixels[i].c[1]) +
					(0.11f * pixels[i].c[2]);
			fb.insert("Y", Slice(HALF, (char *)(hy - bufOffset),
				sizeof(half), xRes * sizeof(half)));
		} else if (channeltype >= 2) {
			// Save RGB
			hrgb = new half[3 * bufSize];
			for (u_int i = 0; i < bufSize; ++i)
				for( u_int j = 0; j < 3; j++)
					hrgb[3 * i + j] = pixels[i].c[j];
			fb.insert("R", Slice(HALF,
				(char *)(hrgb - 3 * bufOffset),
				3 * sizeof(half), xRes * (3 * sizeof(half))));
			fb.insert("G", Slice(HALF,
				(char *)(hrgb - 3 * bufOffset) + sizeof(half),
				3 * sizeof(half), xRes * (3 * sizeof(half))));
			fb.insert("B", Slice(HALF,
				(char *)(hrgb - 3 * bufOffset) + 2 * sizeof(half),
				3 * sizeof (half), xRes * (3 * sizeof (half))));
		}
		if (channeltype == 1 || channeltype == 3) {
			// Add alpha
			ha = new half[bufSize];
			for (u_int i = 0; i < bufSize; ++i)
				ha[i] = alpha[i];		
			fb.insert("A", Slice(HALF, (char *)(ha - bufOffset),
				sizeof(half), xRes * sizeof(half)));
		}
	}

	if(savezbuf) {
		float *fz = &zbuf[0];
		// Add Zbuf framebuffer data (always 32bit FLOAT type)
		fb.insert("Z", Slice(Imf::FLOAT, (char *)(fz - bufOffset),
			sizeof(float), xRes * sizeof(float)));
	}

	try {
		OutputFile file(name.c_str(), header);
		file.setFrameBuffer(fb);
		file.writePixels(yRes);
	} catch (const std::exception &e) {
		LOG(LUX_SEVERE, LUX_BUG) << "Unable to write image file '" <<
			name << "': " << e.what();
	}

	// Cleanup used buffers
	// If the pointer is NULL, delete[] has no effect
	// So it is safe to avoid the NULL check of those pointers
	delete[] fy;
	delete[] hy;
	delete[] hrgb;
	delete[] ha;
}

// Write a single channel float EXR
void WriteOpenEXRImage(const string &name, u_int xRes, u_int yRes, const float *map) {
	Header header(xRes, yRes);
	header.compression() = RLE_COMPRESSION;

	Box2i dataWindow(V2i(0, 0),
		V2i(0 + xRes - 1, 0 + yRes - 1));
	header.dataWindow() = dataWindow;

	Imf::PixelType savetype = Imf::FLOAT;
	header.channels().insert("Y", Imf::Channel(savetype));

	FrameBuffer fb;
	fb.insert("Y", Slice(Imf::FLOAT,
		(char *)map, sizeof(float),
		xRes * sizeof(float)));

	try {
		OutputFile file(name.c_str(), header);
		file.setFrameBuffer(fb);
		file.writePixels(yRes);
	} catch (const std::exception &e) {
		LOG(LUX_SEVERE, LUX_BUG) << "Unable to write image file '" <<
			name << "': " << e.what();
	}
}

}
