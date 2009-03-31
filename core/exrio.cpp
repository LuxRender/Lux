/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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
//#ifdef __APPLE__
#include "lux.h"
#include "error.h"
#include "color.h"
#include "color.h"
#include "spectrum.h"
#include "imagereader.h"
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

        StandardImageReader() {
        };
        ImageData* read(const string &name);
    };

    // EXR Function Definitions

    ImageData *ExrImageReader::read(const string &name) {
        try {
			stringstream ss;
			ss << "Loading OpenEXR Texture: '" << name << "'...";
            luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

			InputFile file(name.c_str());
            Box2i dw = file.header().dataWindow();
            int width = dw.max.x - dw.min.x + 1;
            int height = dw.max.y - dw.min.y + 1;
            //todo: verify if this is always correct
            int noChannels = 3;

			ss.str("");
			ss << width << "x" << height << " (" << noChannels << " channels)";
            luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

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
            for (int i = 0; i < width * height; ++i) {
                float c[3] = { rgb[(3 * i)], rgb[(3 * i) + 1], rgb[(3 * i) + 2] };
                ret[i] = TextureColor<float, 3 > (c);

				//if(i<10) std::cout<<i<<":"<<c[0]<<','<<c[1]<<','<<c[2]<<std::endl;
            }

			delete[] rgb;

            return data;
        } catch (const std::exception &e) {
            std::stringstream ss;
            ss << "Unable to read EXR image file '" << name << "': " << e.what();
            luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
            return NULL;
        }
    }

    template <typename T> ImageData * StandardImageReader<T>::read(const string &name) {
        try {
			stringstream ss;
			ss << "Loading Cimg Texture: '" << name << "'...";
            luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

			CImg<T> image(name.c_str());
            size_t size = sizeof (T);

            ImageData::PixelDataType type;
            if (size == 1)
                type = ImageData::UNSIGNED_CHAR_TYPE;
            if (size == 2)
                type = ImageData::UNSIGNED_SHORT_TYPE;
            if (size == 4)
                type = ImageData::FLOAT_TYPE;

            int width = image.dimx();
            int height = image.dimy();
            int noChannels = image.dimv();
            int pixels = width * height;

			ss.str("");
			ss << width << "x" << height << " (" << noChannels << " channels)";
            luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
			
            TextureColorBase* ret;
			// Dade - we support 1, 3 and 4 channel images
			if (noChannels == 1)
				ret = new TextureColor<T, 1> [width * height];
			else if (noChannels == 3)
                ret = new TextureColor<T, 3> [width * height];
            else if (noChannels == 4)
                ret = new TextureColor<T, 4> [width * height];
			else {
				ss.str("");
				ss << "Unsupported channel count in StandardImageReader::read(" <<
						name << ": " << noChannels;
				luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());

				return NULL;
			}

            ImageData* data = new ImageData(width, height, type, noChannels, ret);
            T *c = new T[noChannels];
            c[0] = 0;
            // XXX should do real RGB -> RGBColor conversion here
            for (int i = 0; i < width; ++i)
                for (int j = 0; j < height; ++j) {
                    for (int k = 0; k < noChannels; ++k) {
                        // assuming that cimg depth is 1, single image layer
                        unsigned long off = i + (j * width) + (k * pixels);

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
        } catch (CImgIOException &e) {
            std::stringstream ss;
            ss << "Unable to read Cimg image file '" << name << "': " << e.message;
            luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
            return NULL;
        }
        return NULL;
    }

    ImageData *ReadImage(const string &name) {
        boost::filesystem::path imagePath(name);
        if (!boost::filesystem::exists(imagePath)) {
            std::stringstream ss;
            ss << "Unable to open image file '" << imagePath.string() << "'";
            luxError(LUX_NOFILE, LUX_ERROR, ss.str().c_str());
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
        if ((extension == "png") ||
                (extension == "jpg") ||
                (extension == "jpeg")) {
            //StandardImageReader stdImageReader;
            StandardImageReader<unsigned char> stdImageReader;
            return stdImageReader.read(name);
        }

        //todo: handle 16 bit tiffs here
        //(but how do we know when a tiff is 16 bit??)
        if ((extension == "tif") || (extension == "tiff")) {
            //StandardImageReader stdImageReader;
            StandardImageReader<unsigned char> stdImageReader;
            return stdImageReader.read(name);
        }
        if (extension == "tga") {
            //StandardImageReader stdImageReader;
            StandardImageReader<unsigned char> stdImageReader;
            return stdImageReader.read(name);
        }

        std::stringstream ss;
        ss << "Cannot recognise file format for image '" << name << "'";
        luxError(LUX_BADFILE, LUX_ERROR, ss.str().c_str());
        return NULL;
    }

    void WriteRGBAImage(const string &name, float *pixels,
            float *alpha, int xRes, int yRes,
            int totalXRes, int totalYRes,
            int xOffset, int yOffset) {
        Header header(totalXRes, totalYRes);
        Box2i dataWindow(V2i(xOffset, yOffset), V2i(xOffset + xRes - 1, yOffset + yRes - 1));
        header.dataWindow() = dataWindow;
        header.channels().insert("R", Channel(HALF));
        header.channels().insert("G", Channel(HALF));
        header.channels().insert("B", Channel(HALF));
        header.channels().insert("A", Channel(HALF));

        half *hrgb = new half[3 * xRes * yRes];
        for (int i = 0; i < 3 * xRes * yRes; ++i)
            hrgb[i] = pixels[i];
        half *ha = new half[xRes * yRes];
        for (int i = 0; i < xRes * yRes; ++i)
            ha[i] = alpha[i];

        hrgb -= 3 * (xOffset + yOffset * xRes);
        ha -= (xOffset + yOffset * xRes);

        FrameBuffer fb;
        fb.insert("R", Slice(HALF, (char *) hrgb, 3 * sizeof (half),
                3 * xRes * sizeof (half)));
        fb.insert("G", Slice(HALF, (char *) hrgb + sizeof (half), 3 * sizeof (half),
                3 * xRes * sizeof (half)));
        fb.insert("B", Slice(HALF, (char *) hrgb + 2 * sizeof (half), 3 * sizeof (half),
                3 * xRes * sizeof (half)));
        fb.insert("A", Slice(HALF, (char *) ha, sizeof (half), xRes * sizeof (half)));

        try {
            OutputFile file(name.c_str(), header);
            file.setFrameBuffer(fb);
            file.writePixels(yRes);
        } catch (const std::exception &e) {
            //Error("Unable to write image file \"%s\": %s", name.c_str(),
            //	e.what());
            std::stringstream ss;
            ss << "Unable to write image file '" << name << "': " << e.what();
            luxError(LUX_BUG, LUX_SEVERE, ss.str().c_str());
        }

        delete[] (hrgb + 3 * (xOffset + yOffset * xRes));
        delete[] (ha + (xOffset + yOffset * xRes));
    }

    void WriteRGBAImageFloat(const string &name, vector<Color> &pixels,
            vector<float> &alpha, int xRes, int yRes,
            int totalXRes, int totalYRes,
            int xOffset, int yOffset) {
		Header header(totalXRes, totalYRes);
		Box2i dataWindow(V2i(xOffset, yOffset), V2i(xOffset + xRes - 1, yOffset + yRes - 1));
		header.dataWindow() = dataWindow;
		header.channels().insert("R", Channel(Imf::FLOAT));
		header.channels().insert("G", Channel(Imf::FLOAT));
		header.channels().insert("B", Channel(Imf::FLOAT));
		header.channels().insert("A", Channel(Imf::FLOAT));

		// NOTE - lordcrc - No need to perform a local copy of the pixels
		float *hrgb = &pixels[0].c[0];
		float *ha = &alpha[0];

		hrgb -= 3 * (xOffset + yOffset * xRes);
		ha -= (xOffset + yOffset * xRes);

		FrameBuffer fb;
		fb.insert("R", Slice(Imf::FLOAT, (char *) hrgb, sizeof (Color),
				xRes * sizeof (Color)));
		fb.insert("G", Slice(Imf::FLOAT, (char *) hrgb + sizeof (float), sizeof (Color),
				xRes * sizeof (Color)));
		fb.insert("B", Slice(Imf::FLOAT, (char *) hrgb + 2 * sizeof (float), sizeof (Color),
				xRes * sizeof (Color)));
		fb.insert("A", Slice(Imf::FLOAT, (char *) ha, sizeof (float), xRes * sizeof (float)));

		try {
			OutputFile file(name.c_str(), header);
			file.setFrameBuffer(fb);
			file.writePixels(yRes);
		} catch (const std::exception &e) {
			std::stringstream ss;
			ss << "Unable to write image file '" << name << "': " << e.what();
			luxError(LUX_BUG, LUX_SEVERE, ss.str().c_str());
		}
    }
}
