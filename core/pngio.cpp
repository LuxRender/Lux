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

// pngio.cpp*
#include "pngio.h"
#include "color.h"
#include "error.h"
#include "osfunc.h"
#include <png.h>

namespace lux {


void lux_png_error(png_structp png_, png_const_charp msg)
{
		LOG( LUX_SEVERE,LUX_SYSTEM)<< "Cannot open PNG file '"<<msg<<"' for output";
}


  void WritePngImage(int channeltype, bool ubit, bool savezbuf, const string &name, vector<RGBColor> &pixels,
            vector<float> &alpha, u_int xPixelCount, u_int yPixelCount,
        u_int xResolution, u_int yResolution,
        u_int xPixelStart, u_int yPixelStart, ColorSystem &cSystem, float screenGamma) {

    //
    // PNG writing starts here!
    //
    //   xPixelCount, yPixelCount....size of cropped region
    //   xResolution, yResolution....size of entire image (usually the same as the cropped image)
    //   xPixelStart, yPixelStart....offset of cropped region (usually 0,0)
    //

    FILE *fp = fopen(name.c_str(), "wb");
	if (!fp) {
		LOG( LUX_SEVERE,LUX_SYSTEM) << "Cannot open PNG file '" << name << "' for output";
		return;
	}

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, lux_png_error, NULL);
    png_infop info = png_create_info_struct(png);
    png_init_io(png, fp);

    // Registered PNG keywords are:
    // Title           7 Mar 95       PNG-1.2
    // Author          7 Mar 95       PNG-1.2
    // Description     7 Mar 95       PNG-1.2
    // Copyright       7 Mar 95       PNG-1.2
    // Creation Time   7 Mar 95       PNG-1.2
    // Software        7 Mar 95       PNG-1.2
    // Disclaimer      7 Mar 95       PNG-1.2
    // Warning         7 Mar 95       PNG-1.2
    // Source          7 Mar 95       PNG-1.2
    // Comment         7 Mar 95       PNG-1.2

    png_text text;
    text.compression = PNG_TEXT_COMPRESSION_NONE;
    text.key = (png_charp)"Software";
    text.text = (png_charp)"LuxRender";
    text.text_length = 4;
    png_set_text(png, info, &text, 1);

    png_color_16 black = {0, 0, 0, 0, 0};
    png_set_background(png, &black, PNG_BACKGROUND_GAMMA_SCREEN, 0, 255.0);

    // gAMA:
    //
    // From the PNG spec: (http://www.w3.org/TR/PNG/)
    //
    // Computer graphics renderers often do not perform gamma encoding, instead
    // making sample values directly proportional to scene light intensity. If
    // the PNG encoder receives sample values that have already been 
    // quantized into integer values, there is no point in doing gamma encoding 
    // on them; that would just result in further loss of information. The 
    // encoder should just write the sample values to the PNG datastream. This
    // does not imply that the gAMA chunk should contain a gamma value of 1.0 
    // because the desired end-to-end transfer function from scene intensity to 
    // display output intensity is not necessarily linear. However, the desired 
    // gamma value is probably not far from 1.0. It may depend on whether the 
    // scene being rendered is a daylight scene or an indoor scene, etc.
    //
    // Given the above text, I decided that the user's requested gamma value
    // should be directly encoded into the PNG gAMA chunk; it is NOT a request
    // for pbrt to do gamma processing, since doing so would infer a needless 
    // loss of information.  The PNG authors say that the gamma value should
    // not necessarily be 1.0, but given the physically rigorous nature of 
    // pbrt, I think it would be rare to use something other than 1.0.
    //
    // Note there is no option for premultiply alpha.
    // It was purposely omitted because the PNG spec states that PNG never uses this.
    //
    // cHRM: is: CIE x,y chromacities of R, G, B and white.
    //
    // x = X / (X + Y + Z)
    // y = Y / (X + Y + Z)

	float fileGamma = 1.0f;
    png_set_gamma(png, screenGamma, fileGamma);

    png_set_cHRM(png, info, cSystem.xWhite, cSystem.yWhite, cSystem.xRed, 
		cSystem.yRed, cSystem.xGreen, cSystem.yGreen, cSystem.xBlue, cSystem.yBlue);

	int colorType;
	switch (channeltype) {
		case 0: 
			colorType = PNG_COLOR_TYPE_GRAY;
			break;
		case 1: 
			colorType = PNG_COLOR_TYPE_GRAY_ALPHA;
			break;
		case 2: 
			colorType = PNG_COLOR_TYPE_RGB;
			break;
		case 3: 
			colorType = PNG_COLOR_TYPE_RGB_ALPHA;
			break;
		default:
			colorType = PNG_COLOR_TYPE_RGB;
	}


    png_set_IHDR(
        png, info,
		xPixelCount, yPixelCount, ubit ? 16 : 8,
        colorType, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png, info);

	// swap order of samples if on little endian machines
	// png expects network byte order
	if (ubit && osIsLittleEndian())
       png_set_swap(png);

