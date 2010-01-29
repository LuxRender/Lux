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

// exrio.cpp*
#include "lux.h"
#include "error.h"
#include "color.h"
#include "spectrum.h"
#include "imagereader.h"
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

    template <class T >
    class StandardImageReader : public ImageReader {
    public:
        StandardImageReader() { };
	virtual ~StandardImageReader() { }

        virtual ImageData* read(const string &name);
    };
	
	template <class T>
	ImageData* createImageData( const string& name, const CImg<T>& image );

    // EXR Function Definitions

    ImageData *ExrImageReader::read(const string &name) {
        try {
            LOG(LUX_INFO,LUX_NOERROR)<< "Loading OpenEXR Texture: '" << name << "'...";

			InputFile file(name.c_str());
            Box2i dw = file.header().dataWindow();
            u_int width = dw.max.x - dw.min.x + 1;
            u_int height = dw.max.y - dw.min.y + 1;
            //todo: verify if this is always correct
            u_int noChannels = 3;

			LOG(LUX_INFO,LUX_NOERROR)<<  width << "x" << height << " (" << noChannels << " channels)";

            half *rgb = new half[noChannels * width * height];

            FrameBuffer frameBuffer;
            frameBuffer.insert("R", Slice(HALF, (char *) rgb,
                    3 * sizeof (half), width * 3 * sizeof (half), 1, 1, 0.0));
            frameBuffer.insert("G", Slice(HALF, (char *) rgb + sizeof (half),
                    3 * sizeof (half), width * 3 * sizeof (half), 1, 1, 0.0));
            frameBuffer.insert("B", Slice(HALF, (char *) rgb + 2 * sizeof (half),
                    3 * sizeof (half), width * 3 * sizeof (half), 1, 1, 0.0));

            file.setFrameBuffer(frameBuffer);
            file.readPixels(dw.min.y, dw.max.y);
            TextureColor<float, 3 > *ret = new TextureColor<float, 3 > [width * height];
            ImageData* data = new ImageData(width, height, ImageData::FLOAT_TYPE, noChannels, ret);

            // XXX should do real RGB -> RGBColor conversion here
            for (u_int i = 0; i < width * height; ++i) {
                float c[3] = { rgb[(3 * i)], rgb[(3 * i) + 1], rgb[(3 * i) + 2] };
                ret[i] = TextureColor<float, 3 > (c);

				//if(i<10) std::cout<<i<<":"<<c[0]<<','<<c[1]<<','<<c[2]<<std::endl;
            }

			delete[] rgb;

            return data;
        } catch (const std::exception &e) {
        	LOG(LUX_ERROR,LUX_BUG)<< "Unable to read EXR image file '" << name << "': " << e.what();
            return NULL;
        }
    }

	template <typename T> ImageData * createImageData( const string& name, const CImg<T>& image ) {
		size_t size = sizeof (T);

        ImageData::PixelDataType type;
        if (size == 1)
            type = ImageData::UNSIGNED_CHAR_TYPE;
        if (size == 2)
            type = ImageData::UNSIGNED_SHORT_TYPE;
        if (size == 4)
            type = ImageData::FLOAT_TYPE;

        u_int width = image.dimx();
        u_int height = image.dimy();
        u_int noChannels = image.dimv();
        u_int pixels = width * height;

        LOG(LUX_INFO,LUX_NOERROR)<< width << "x" << height << " (" << noChannels << " channels)";
		
        TextureColorBase* ret;
		// Dade - we support 1, 3 and 4 channel images
		if (noChannels == 1)
			ret = new TextureColor<T, 1> [width * height];
		else if (noChannels == 3)
            ret = new TextureColor<T, 3> [width * height];
        else if (noChannels == 4)
            ret = new TextureColor<T, 4> [width * height];
		else {
			LOG(LUX_ERROR,LUX_SYSTEM)<< "Unsupported channel count in StandardImageReader::read(" << name << ": " << noChannels;
			return NULL;
		}

        ImageData* data = new ImageData(width, height, type, noChannels, ret);
        T *c = new T[noChannels];
        c[0] = 0;
        // XXX should do real RGB -> RGBColor conversion here
        for (u_int i = 0; i < width; ++i)
            for (u_int j = 0; j < height; ++j) {
                for (u_int k = 0; k < noChannels; ++k) {
                    // assuming that cimg depth is 1, single image layer
                    u_int off = i + (j * width) + (k * pixels);

					if (noChannels == 1)
						((TextureColor<T, 1> *)ret)[i + (j * width)].c[k] = image[off];
					else if (noChannels == 3)
                        ((TextureColor<T, 3> *)ret)[i + (j * width)].c[k] = image[off];
                    else
                        ((TextureColor<T, 4> *)ret)[i + (j * width)].c[k] = image[off];
                }
            }
        delete [] c;

        return data;
    }

    template <typename T> ImageData * StandardImageReader<T>::read(const string &name) {
        try {
        	LOG(LUX_INFO,LUX_NOERROR)<< "Loading Cimg Texture: '" << name << "'...";

			CImg<T> image(name.c_str());
            size_t size = sizeof (T);

			if( size != 1 ) {
				// Check if conversion to a smaller type is possible without data loss
				T maxVal = image.max();
				if( maxVal <= cimg::type<unsigned char>::max() ) {
					CImg<unsigned char> tmpImage = image;
					return createImageData( name, tmpImage );
				}
			}
			return createImageData( name, image );
		} catch (CImgIOException &e) {
			LOG(LUX_ERROR,LUX_BUG)<< "Unable to read Cimg image file '" << name << "': " << e.message;
            return NULL;
        }
        return NULL;
	}

    ImageData *ReadImage(const string &name) {
		try {
			boost::filesystem::path imagePath(name);
			// boost::filesystem::exists() can throw an exception under Windows
			// if the drive in imagePath doesn't exist
			if (!boost::filesystem::exists(imagePath)) {
				LOG(LUX_ERROR,LUX_NOFILE)<< "Unable to open image file '" << imagePath.string() << "'";
				return NULL;
			}

			std::string extension = boost::filesystem::extension(imagePath).substr(1);
			//transform extension to lowercase
	#if defined(WIN32) && !defined(__CYGWIN__)
			std::transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int)) tolower);
	#else
			std::transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int)) std::tolower);
	#endif

			if (extension == "exr") {
				ExrImageReader exrReader;
				ImageData* data = exrReader.read(name);
				data->setIsExrImage(true);

				return data;
			}
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
			if ((extension == "raw") ||
					(extension == "asc") ||
					(extension == "hdr") ||
					(extension == "inr") ||
					(extension == "ppm") ||
					(extension == "pgm") ||
					(extension == "bmp") ||
					(extension == "pan") ||
					(extension == "dlm")) {
				//StandardImageReader stdImageReader;
				StandardImageReader<unsigned char> stdImageReader;
				return stdImageReader.read(name);
			}
			// linked formats
			if ((extension == "jpg") ||
				(extension == "jpeg")) {
				//StandardImageReader stdImageReader;
				StandardImageReader<unsigned char> stdImageReader;
				return stdImageReader.read(name);
			}
			// handle potential 16 bit image types separately
			if((extension == "png") ||
			   (extension == "tif") || (extension == "tiff") ||
			   (extension == "tga") ) {
				//StandardImageReader stdImageReader;
				StandardImageReader<unsigned short> stdImageReader;
				return stdImageReader.read(name);
			}

			LOG(LUX_ERROR,LUX_BADFILE)<< "Cannot recognise file format for image '" << name << "'";
			return NULL;
		} catch (const std::exception &e) {
			LOG(LUX_ERROR,LUX_BUG)<< "Unable to read image file '" << name << "': " << e.what();
			return NULL;
		}
    }

   void WriteOpenEXRImage(int channeltype, bool halftype, bool savezbuf, int compressiontype, const string &name, vector<RGBColor> &pixels,
            vector<float> &alpha, u_int xRes, u_int yRes,
            u_int totalXRes, u_int totalYRes,
            u_int xOffset, u_int yOffset, vector<float> &zbuf) {
		Header header(totalXRes, totalYRes);

		// Set Compression
		switch(compressiontype)
		{
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

		Box2i dataWindow(V2i(xOffset, yOffset), V2i(xOffset + xRes - 1, yOffset + yRes - 1));
		header.dataWindow() = dataWindow;

		// Set base channel type
		Imf::PixelType savetype = Imf::FLOAT;
		if(halftype)
			savetype = Imf::HALF;

		// Define channels
		if(channeltype == 0) {
			header.channels().insert("Y", Channel(savetype));
		} else if(channeltype == 1) {
			header.channels().insert("Y", Channel(savetype));
			header.channels().insert("A", Channel(savetype));
		} else if(channeltype == 2) {
			header.channels().insert("R", Channel(savetype));
			header.channels().insert("G", Channel(savetype));
			header.channels().insert("B", Channel(savetype));
		} else {
			header.channels().insert("R", Channel(savetype));
			header.channels().insert("G", Channel(savetype));
			header.channels().insert("B", Channel(savetype));
			header.channels().insert("A", Channel(savetype));
		}

		// Add 32bit float Zbuf channel if required
		if(savezbuf)
			header.channels().insert("Z", Channel(Imf::FLOAT));

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

		if(!halftype) {
			// Write framebuffer data for 32bit FLOAT type
			if(channeltype <= 1) {
				// Save Y
				fy = new float[bufSize];
				// FIXME use the correct color space
				for (u_int i = 0; i < bufSize; ++i)
					fy[i] = (0.3f * pixels[i].c[0]) + (0.59f * pixels[i].c[1]) + (0.11f * pixels[i].c[2]);
				fb.insert("Y", Slice(Imf::FLOAT, (char *)(fy - bufOffset), sizeof(float), xRes * sizeof(float)));
			} else if(channeltype >= 2) {
				// Save RGB
				float *frgb = &pixels[0].c[0];
				fb.insert("R", Slice(Imf::FLOAT, (char *)(frgb - 3 * bufOffset), sizeof(RGBColor), xRes * sizeof(RGBColor)));
				fb.insert("G", Slice(Imf::FLOAT, (char *)(frgb - 3 * bufOffset) + sizeof(float), sizeof(RGBColor), xRes * sizeof(RGBColor)));
				fb.insert("B", Slice(Imf::FLOAT, (char *)(frgb - 3 * bufOffset) + 2 * sizeof(float), sizeof(RGBColor), xRes * sizeof(RGBColor)));
			}
			if(channeltype == 1 || channeltype == 3) {
				// Add alpha
				float *fa = &alpha[0];
				fb.insert("A", Slice(Imf::FLOAT, (char *)(fa - bufOffset), sizeof(float), xRes * sizeof(float)));
			}
		} else {
			// Write framebuffer data for 16bit HALF type
			if(channeltype <= 1) {
				// Save Y
				hy = new half[bufSize];
				//FIXME use correct color space
				for (u_int i = 0; i < bufSize; ++i)
					hy[i] = (0.3f * pixels[i].c[0]) + (0.59f * pixels[i].c[1]) + (0.11f * pixels[i].c[2]);
				fb.insert("Y", Slice(HALF, (char *)(hy - bufOffset), sizeof(half), xRes * sizeof(half)));
			} else if(channeltype >= 2) {
				// Save RGB
				hrgb = new half[3 * bufSize];
				for (u_int i = 0; i < bufSize; ++i)
					for( u_int j = 0; j < 3; j++)
						hrgb[3 * i + j] = pixels[i].c[j];
				fb.insert("R", Slice(HALF, (char *)(hrgb - 3 * bufOffset), 3 * sizeof(half), xRes * (3 * sizeof(half))));
				fb.insert("G", Slice(HALF, (char *)(hrgb - 3 * bufOffset) + sizeof(half), 3 * sizeof(half), xRes * (3 * sizeof(half))));
				fb.insert("B", Slice(HALF, (char *)(hrgb - 3 * bufOffset) + 2 * sizeof(half), 3 * sizeof (half), xRes * (3 * sizeof (half))));
			}
			if(channeltype == 1 || channeltype == 3) {
				// Add alpha
				ha = new half[bufSize];
				for (u_int i = 0; i < bufSize; ++i)
					ha[i] = alpha[i];		
				fb.insert("A", Slice(HALF, (char *)(ha - bufOffset), sizeof(half), xRes * sizeof(half)));
			}
		}

		if(savezbuf) {
			float *fz = &zbuf[0];
			// Add Zbuf framebuffer data (always 32bit FLOAT type)
			fb.insert("Z", Slice(Imf::FLOAT, (char *)(fz - bufOffset), sizeof(float), xRes * sizeof(float)));
		}

		try {
			OutputFile file(name.c_str(), header);
			file.setFrameBuffer(fb);
			file.writePixels(yRes);
		} catch (const std::exception &e) {
			LOG(LUX_SEVERE,LUX_BUG)<< "Unable to write image file '" << name << "': " << e.what();
		}

		// Cleanup used buffers
		// If the pointer is NULL, delete[] has no effect
		// So it is safe to avoid the NULL check of those pointers
		delete[] fy;
		delete[] hy;
		delete[] hrgb;
		delete[] ha;

    }
}
