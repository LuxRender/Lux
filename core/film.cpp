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

// film.cpp*
#include "film.h"
#include "dynload.h"
#include "paramset.h"
#include "tonemap.h"
#include "stats.h"

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
#if cimg_OS!=2
#include <pthread.h>
#endif


namespace lux
{

	// Image Pipeline Function Definitions
	void ApplyImagingPipeline(vector<Color> &pixels,
		int xResolution, int yResolution, GREYCStorationParams &GREYCParams, ColorSystem &colorSpace,
		bool &haveBloomImage, Color *&bloomImage, bool bloomUpdate, float bloomRadius, float bloomWeight,
		bool VignettingEnabled, float VignetScale,
		const char *toneMapName, const ParamSet *toneMapParams,
		float gamma, float dither)
	{
		const int nPix = xResolution * yResolution ;
		// Possibly apply bloom effect to image
		if (bloomRadius > 0.f && bloomWeight > 0.f) {
			if(bloomUpdate) {
				// Compute image-space extent of bloom effect
				const int bloomSupport = Float2Int(bloomRadius *
					max(xResolution, yResolution));
				const int bloomWidth = bloomSupport / 2;
				// Initialize bloom filter table
				vector<float> bloomFilter(bloomWidth * bloomWidth);
				for (int i = 0; i < bloomWidth * bloomWidth; ++i) {
					float dist = sqrtf(float(i)) / float(bloomWidth);
					bloomFilter[i] = powf(max(0.f, 1.f - dist), 4.f);
				}

				// Allocate persisting bloom image layer if unallocated
				if(!haveBloomImage) {
					bloomImage = new Color[nPix];
					haveBloomImage = true;
				}

				// Apply bloom filter to image pixels
				//			vector<Color> bloomImage(nPix);
				ProgressReporter prog(yResolution, "Bloom filter"); //NOBOOK
				for (int y = 0; y < yResolution; ++y) {
					for (int x = 0; x < xResolution; ++x) {
						// Compute bloom for pixel _(x,y)_
						// Compute extent of pixels contributing bloom
						int x0 = max(0, x - bloomWidth);
						int x1 = min(x + bloomWidth, xResolution - 1);
						int y0 = max(0, y - bloomWidth);
						int y1 = min(y + bloomWidth, yResolution - 1);
						int offset = y * xResolution + x;
						float sumWt = 0.;
						for (int by = y0; by <= y1; ++by) {
							for (int bx = x0; bx <= x1; ++bx) {
								// Accumulate bloom from pixel $(bx,by)$
								int dx = x - bx, dy = y - by;
								if (dx == 0 && dy == 0) continue;
								int dist2 = dx*dx + dy*dy;
								if (dist2 < bloomWidth * bloomWidth) {
									int bloomOffset = bx + by * xResolution;
									float wt = bloomFilter[dist2];
									sumWt += wt;
									bloomImage[offset].AddWeighted(wt, pixels[bloomOffset]);
								}
							}
						}
						bloomImage[offset] /= sumWt;
					}
					prog.Update(); //NOBOOK
				}
				prog.Done(); //NOBOOK
			}

			// Mix bloom effect into each pixel
			if(haveBloomImage && bloomImage != NULL)
				for (int i = 0; i < nPix; ++i)
					pixels[i] = Lerp(bloomWeight, pixels[i], bloomImage[i]);
		}
		// Apply tone reproduction to image
		if (toneMapName) {
			ToneMap *toneMap = MakeToneMap(toneMapName,
				toneMapParams ? *toneMapParams : ParamSet());
			if (toneMap)
				toneMap->Map(pixels, xResolution, yResolution, 100.f);
			delete toneMap;
		}
		// Convert to RGB
		const float invGamma = 1.f / gamma;
		for (int i = 0; i < nPix; ++i) {
			pixels[i] = colorSpace.ToRGBConstrained(XYZColor(pixels[i].c));
			// Do gamma correction
			pixels[i] = pixels[i].Pow(invGamma);
		}

		// Add vignetting & chromatic abberation effect
		// These are paired in 1 loop as they can share quite a few calculations

		if(VignettingEnabled && VignetScale != 0.0f) {
			//for each pixel in the source image
			for(int y=0; y<yResolution; ++y)
				for(int x=0; x<xResolution; ++x)
				{
					const float nPx = (float)x/xResolution;
					const float nPy = (float)y/yResolution;
					const float xOffset = nPx - 0.5f;
					const float yOffset = nPy - 0.5f;
					float tOffset = sqrtf(xOffset*xOffset + yOffset*yOffset);

					// normalize to range [0.f - 1.f]
					const float invNtOffset = 1.f - (fabsf(tOffset) * 1.42);

					// Vignetting
					for(int i=0;i<3;i++)
						pixels[xResolution*y + x].c[i] *= invNtOffset * VignetScale;
				}
		}

		/*
			const Vec2 normed_pos(x/(float)out.getWidth(), y/(float)out.getHeight());
			const Vec2 offset = normed_pos - Vec2(0.5f, 0.5f);
			const float offset_term = offset.length();
			const Vec2 rb_pos = Vec2(0.5f, 0.5f) + offset * (1.f + offset_term*amount);
			const Vec2 g_pos = Vec2(0.5f, 0.5f) + offset * (1.f - offset_term*amount);

			out.getPixel(x, y) += Colour3(1.f, 0.f, 1.f) * bilinearSampleImage(in, Vec2(rb_pos.x*(float)out.getWidth(), rb_pos.y*(float)out.getHeight()));
			out.getPixel(x, y) += Colour3(0.f, 1.f, 0.f) * bilinearSampleImage(in, Vec2(g_pos.x*(float)out.getWidth(), g_pos.y*(float)out.getHeight()));
*/

		// remove / automate
		int chiu_radius = 0;
		bool chiu_includecenter = true;

		// Apply Chiu Noise Reduction Filter
		if(chiu_radius > 0) {
			std::vector<Color> chiuImage(nPix);
			for(int i=0; i<nPix; i++)
				chiuImage[i] *= 0.f;

			const int pixel_rad = (int)chiu_radius;
			const int lookup_size = pixel_rad + pixel_rad + 1;

			//build filter lookup table
			std::vector<float> weights(lookup_size * lookup_size);

			for(int y=0; y<lookup_size; ++y)
				for(int x=0; x<lookup_size; ++x)
				{
					if(x == pixel_rad && y == pixel_rad)
					{
						weights[lookup_size*y + x] = chiu_includecenter ? 1.0f : 0.0f;
					}
					else
					{
						const float dx = (float)(x - pixel_rad);
						const float dy = (float)(y - pixel_rad);
						const float dist = sqrt(dx*dx + dy*dy);
						const float weight = pow(max(0.0f, 1.0f - dist / chiu_radius), 4.0f);
						weights[lookup_size*y + x] = weight;
					}
				}

				float sumweight = 0.0f;
				for(int y=0; y<lookup_size; ++y)
					for(int x=0; x<lookup_size; ++x)
						sumweight += weights[lookup_size*y + x];

				//normalise filter kernel
				for(int y=0; y<lookup_size; ++y)
					for(int x=0; x<lookup_size; ++x)
						weights[lookup_size*y + x] /= sumweight;

				//for each pixel in the source image
				for(int y=0; y<yResolution; ++y)
				{
					//get min and max of current filter rect along y axis
					const int miny = max(0, y - pixel_rad);
					const int maxy = min(yResolution, y + pixel_rad + 1);

					for(int x=0; x<xResolution; ++x)
					{
						//get min and max of current filter rect along x axis
						const int minx = max(0, x - pixel_rad);
						const int maxx = min(xResolution, x + pixel_rad + 1);

						// For each pixel in the out image, in the filter radius
						for(int ty=miny; ty<maxy; ++ty)
							for(int tx=minx; tx<maxx; ++tx)
							{
								const int dx=x-tx+pixel_rad;
								const int dy=y-ty+pixel_rad;
								const float factor = weights[lookup_size*dy + dx];
								chiuImage[xResolution*ty + tx].AddWeighted(factor, pixels[xResolution*y + x]);
							}
					}
				}
				// Copyback
				for(int i=0; i<nPix; i++)
					pixels[i] = chiuImage[i];

				// remove used intermediate memory
				chiuImage.clear();
				weights.clear();
		}

		// Apply GREYCStoration noise reduction filter
		if(GREYCParams.enabled) {
			// Define Cimg image buffer and copy data
			CImg<float> img(xResolution, yResolution, 1, 3);
			for(int y=0; y<yResolution; y++) {
				for(int x=0; x<xResolution; x++) {
					int index = xResolution * y + x;
					// Note - Cimg float data must be in range [0,255] for GREYCStoration to work %100
					for(int j=0; j<3; j++)
						img(x, y, 0, j) = pixels[index].c[j] * 255;
				}
			}

			static const CImg<unsigned char> empty_mask;

			img.blur_anisotropic(empty_mask, GREYCParams.amplitude,
				GREYCParams.sharpness,
				GREYCParams.anisotropy,
				GREYCParams.alpha,
				GREYCParams.sigma,
				GREYCParams.dl,
				GREYCParams.da,
				GREYCParams.gauss_prec,
				GREYCParams.interp,
				GREYCParams.fast_approx,
				1.0f); // gfact? 1.0f.

			/*			// Begin iteration loop
			for (unsigned int iter=0; iter<GREYCParams.nb_iter; iter++) {
			// This function will start a thread running one iteration of the GREYCstoration filter.
			// It returns immediately, so you can do what you want after (update a progress bar for instance).
			img.greycstoration_run(GREYCParams.amplitude,
			GREYCParams.sharpness,
			GREYCParams.anisotropy,
			GREYCParams.alpha,
			GREYCParams.sigma,
			1.0f,
			GREYCParams.dl,
			GREYCParams.da,
			GREYCParams.gauss_prec,
			GREYCParams.interp,
			GREYCParams.fast_approx,
			GREYCParams.tile,
			GREYCParams.btile,
			GREYCParams.threads);

			// Here, we print the overall progress percentage.
			do {
			// pr_iteration is the progress percentage for the current iteration
			const float pr_iteration = img.greycstoration_progress();
			// This simply computes the global progression indice (including all iterations)
			const unsigned int pr_global = (unsigned int)((iter*100 + pr_iteration)/GREYCParams.nb_iter);
			// Wait a little bit
			cimg::wait(100);
			// Interrupt example
			// img.greycstoration_stop();
			} while (img.greycstoration_is_running());
			}
			*/
			// Copy data from cimg buffer back to pixels vector
			const float inv_byte = 1.f/255;
			for(int y=0; y<yResolution; y++) {
				for(int x=0; x<xResolution; x++) {
					int index = xResolution * y + x;
					for(int j=0; j<3; j++)
						pixels[index].c[j] = img(x, y, 0, j) * inv_byte;
				}
			}

			// remove used intermediate cimg buffer
			//img.~CImg();
		}

		// Dither image
		if (dither > 0.f)
			for (int i = 0; i < nPix; ++i)
				pixels[i] += 2.f * dither * (lux::random::floatValueP() - .5f);
	}

}//namespace lux