	// write png file row by row
	if (ubit) {
		// 16 bit per channel
		std::vector<png_uint_16> row(xPixelCount*4);

		for (u_int y = 0; y < yPixelCount; ++y)
		{
			int i = 0;

			switch (colorType) {
				case PNG_COLOR_TYPE_GRAY:
					{
						for (u_int x = 0; x < xPixelCount; ++x)
						{
							png_uint_16 gray = static_cast<png_uint_16>(Clamp(65535.f * 
								pixels[x + y * xPixelCount].Y(), 0.f, 65535.f));

							row[i++] = gray;
						}
					}
					break;
				case PNG_COLOR_TYPE_GRAY_ALPHA:
					{
						for (u_int x = 0; x < xPixelCount; ++x)
						{
							png_uint_16 gray = static_cast<png_uint_16>(Clamp(65535.f * 
								pixels[x + y * xPixelCount].Y(), 0.f, 65535.f));
							png_uint_16 a = static_cast<png_uint_16>(Clamp(65535.f * alpha[x + y * xPixelCount], 0.f, 65535.f));

							row[i++] = gray;
							row[i++] = a;
						}
					}
					break;
				case PNG_COLOR_TYPE_RGB:
					{
						for (u_int x = 0; x < xPixelCount; ++x)
						{
							const RGBColor &c = pixels[(x + y * xPixelCount)];
							png_uint_16 r = static_cast<png_uint_16>(Clamp(65535.f * c.c[0], 0.f, 65535.f));
							png_uint_16 g = static_cast<png_uint_16>(Clamp(65535.f * c.c[1], 0.f, 65535.f));
							png_uint_16 b = static_cast<png_uint_16>(Clamp(65535.f * c.c[2], 0.f, 65535.f));

							row[i++] = r;
							row[i++] = g;
							row[i++] = b;
						}
					}
					break;
				case PNG_COLOR_TYPE_RGB_ALPHA:
					{
						for (u_int x = 0; x < xPixelCount; ++x)
						{
							const RGBColor &c = pixels[(x + y * xPixelCount)];
							png_uint_16 r = static_cast<png_uint_16>(Clamp(65535.f * c.c[0], 0.f, 65535.f));
							png_uint_16 g = static_cast<png_uint_16>(Clamp(65535.f * c.c[1], 0.f, 65535.f));
							png_uint_16 b = static_cast<png_uint_16>(Clamp(65535.f * c.c[2], 0.f, 65535.f));
							png_uint_16 a = static_cast<png_uint_16>(Clamp(65535.f * alpha[x + y * xPixelCount], 0.f, 65535.f));

							row[i++] = r;
							row[i++] = g;
							row[i++] = b;
							row[i++] = a;
						}
					}
					break;
			}
			
			png_write_row(png, reinterpret_cast<png_bytep>(&row[0]));
		}
	} else {
		// 8 bit per channel
		std::vector<png_byte> row(xPixelCount*4);

		for (u_int y = 0; y < yPixelCount; ++y)
		{
			int i = 0;

			switch (colorType) {
				case PNG_COLOR_TYPE_GRAY:
					{
						for (u_int x = 0; x < xPixelCount; ++x)
						{
							png_byte gray = static_cast<png_byte>(Clamp(255.f * 
								pixels[x + y * xPixelCount].Y(), 0.f, 255.f));

							row[i++] = gray;
						}
					}
					break;
				case PNG_COLOR_TYPE_GRAY_ALPHA:
					{
						for (u_int x = 0; x < xPixelCount; ++x)
						{
							png_byte gray = static_cast<png_byte>(Clamp(255.f * 
								pixels[x + y * xPixelCount].Y(), 0.f, 255.f));
							png_byte a = static_cast<png_byte>(Clamp(255.f * alpha[x + y * xPixelCount], 0.f, 255.f));

							row[i++] = gray;
							row[i++] = a;
						}
					}
					break;
				case PNG_COLOR_TYPE_RGB:
					{
						for (u_int x = 0; x < xPixelCount; ++x)
						{
							const RGBColor &c = pixels[(x + y * xPixelCount)];
							png_byte r = static_cast<png_byte>(Clamp(255.f * c.c[0], 0.f, 255.f));
							png_byte g = static_cast<png_byte>(Clamp(255.f * c.c[1], 0.f, 255.f));
							png_byte b = static_cast<png_byte>(Clamp(255.f * c.c[2], 0.f, 255.f));

							row[i++] = r;
							row[i++] = g;
							row[i++] = b;
						}
					}
					break;
				case PNG_COLOR_TYPE_RGB_ALPHA:
					{
						for (u_int x = 0; x < xPixelCount; ++x)
						{
							const RGBColor &c = pixels[(x + y * xPixelCount)];
							png_byte r = static_cast<png_byte>(Clamp(255.f * c.c[0], 0.f, 255.f));
							png_byte g = static_cast<png_byte>(Clamp(255.f * c.c[1], 0.f, 255.f));
							png_byte b = static_cast<png_byte>(Clamp(255.f * c.c[2], 0.f, 255.f));
							png_byte a = static_cast<png_byte>(Clamp(255.f * alpha[x + y * xPixelCount], 0.f, 255.f));

							row[i++] = r;
							row[i++] = g;
							row[i++] = b;
							row[i++] = a;
						}
					}
					break;			
			}
			
			png_write_row(png, &row[0]);
		}
	}

	// cleanup
	png_write_end(png, info);
	png_destroy_write_struct(&png, &info);

    fclose(fp);

}

}
