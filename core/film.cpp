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
#include "randomgen.h"
#include "dynload.h"
#include "paramset.h"
#include "tonemap.h"
#include "cameraresponse.h"
#include "filter.h"
#include "contribution.h"
#include "stats.h"
#include "blackbodyspd.h"
#include "osfunc.h"

#include <iostream>
#include <fstream>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
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
#if cimg_OS!=2
#include <pthread.h>
#endif

using namespace boost::iostreams;
using namespace lux;

namespace lux
{

template<class T> T bilinearSampleImage(const vector<T> &pixels,
	const u_int xResolution, const u_int yResolution, 
	const float x, const float y)
{
	u_int x1 = Clamp(Floor2UInt(x), 0U, xResolution - 1);
	u_int y1 = Clamp(Floor2UInt(y), 0U, yResolution - 1);
	u_int x2 = Clamp(x1 + 1, 0U, xResolution - 1);
	u_int y2 = Clamp(y1 + 1, 0U, yResolution - 1);
	float tx = Clamp(x - static_cast<float>(x1), 0.f, 1.f);
	float ty = Clamp(y - static_cast<float>(y1), 0.f, 1.f);

	T c(0.f);
	c.AddWeighted((1.f - tx) * (1.f - ty), pixels[y1 * xResolution + x1]);
	c.AddWeighted(tx         * (1.f - ty), pixels[y1 * xResolution + x2]);
	c.AddWeighted((1.f - tx) * ty,         pixels[y2 * xResolution + x1]);
	c.AddWeighted(tx         * ty,         pixels[y2 * xResolution + x2]);

	return c;
}

// horizontal blur
void horizontalGaussianBlur(const vector<XYZColor> &in, vector<XYZColor> &out,
	const u_int xResolution, const u_int yResolution, float std_dev)
{
	u_int rad_needed = Ceil2UInt(std_dev * 4.f);//kernel_radius;

	const u_int lookup_size = rad_needed + 1;
	//build filter lookup table
	std::vector<float> filter_weights(lookup_size);
	float sweight = 0.f;
	for(u_int x = 0; x < lookup_size; ++x) {
		filter_weights[x] = expf(-static_cast<float>(x) * static_cast<float>(x) / (std_dev * std_dev));
		if (x > 0) {
			if (filter_weights[x] < 1e-12f) {
				rad_needed = x;
				break;
			}
			sweight += 2.f * filter_weights[x];
		} else
			sweight += filter_weights[x];
	}

	//normalise filter kernel
	sweight = 1.f / sweight;

	const u_int pixel_rad = rad_needed;

	//------------------------------------------------------------------
	//blur in x direction
	//------------------------------------------------------------------
	for(u_int y = 0; y < yResolution; ++y) {
		for(u_int x = 0; x < xResolution; ++x) {
			const u_int a = y * xResolution + x;

			out[a] = XYZColor(0.f);

			for (u_int i = max(x, pixel_rad) - pixel_rad; i <= min(x + pixel_rad, xResolution - 1); ++i) {
				if (i < x)
					out[a].AddWeighted(filter_weights[x - i], in[a + i - x]);
				else
					out[a].AddWeighted(filter_weights[i - x], in[a + i - x]);
			}
		}
	}
}

void rotateImage(const vector<XYZColor> &in, vector<XYZColor> &out,
	const u_int xResolution, const u_int yResolution, float angle)
{
	const u_int maxRes = max(xResolution, yResolution);

	const float s = sinf(-angle);
	const float c = cosf(-angle);

	const float cx = xResolution * 0.5f;
	const float cy = yResolution * 0.5f;

	for(u_int y = 0; y < maxRes; ++y) {
		float px = 0.f - maxRes * 0.5f;
		float py = y - maxRes * 0.5f;

		float rx = px * c - py * s + cx;
		float ry = px * s + py * c + cy;
		for(u_int x = 0; x < maxRes; ++x) {
			out[y*maxRes + x] = bilinearSampleImage<XYZColor>(in, xResolution, yResolution, rx, ry);
			// x = x + dx
			rx += c;
			ry += s;
		}
	}
}


// Image Pipeline Function Definitions
void ApplyImagingPipeline(vector<XYZColor> &xyzpixels,
	u_int xResolution, u_int yResolution,
	const GREYCStorationParams &GREYCParams, const ChiuParams &chiuParams,
	const ColorSystem &colorSpace, Histogram *histogram, bool HistogramEnabled,
	bool &haveBloomImage, XYZColor *&bloomImage, bool bloomUpdate,
	float bloomRadius, float bloomWeight,
	bool VignettingEnabled, float VignetScale,
	bool aberrationEnabled, float aberrationAmount,
	bool &haveGlareImage, XYZColor *&glareImage, bool glareUpdate,
	float glareAmount, float glareRadius, u_int glareBlades, float glareThreshold,
	const char *toneMapName, const ParamSet *toneMapParams,
	const CameraResponse *response, float gamma, float dither)
{
	const u_int nPix = xResolution * yResolution;

	// Clamp input
	for (u_int i = 0; i < nPix; ++i)
		xyzpixels[i] = xyzpixels[i].Clamp();


	// Possibly apply bloom effect to image
	if (bloomRadius > 0.f && bloomWeight > 0.f) {
		if(bloomUpdate) {
			// Compute image-space extent of bloom effect
			const u_int bloomSupport = Float2UInt(bloomRadius *
				max(xResolution, yResolution));
			const u_int bloomWidth = bloomSupport / 2;
			// Initialize bloom filter table
			vector<float> bloomFilter(bloomWidth * bloomWidth);
			for (u_int i = 0; i < bloomWidth * bloomWidth; ++i) {
				float dist = sqrtf(i) / bloomWidth;
				bloomFilter[i] = powf(max(0.f, 1.f - dist), 4.f);
			}

			// Allocate persisting bloom image layer if unallocated
			if(!haveBloomImage) {
				bloomImage = new XYZColor[nPix];
				haveBloomImage = true;
			}

			// Apply bloom filter to image pixels
			//			vector<Color> bloomImage(nPix);
//			ProgressReporter prog(yResolution, "Bloom filter"); //NOBOOK //intermediate crashfix until imagepipelinerefactor is done - Jens
			for (u_int y = 0; y < yResolution; ++y) {
				for (u_int x = 0; x < xResolution; ++x) {
					// Compute bloom for pixel _(x,y)_
					// Compute extent of pixels contributing bloom
					const u_int x0 = max(x, bloomWidth) - bloomWidth;
					const u_int x1 = min(x + bloomWidth, xResolution - 1);
					const u_int y0 = max(y, bloomWidth) - bloomWidth;
					const u_int y1 = min(y + bloomWidth, yResolution - 1);
					const u_int offset = y * xResolution + x;
					float sumWt = 0.f;
					for (u_int by = y0; by <= y1; ++by) {
						for (u_int bx = x0; bx <= x1; ++bx) {
							if (bx == x && by == y)
								continue;
							// Accumulate bloom from pixel $(bx,by)$
							const u_int dist2 = (x - bx) * (x - bx) + (y - by) * (y - by);
							if (dist2 < bloomWidth * bloomWidth) {
								u_int bloomOffset = bx + by * xResolution;
								float wt = bloomFilter[dist2];
								sumWt += wt;
								bloomImage[offset].AddWeighted(wt, xyzpixels[bloomOffset]);
							}
						}
					}
					bloomImage[offset] /= sumWt;
				}
//				prog.Update(); //NOBOOK //intermediate crashfix until imagepipelinerefactor is done - Jens
			}
//			prog.Done(); //NOBOOK //intermediate crashfix until imagepipelinerefactor is done - Jens
		}

		// Mix bloom effect into each pixel
		if(haveBloomImage && bloomImage != NULL)
			for (u_int i = 0; i < nPix; ++i)
				xyzpixels[i] = Lerp(bloomWeight, xyzpixels[i], bloomImage[i]);
	}

	if (glareRadius > 0.f && glareAmount > 0.f) {
		if (glareUpdate) {
			// Allocate persisting glare image layer if unallocated
			if(!haveGlareImage) {
				glareImage = new XYZColor[nPix];
				haveGlareImage = true;
			}

			u_int maxRes = max(xResolution, yResolution);
			u_int nPix2 = maxRes * maxRes;

			std::vector<XYZColor> rotatedImage(nPix2);
			std::vector<XYZColor> blurredImage(nPix2);
			std::vector<XYZColor> darkenedImage(nPix);
			for(u_int i = 0; i < nPix; ++i)
				glareImage[i] = XYZColor(0.f);
			
			// Search for the brightest pixel in the image
			u_int max = 0;
			for(u_int i = 1; i < nPix; ++i) {
				if (xyzpixels[i].c[1] > xyzpixels[max].c[1])
					max = i;
			}
			
			// glareThreshold ranges between 0-1,
			// but this relative value has to be converted to
			//an absolute value fitting the image being processed
			float glareAbsoluteThreshold = xyzpixels[max].c[1] *
				glareThreshold;
			// Every pixel that is not bright enough is made black
			for(u_int i = 0; i < nPix; ++i) {
				if(xyzpixels[i].c[1] < glareAbsoluteThreshold)
					darkenedImage[i] = XYZColor(0.f);
				else
					darkenedImage[i] = xyzpixels[i];
			}

			const float radius = maxRes * glareRadius;

			// add rotated versions of the blurred image
			const float invBlades = 1.f / glareBlades;
			float angle = 0.f;
			for (u_int i = 0; i < glareBlades; ++i) {
				rotateImage(darkenedImage, rotatedImage, xResolution, yResolution, angle);
				horizontalGaussianBlur(rotatedImage, blurredImage, maxRes, maxRes, radius);
				rotateImage(blurredImage, rotatedImage, maxRes, maxRes, -angle);

				// add to output
				for(u_int y = 0; y < yResolution; ++y) {
					for(u_int x = 0; x < xResolution; ++x) {
						const u_int sx = x + (maxRes - xResolution) / 2;
						const u_int sy = y + (maxRes - yResolution) / 2;

						glareImage[y * xResolution + x] += rotatedImage[sy * maxRes + sx];
					}
				}
				angle += 2.f * M_PI * invBlades;
			}

			// normalize
			for(u_int i = 0; i < nPix; ++i)
				glareImage[i] *= invBlades;

			rotatedImage.clear();
			blurredImage.clear();
			darkenedImage.clear();
		}

		if (haveGlareImage && glareImage != NULL) {
			for(u_int i = 0; i < nPix; ++i) {
				xyzpixels[i] += glareAmount * glareImage[i];
			}
		}
	}

	// Apply tone reproduction to image
	if (toneMapName) {
		ToneMap *toneMap = MakeToneMap(toneMapName,
			toneMapParams ? *toneMapParams : ParamSet());
		if (toneMap)
			toneMap->Map(xyzpixels, xResolution, yResolution, 100.f);
		delete toneMap;
	}

	// Convert to RGB
	vector<RGBColor> &rgbpixels = reinterpret_cast<vector<RGBColor> &>(xyzpixels);
	for (u_int i = 0; i < nPix; ++i)
		rgbpixels[i] = colorSpace.ToRGBConstrained(xyzpixels[i]);

	// DO NOT USE xyzpixels ANYMORE AFTER THIS POINT
	if (response && response->validFile) {
		for (u_int i = 0; i < nPix; ++i)
			response->Map(rgbpixels[i]);
	}

	// Add vignetting & chromatic aberration effect
	// These are paired in 1 loop as they can share quite a few calculations
	if ((VignettingEnabled && VignetScale != 0.0f) ||
		(aberrationEnabled && aberrationAmount > 0.f)) {

		RGBColor *outp = &rgbpixels[0];
		std::vector<RGBColor> aberrationImage;
		if (aberrationEnabled) {
			aberrationImage.resize(nPix, RGBColor(0.f));
			outp = &aberrationImage[0];
		}

		const float invxRes = 1.f / xResolution;
		const float invyRes = 1.f / yResolution;
		//for each pixel in the source image
		for(u_int y = 0; y < yResolution; ++y) {
			for(u_int x = 0; x < xResolution; ++x) {
				const float nPx = x * invxRes;
				const float nPy = y * invyRes;
				const float xOffset = nPx - 0.5f;
				const float yOffset = nPy - 0.5f;
				const float tOffset = sqrtf(xOffset * xOffset + yOffset * yOffset);
					
				if (aberrationEnabled && aberrationAmount > 0.f) {
					const float rb_x = (0.5f + xOffset * (1.f + tOffset * aberrationAmount)) * xResolution;
					const float rb_y = (0.5f + yOffset * (1.f + tOffset * aberrationAmount)) * yResolution;
					const float g_x =  (0.5f + xOffset * (1.f - tOffset * aberrationAmount)) * xResolution;
					const float g_y =  (0.5f + yOffset * (1.f - tOffset * aberrationAmount)) * yResolution;

					const float redblue[] = {1.f, 0.f, 1.f};
					const float green[] = {0.f, 1.f, 0.f};

					outp[xResolution * y + x] += RGBColor(redblue) * bilinearSampleImage<RGBColor>(rgbpixels, xResolution, yResolution, rb_x, rb_y);
					outp[xResolution * y + x] += RGBColor(green) * bilinearSampleImage<RGBColor>(rgbpixels, xResolution, yResolution, g_x, g_y);
				}

				// Vignetting
				if(VignettingEnabled && VignetScale != 0.0f) {
					// normalize to range [0.f - 1.f]
					const float invNtOffset = 1.f - (fabsf(tOffset) * 1.42f);
					float vWeight = Lerp(invNtOffset, 1.f - VignetScale, 1.f);
					for (u_int i = 0; i < 3; ++i)
						outp[xResolution*y + x].c[i] *= vWeight;
				}
			}
		}

		if (aberrationEnabled) {
			for(u_int i = 0; i < nPix; ++i)
				rgbpixels[i] = aberrationImage[i];
		}

		aberrationImage.clear();
	}

	// Calculate histogram (if it is enabled and exists)
	if (HistogramEnabled && histogram)
		histogram->Calculate(rgbpixels, xResolution, yResolution);

	// Apply Chiu Noise Reduction Filter
	if(chiuParams.enabled) {
		std::vector<RGBColor> chiuImage(nPix, RGBColor(0.f));

		// NOTE - lordcrc - if includecenter is false, make sure radius 
		// is a tad higher than 1 to include other pixels
		const float radius = max(chiuParams.radius, 1.f + (chiuParams.includecenter ? 0.f : 1e-6f));

		const u_int pixel_rad = Ceil2UInt(radius);
		const u_int lookup_size = 2 * pixel_rad + 1;

		//build filter lookup table
		std::vector<float> weights(lookup_size * lookup_size);

		float sumweight = 0.f;
		u_int offset = 0;
		for(int y = -static_cast<int>(pixel_rad); y <= static_cast<int>(pixel_rad); ++y) {
			for(int x = -static_cast<int>(pixel_rad); x <= static_cast<int>(pixel_rad); ++x) {
				if(x == 0 && y == 0)
					weights[offset] = chiuParams.includecenter ? 1.0f : 0.0f;
				else {
					const float dx = x;
					const float dy = y;
					const float dist = sqrtf(dx * dx + dy * dy);
					const float weight = powf(max(0.0f, 1.0f - dist / radius), 4.0f);
					weights[offset] = weight;
				}
				sumweight += weights[offset++];
			}
		}

		//normalise filter kernel
		for(u_int y = 0; y < lookup_size; ++y)
			for(u_int x = 0; x < lookup_size; ++x)
				weights[lookup_size*y + x] /= sumweight;

		//for each pixel in the source image
		for (u_int y = 0; y < yResolution; ++y) {
			//get min and max of current filter rect along y axis
			const u_int miny = max(y, pixel_rad) - pixel_rad;
			const u_int maxy = min(yResolution - 1, y + pixel_rad);

			for (u_int x = 0; x < xResolution; ++x) {
				//get min and max of current filter rect along x axis
				const u_int minx = max(x, pixel_rad) - pixel_rad;
				const u_int maxx = min(xResolution - 1, x + pixel_rad);

				// For each pixel in the out image, in the filter radius
				for(u_int ty = miny; ty < maxy; ++ty) {
					for(u_int tx = minx; tx < maxx; ++tx) {
						const u_int dx = x - tx + pixel_rad;
						const u_int dy = y - ty + pixel_rad;
						const float factor = weights[lookup_size*dy + dx];
						chiuImage[xResolution*ty + tx].AddWeighted(factor, rgbpixels[xResolution*y + x]);
					}
				}
			}
		}
		// Copyback
		for(u_int i = 0; i < nPix; ++i)
			rgbpixels[i] = chiuImage[i];

		// remove used intermediate memory
		chiuImage.clear();
		weights.clear();
	}

	// Apply GREYCStoration noise reduction filter
	if(GREYCParams.enabled) {
		// Define Cimg image buffer and copy data
		CImg<float> img(xResolution, yResolution, 1, 3);
		for(u_int y = 0; y < yResolution; ++y) {
			for(u_int x = 0; x < xResolution; ++x) {
				const u_int index = xResolution * y + x;
				// Note - Cimg float data must be in range [0,255] for GREYCStoration to work %100
				for(u_int j = 0; j < 3; ++j)
					img(x, y, 0, j) = rgbpixels[index].c[j] * 255;
			}
		}

		for (unsigned int iter=0; iter<GREYCParams.nb_iter; iter++) {
			img.blur_anisotropic(GREYCParams.amplitude,
				GREYCParams.sharpness,
				GREYCParams.anisotropy,
				GREYCParams.alpha,
				GREYCParams.sigma,
				GREYCParams.dl,
				GREYCParams.da,
				GREYCParams.gauss_prec,
				GREYCParams.interp,
				GREYCParams.fast_approx,
				1.0f); // gfact
		}

		// Copy data from cimg buffer back to pixels vector
		const float inv_byte = 1.f/255;
		for(u_int y = 0; y < yResolution; ++y) {
			for(u_int x = 0; x < xResolution; ++x) {
				const u_int index = xResolution * y + x;
				for(u_int j = 0; j < 3; ++j)
					rgbpixels[index].c[j] = img(x, y, 0, j) * inv_byte;
			}
		}
	}

	// Dither image
	if (dither > 0.f)
		for (u_int i = 0; i < nPix; ++i)
			rgbpixels[i] += 2.f * dither * (lux::random::floatValueP() - .5f);

	// Do gamma correction
	const float invGamma = 1.f / gamma;
	for (u_int i = 0; i < nPix; ++i)
		rgbpixels[i] = rgbpixels[i].Pow(invGamma);
}


// Filter Look Up Table Definitions

FilterLUT::FilterLUT(Filter *filter, const float offsetX, const float offsetY) {
	const int x0 = Ceil2Int(offsetX - filter->xWidth);
	const int x1 = Floor2Int(offsetX + filter->xWidth);
	const int y0 = Ceil2Int(offsetY - filter->yWidth);
	const int y1 = Floor2Int(offsetY + filter->yWidth);
	lutWidth = x1 - x0 + 1;
	lutHeight = y1 - y0 + 1;
	//lut = new float[lutWidth * lutHeight];
	lut.resize(lutWidth * lutHeight);

	float totalWeight = 0.f;
	unsigned int index = 0;
	for (int iy = y0; iy <= y1; ++iy) {
		for (int ix = x0; ix <= x1; ++ix) {
			const float filterVal = filter->Evaluate(fabsf(ix - offsetX), fabsf(iy - offsetY));
			totalWeight += filterVal;
			lut[index++] = filterVal;
		}
	}

	// Normalize LUT
	index = 0;
	for (int iy = y0; iy <= y1; ++iy) {
		for (int ix = x0; ix <= x1; ++ix)
			lut[index++] /= totalWeight;
	}
}

FilterLUTs::FilterLUTs(Filter *filter, const unsigned int size) {		
	lutsSize = size + 1;
	step = 1.f / float(size);

	luts.resize(lutsSize * lutsSize);

	for (unsigned int iy = 0; iy < lutsSize; ++iy) {
		for (unsigned int ix = 0; ix < lutsSize; ++ix) {
			const float x = ix * step - 0.5f + step / 2.f;
			const float y = iy * step - 0.5f + step / 2.f;

			luts[ix + iy * lutsSize] = FilterLUT(filter, x, y);
		}
	}
}

// Film Function Definitions

u_int Film::GetXResolution()
{
	return xResolution;
}

u_int Film::GetYResolution()
{
	return yResolution;
}

Film::Film(u_int xres, u_int yres, Filter *filt, u_int filtRes, const float crop[4], 
		   const string &filename1, bool premult, bool useZbuffer,
		   bool w_resume_FLM, bool restart_resume_FLM, int haltspp, int halttime,
		   int reject_warmup, bool debugmode) :
	Queryable("film"),
	xResolution(xres), yResolution(yres),
	EV(0.f), averageLuminance(0.f),  numberOfSamplesFromNetwork(0), numberOfLocalSamples(0),
	filter(filt), filename(filename1),
	colorSpace(0.63f, 0.34f, 0.31f, 0.595f, 0.155f, 0.07f, 0.314275f, 0.329411f), // default is SMPTE
	ZBuffer(NULL), use_Zbuf(useZbuffer),
	debug_mode(debugmode), premultiplyAlpha(premult),
	warmupComplete(false), reject_warmup_samples(reject_warmup),
	writeResumeFlm(w_resume_FLM), restartResumeFlm(restart_resume_FLM),
	haltSamplesPerPixel(haltspp), haltTime(halttime), histogram(NULL), enoughSamplesPerPixel(false)
{
	// Compute film image extent
	memcpy(cropWindow, crop, 4 * sizeof(float));
	xPixelStart = Ceil2UInt(xResolution * cropWindow[0]);
	xPixelCount = max(1U, Ceil2UInt(xResolution * cropWindow[1]) - xPixelStart);
	yPixelStart = Ceil2UInt(yResolution * cropWindow[2]);
	yPixelCount = max(1U, Ceil2UInt(yResolution * cropWindow[3]) - yPixelStart);
	int xRealWidth = Floor2Int(xPixelStart + .5f + xPixelCount + filter->xWidth) - Floor2Int(xPixelStart + .5f - filter->xWidth);
	int yRealHeight = Floor2Int(yPixelStart + .5f + yPixelCount + filter->yWidth) - Floor2Int(yPixelStart + .5f - filter->yWidth);
	samplePerPass = xRealWidth * yRealHeight;

	// calculate reject warmup samples
	reject_warmup_samples = (static_cast<double>(xRealWidth) * static_cast<double>(yRealHeight)) * reject_warmup;

	maxY = debug_mode ? INFINITY : 0.f;
	warmupSamples = 0;
	warmupComplete = debug_mode;

	boost::xtime_get(&creationTime, boost::TIME_UTC);

	//Queryable parameters
	AddIntAttribute(*this, "xResolution", "Horizontal resolution (pixels)", &Film::GetXResolution);
	AddIntAttribute(*this, "yResolution", "Vertical resolution (pixels)", &Film::GetYResolution);
	AddStringAttribute(*this, "filename", "Output filename", filename, &Film::filename, Queryable::ReadWriteAccess);
	AddFloatAttribute(*this, "EV", "Exposure value", &Film::EV);
	AddFloatAttribute(*this, "averageLuminance", "Average Image Luminance", &Film::averageLuminance);
	AddDoubleAttribute(*this, "numberOfLocalSamples", "Number of samples contributed to film on the local machine", &Film::numberOfLocalSamples);
	AddDoubleAttribute(*this, "numberOfSamplesFromNetwork", "Number of samples contributed from network slaves", &Film::numberOfSamplesFromNetwork);
	AddBoolAttribute(*this, "enoughSamples", "Indicates if the halt condition been reached", &Film::enoughSamplesPerPixel);
	AddIntAttribute(*this, "haltSamplesPerPixel", "Halt Samples per Pixel", haltSamplesPerPixel, &Film::haltSamplesPerPixel, Queryable::ReadWriteAccess);
	AddIntAttribute(*this, "haltTime", "Halt time in seconds", haltTime, &Film::haltTime, Queryable::ReadWriteAccess);
	AddBoolAttribute(*this, "writeResumeFlm", "Write resume file", writeResumeFlm, &Film::writeResumeFlm, Queryable::ReadWriteAccess);
	AddBoolAttribute(*this, "restartResumeFlm", "Restart (overwrite) resume file", restartResumeFlm, &Film::restartResumeFlm, Queryable::ReadWriteAccess);

	// Precompute filter tables
	filterLUTs = new FilterLUTs(filt, max(min(filtRes, 64u), 2u));
}

Film::~Film()
{
	delete filterLUTs;
	delete filter;
	delete ZBuffer;
	delete histogram;
	delete contribPool;
}

void Film::RequestBufferGroups(const vector<string> &bg)
{
	for (u_int i = 0; i < bg.size(); ++i)
		bufferGroups.push_back(BufferGroup(bg[i]));
}

u_int Film::RequestBuffer(BufferType type, BufferOutputConfig output,
	const string& filePostfix)
{
	bufferConfigs.push_back(BufferConfig(type, output, filePostfix));
	return bufferConfigs.size() - 1;
}

void Film::CreateBuffers()
{
	if (bufferGroups.size() == 0)
		bufferGroups.push_back(BufferGroup("default"));
	for (u_int i = 0; i < bufferGroups.size(); ++i)
		bufferGroups[i].CreateBuffers(bufferConfigs,xPixelCount,yPixelCount);

	// Allocate ZBuf buffer if needed
	if(use_Zbuf)
		ZBuffer = new PerPixelNormalizedFloatBuffer(xPixelCount,yPixelCount);

    // Dade - check if we have to resume a rendering and restore the buffers
    if(writeResumeFlm) {
		const string fname = filename+".flm";
		if (restartResumeFlm) {
			const string oldfname = fname + "1";
			if (boost::filesystem::exists(fname)) {
				if (boost::filesystem::exists(oldfname))
					remove(oldfname.c_str());
				rename(fname.c_str(), oldfname.c_str());
			}
		} else {
			// Dade - check if the film file exists
			std::ifstream ifs(fname.c_str(), std::ios_base::in | std::ios_base::binary);

			if(ifs.good()) {
				// Dade - read the data
				LOG(LUX_INFO,LUX_NOERROR)<< "Reading film status from file " << fname;
				UpdateFilm(ifs);
			}

			ifs.close();
		}
    }

	// initialize the contribution pool
	contribPool = new ContributionPool(this);
}

void Film::SetGroupName(u_int index, const string& name) 
{
	if( index >= bufferGroups.size())
		return;
	bufferGroups[index].name = name;
}
string Film::GetGroupName(u_int index) const
{
	if (index >= bufferGroups.size())
		return "";
	return bufferGroups[index].name;
}
void Film::SetGroupEnable(u_int index, bool status)
{
	if (index >= bufferGroups.size())
		return;
	bufferGroups[index].enable = status;
}
bool Film::GetGroupEnable(u_int index) const
{
	if (index >= bufferGroups.size())
		return false;
	return bufferGroups[index].enable;
}
void Film::SetGroupScale(u_int index, float value)
{
	if (index >= bufferGroups.size())
		return;
	bufferGroups[index].globalScale = value;
	ComputeGroupScale(index);
}
float Film::GetGroupScale(u_int index) const
{
	if (index >= bufferGroups.size())
		return 0.f;
	return bufferGroups[index].globalScale;
}
void Film::SetGroupRGBScale(u_int index, const RGBColor &value)
{
	if (index >= bufferGroups.size())
		return;
	bufferGroups[index].rgbScale = value;
	ComputeGroupScale(index);
}
RGBColor Film::GetGroupRGBScale(u_int index) const
{
	if (index >= bufferGroups.size())
		return 0.f;
	return bufferGroups[index].rgbScale;
}
void Film::SetGroupTemperature(u_int index, float value)
{
	if (index >= bufferGroups.size())
		return;
	bufferGroups[index].temperature = value;
	ComputeGroupScale(index);
}
float Film::GetGroupTemperature(u_int index) const
{
	if (index >= bufferGroups.size())
		return 0.f;
	return bufferGroups[index].temperature;
}
void Film::ComputeGroupScale(u_int index)
{
	const XYZColor white(colorSpace.ToXYZ(RGBColor(1.f)));
	if (bufferGroups[index].temperature > 0.f) {
		XYZColor colorTemp(BlackbodySPD(bufferGroups[index].temperature));
		colorTemp /= colorTemp.Y();
		bufferGroups[index].convert = ColorAdaptator(white,
			colorSpace.ToXYZ(bufferGroups[index].rgbScale)) *
			ColorAdaptator(white, colorTemp);
	} else {
		bufferGroups[index].convert = ColorAdaptator(white,
			colorSpace.ToXYZ(bufferGroups[index].rgbScale));
	}
	bufferGroups[index].convert *= bufferGroups[index].globalScale;
}

void Film::GetSampleExtent(int *xstart, int *xend,
	int *ystart, int *yend) const
{
	*xstart = Floor2Int(xPixelStart + .5f - filter->xWidth);
	*xend   = Floor2Int(xPixelStart + .5f + xPixelCount + filter->xWidth);
	*ystart = Floor2Int(yPixelStart + .5f - filter->yWidth);
	*yend   = Floor2Int(yPixelStart + .5f + yPixelCount + filter->yWidth);
}

void Film::AddSampleCount(float count) {
	if (haltTime > 0) {
		// Check if we have met the enough rendering time condition
		boost::xtime t;
		boost::xtime_get(&t, boost::TIME_UTC);
		if (t.sec - creationTime.sec > haltTime)
			enoughSamplesPerPixel = true;
	}

	numberOfLocalSamples += count;

	for (u_int i = 0; i < bufferGroups.size(); ++i) {
		bufferGroups[i].numberOfSamples += count;

		// Dade - check if we have enough samples per pixel. The rendering stop
		// when one of the buffer groups has enough samples (at the moment all
		// buffer groups have always the same samples count; in the future
		// it could be better to stop when all buffer groups have enough samples)
		if ((haltSamplesPerPixel > 0) &&
			(bufferGroups[i].numberOfSamples  >= haltSamplesPerPixel * samplePerPass))
			enoughSamplesPerPixel = true;
	}
}

void Film::AddSample(Contribution *contrib) {
	XYZColor xyz = contrib->color;
	const float alpha = contrib->alpha;
	const float weight = contrib->variance;

	// Issue warning if unexpected radiance value returned
	if (!(xyz.Y() >= 0.f) || isinf(xyz.Y())) {
		if(debug_mode) {
			LOG(LUX_WARNING,LUX_LIMIT) << "Out of bound intensity in Film::AddSample: "
			   << xyz.Y() << ", sample discarded";
		}
		return;
	}

	if (!(alpha >= 0.f) || isinf(alpha)) {
		if(debug_mode) {
			LOG(LUX_WARNING,LUX_LIMIT) << "Out of bound  alpha in Film::AddSample: "
			   << alpha << ", sample discarded";
		}
		return;
	}

	if (!(weight >= 0.f) || isinf(weight)) {
		if(debug_mode) {
			LOG(LUX_WARNING,LUX_LIMIT) << "Out of bound  weight in Film::AddSample: "
			   << weight << ", sample discarded";
		}
		return;
	}

	// Reject samples higher than max Y() after warmup period
	if (warmupComplete) {
		if(xyz.Y() > maxY)
			return;
	} else {
	 	maxY = max(maxY, xyz.Y());
		++warmupSamples;
	 	if (warmupSamples >= reject_warmup_samples)
			warmupComplete = true;
	}

	if (premultiplyAlpha)
		xyz *= alpha;

	BufferGroup &currentGroup = bufferGroups[contrib->bufferGroup];
	Buffer *buffer = currentGroup.getBuffer(contrib->buffer);

	// Compute sample's raster extent
	float dImageX = contrib->imageX - 0.5f;
	float dImageY = contrib->imageY - 0.5f;

	// Get filter coefficients
	const FilterLUT &filterLUT = 
		filterLUTs->GetLUT(dImageX - Floor2Int(contrib->imageX), dImageY - Floor2Int(contrib->imageY));
	const float *lut = filterLUT.GetLUT();

	int x0 = Ceil2Int (dImageX - filter->xWidth);
	int x1 = x0 + filterLUT.GetWidth();
	int y0 = Ceil2Int (dImageY - filter->yWidth);
	int y1 = y0 + filterLUT.GetHeight();
	if (x1 < x0 || y1 < y0 || x1 < 0 || y1 < 0)
		return;

	for (u_int y = static_cast<u_int>(max(y0, static_cast<int>(yPixelStart))); y < static_cast<u_int>(min(y1, static_cast<int>(yPixelStart + yPixelCount))); ++y) {
		const int yoffset = (y-y0) * filterLUT.GetWidth();
		for (u_int x = static_cast<u_int>(max(x0, static_cast<int>(xPixelStart))); x < static_cast<u_int>(min(x1, static_cast<int>(xPixelStart + xPixelCount))); ++x) {
			// Evaluate filter value at $(x,y)$ pixel
			const int xoffset = x-x0;
			const float filterWt = lut[yoffset + xoffset];
			// Update pixel values with filtered sample contribution
			buffer->Add(x - xPixelStart,y - yPixelStart,
				xyz, alpha, filterWt * weight);
			// Update ZBuffer values with filtered zdepth contribution
			if(use_Zbuf && contrib->zdepth != 0.f)
				ZBuffer->Add(x - xPixelStart, y - yPixelStart, contrib->zdepth, 1.0f);
		}
	}
}

void Film::SetSample(const Contribution *contrib) {
	XYZColor xyz = contrib->color;
	const float alpha = contrib->alpha;
	const float weight = contrib->variance;

	// Issue warning if unexpected radiance value returned
	if (!(xyz.Y() >= 0.f) || isinf(xyz.Y())) {
		if(debug_mode) {
			LOG(LUX_WARNING,LUX_LIMIT) << "Out of bound intensity in Film::AddSample: "
			   << xyz.Y() << ", sample discarded";
		}
		return;
	}

	if (!(alpha >= 0.f) || isinf(alpha)) {
		if(debug_mode) {
			LOG(LUX_WARNING,LUX_LIMIT) << "Out of bound  alpha in Film::AddSample: "
			   << alpha << ", sample discarded";
		}
		return;
	}

	if (!(weight >= 0.f) || isinf(weight)) {
		if(debug_mode) {
			LOG(LUX_WARNING,LUX_LIMIT) << "Out of bound  weight in Film::AddSample: "
			   << weight << ", sample discarded";
		}
		return;
	}

	// Reject samples higher than max Y() after warmup period
	if (warmupComplete) {
		if(xyz.Y() > maxY)
			return;
	} else {
	 	maxY = max(maxY, xyz.Y());
		++warmupSamples;
	 	if (warmupSamples >= reject_warmup_samples)
			warmupComplete = true;
	}

	if (premultiplyAlpha)
		xyz *= alpha;

	BufferGroup &currentGroup = bufferGroups[contrib->bufferGroup];
	Buffer *buffer = currentGroup.getBuffer(contrib->buffer);

	// Compute sample's raster extent
	const u_int dImageX = contrib->imageX - 0.5f;
	const u_int dImageY = contrib->imageY - 0.5f;
	buffer->Set(dImageX, dImageY, xyz, alpha);

	// Update ZBuffer values with filtered zdepth contribution
	if(use_Zbuf && contrib->zdepth != 0.f)
		ZBuffer->Add(dImageX, dImageY, contrib->zdepth, 1.0f);
}

void Film::WriteResumeFilm(const string &filename)
{
	// Dade - save the status of the film to the file
	LOG(LUX_INFO, LUX_NOERROR) << "Writing film status to file";

	string tempfilename = filename + ".temp";

    std::ofstream filestr(tempfilename.c_str(), std::ios_base::out | std::ios_base::binary);
	if(!filestr) {
		LOG(LUX_SEVERE,LUX_SYSTEM) << "Cannot open file '" << tempfilename << "' for writing resume film";

		return;
	}

	bool writeSuccessful = TransmitFilm(filestr,false,true);

    filestr.close();

	if (writeSuccessful) {
		// if remove fails, rename might depending on platform fail, catch error there
		remove(filename.c_str());
		if (rename(tempfilename.c_str(), filename.c_str())) {
			LOG(LUX_ERROR, LUX_SYSTEM) << 
				"Failed to rename new resume film, leaving new resume film as '" << tempfilename << "'";
			return;
		}
		LOG(LUX_INFO, LUX_NOERROR) << "Film status written to '" << filename << "'";
	}
}


/**
 * FLM format
 * ----------
 *
 * Layout:
 *
 *   HEADER
 *   magic_number                  - int   - the magic number number
 *   version_number                - int   - the version number
 *   x_resolution                  - int   - the x resolution of the buffers
 *   y_resolution                  - int   - the y resolution of the buffers
 *   #buffer_groups                - u_int - the number of lightgroups
 *   #buffer_configs               - u_int - the number of buffers per light group
 *   for i in 1:#buffer_configs
 *     buffer_type                 - int   - the type of the i'th buffer
 *   #parameters                   - u_int - the number of stored parameters
 *   for i in 1:#parameters
 *     param_type                  - int   - the type of the i'th parameter
 *     param_size                  - int   - the size of the value of the i'th parameter in bytes
 *     param_id                    - int   - the id of the i'th parameter
 *     param_index                 - int   - the index of the i'th parameter
 *     param_value                 - *     - the value of the i'th parameter
 *
 *   DATA
 *   for i in 1:#buffer_groups
 *     #samples                    - float - the number of samples in the i'th buffer group
 *     for j in 1:#buffer_configs
 *       for y in 1:y_resolution
 *         for x in 1:x_resolution
 *           X                     - float - the weighted sum of all X values added to the pixel
 *           Y                     - float - the weighted sum of all Y values added to the pixel
 *           Z                     - float - the weighted sum of all Z values added to the pixel
 *           alpha                 - float - the weighted sum of all alpha values added to the pixel
 *           weight_sum            - float - the sum of al weights of all values added to the pixel
 *     
 *
 * Remarks:
 *  - data is written as binary little-endian
 *  - data is gzipped
 *  - the version is not intended for backward/forward compatibility but just as a check
 */
static const int FLM_MAGIC_NUMBER = 0xCEBCD816;
static const int FLM_VERSION = 0; // should be incremented on each change to the format to allow detecting unsupported FLM data!
enum FlmParameterType {
	FLM_PARAMETER_TYPE_FLOAT = 0,
	FLM_PARAMETER_TYPE_STRING = 1
};

class FlmParameter {
public:
	FlmParameter() {}
	FlmParameter(Film *aFilm, FlmParameterType aType, luxComponentParameters aParam, u_int aIndex) {
		type = aType;
		id = aParam;
		index = aIndex;
		switch (type) {
			case FLM_PARAMETER_TYPE_FLOAT:
				size = 4;
				floatValue = static_cast<float>(aFilm->GetParameterValue(aParam, aIndex));
				break;
			case FLM_PARAMETER_TYPE_STRING:
				stringValue = aFilm->GetStringParameterValue(aParam, aIndex);
				size = stringValue.size();
				break;
			default: {
				LOG(LUX_ERROR,LUX_SYSTEM) << "Invalid parameter type (expected value in [0,1], got=" << type << ")";
				break;
			}
		}
	}

	void Set(Film *aFilm) {
		switch (type) {
			case FLM_PARAMETER_TYPE_FLOAT:
				aFilm->SetParameterValue(id, floatValue, index);
				break;
			case FLM_PARAMETER_TYPE_STRING:
				aFilm->SetStringParameterValue(id, stringValue, index);
				break;
			default:
				// ignore invalid type (already reported in constructor)
				break;
		}
	}

	bool Read(std::basic_istream<char> &is, bool isLittleEndian, Film *film ) {
		int tmpType;
		tmpType = osReadLittleEndianInt(isLittleEndian, is);
		type = FlmParameterType(tmpType);
		if (!is.good()) {
			LOG(LUX_ERROR,LUX_SYSTEM)<< "Error while receiving film";
			return false;
		}
		size = osReadLittleEndianUInt(isLittleEndian, is);
		if (!is.good()) {
			LOG(LUX_ERROR,LUX_SYSTEM)<< "Error while receiving film";
			return false;
		}
		id = static_cast<luxComponentParameters>(osReadLittleEndianInt(isLittleEndian, is));
		if (!is.good()) {
			LOG(LUX_ERROR,LUX_SYSTEM)<< "Error while receiving film";
			return false;
		}
		index = osReadLittleEndianUInt(isLittleEndian, is);
		if (!is.good()) {
			LOG(LUX_ERROR,LUX_SYSTEM)<< "Error while receiving film";
			return false;
		}
		switch (type) {
			case FLM_PARAMETER_TYPE_FLOAT:
				if (size != 4) {
					LOG(LUX_ERROR,LUX_SYSTEM) << "Invalid parameter size (expected value for float is 4, received=" << size << ")";
					return false;
				}
				floatValue = osReadLittleEndianFloat(isLittleEndian, is);
				break;
			case FLM_PARAMETER_TYPE_STRING: {
				char* chars = new char[size+1];
				is.read(chars, size);
				chars[size] = '\0';
				stringValue = string(chars);
				delete[] chars;
				break;
			}
			default: {
				LOG(LUX_ERROR,LUX_SYSTEM) << "Invalid parameter type (expected value in [0,1], received=" << tmpType << ")";
				return false;
			}
		}
		return true;
	}
	void Write(std::basic_ostream<char> &os, bool isLittleEndian) const {
		osWriteLittleEndianInt(isLittleEndian, os, type);
		osWriteLittleEndianUInt(isLittleEndian, os, size);
		osWriteLittleEndianInt(isLittleEndian, os, id);
		osWriteLittleEndianUInt(isLittleEndian, os, index);
		switch (type) {
			case FLM_PARAMETER_TYPE_FLOAT:
				osWriteLittleEndianFloat(isLittleEndian, os, floatValue);
				break;
			case FLM_PARAMETER_TYPE_STRING:
				os.write(stringValue.c_str(), size);
				break;
			default:
				// ignore invalid type (already reported in constructor)
				break;
		}
	}

private:
	FlmParameterType type;
	u_int size;
	luxComponentParameters id;
	u_int index;
		
	float floatValue;
	string stringValue;
};

class FlmHeader {
public:
	FlmHeader() {}
	bool Read(filtering_stream<input> &in, bool isLittleEndian, Film *film );
	void Write(std::basic_ostream<char> &os, bool isLittleEndian) const;

	int magicNumber;
	int versionNumber;
	u_int xResolution;
	u_int yResolution;
	u_int numBufferGroups;
	u_int numBufferConfigs;
	vector<int> bufferTypes;
	u_int numParams;
	vector<FlmParameter> params;
};

bool FlmHeader::Read(filtering_stream<input> &in, bool isLittleEndian, Film *film ) {
	// Read and verify magic number and version
	magicNumber = osReadLittleEndianInt(isLittleEndian, in);
	if (!in.good()) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< "Error while receiving film";
		return false;
	}
	if (magicNumber != FLM_MAGIC_NUMBER) {
		LOG(LUX_ERROR,LUX_SYSTEM) << "Invalid FLM magic number (expected=" << FLM_MAGIC_NUMBER 
			<< ", received=" << magicNumber << ")";
		return false;
	}
	versionNumber = osReadLittleEndianInt(isLittleEndian, in);
	if (!in.good()) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< "Error while receiving film";
		return false;
	}
	if (versionNumber != FLM_VERSION) {
		LOG(LUX_ERROR,LUX_SYSTEM) << "Invalid FLM version (expected=" << FLM_VERSION 
			<< ", received=" << versionNumber << ")";
		return false;
	}
	// Read and verify the buffer resolution
	xResolution = osReadLittleEndianUInt(isLittleEndian, in);
	yResolution = osReadLittleEndianUInt(isLittleEndian, in);
	if (xResolution == 0 || yResolution == 0 ) {
		LOG(LUX_ERROR,LUX_SYSTEM)
			<< "Invalid resolution (expected positive resolution, received="
			<< xResolution << "x" << yResolution
			<< ")";
		return false;
	}
	if (film != NULL &&
		(xResolution != film->GetXPixelCount() ||
		yResolution != film->GetYPixelCount())) {
		LOG(LUX_ERROR,LUX_SYSTEM)
			<< "Invalid resolution (expected=" << film->GetXPixelCount() << "x" << film->GetYPixelCount()
			<< ", received=" << xResolution << "x" << yResolution << ")";
		return false;
	}
	// Read and verify #buffer groups and buffer configs
	numBufferGroups = osReadLittleEndianUInt(isLittleEndian, in);
	if (!in.good()) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< "Error while receiving film";
		return false;
	}
	if (film != NULL && numBufferGroups != film->GetNumBufferGroups()) {
		LOG(LUX_ERROR,LUX_SYSTEM)
			<< "Invalid number of buffer groups (expected=" << film->GetNumBufferGroups() 
			<< ", received=" << numBufferGroups << ")";
		return false;
	}
	numBufferConfigs = osReadLittleEndianUInt(isLittleEndian, in);
	if (!in.good()) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< "Error while receiving film";
		return false;
	}
	if (film != NULL && numBufferConfigs != film->GetNumBufferConfigs()) {
		LOG(LUX_ERROR,LUX_SYSTEM)
			<< "Invalid number of buffers (expected=" << film->GetNumBufferConfigs()
			<< ", received=" << numBufferConfigs << ")";
		return false;
	}
	for (u_int i = 0; i < numBufferConfigs; ++i) {
		int type;
		type = osReadLittleEndianInt(isLittleEndian, in);
		if (!in.good()) {
			LOG(LUX_ERROR,LUX_SYSTEM)<< "Error while receiving film";
			return false;
		}
		if (type < 0 || type >= NUM_OF_BUFFER_TYPES) {
			LOG(LUX_ERROR,LUX_SYSTEM) << "Invalid buffer type for buffer " << i << "(expected number in [0," << NUM_OF_BUFFER_TYPES << "[, received=" << type << ")";
			return false;
		}
		if (film != NULL && type != film->GetBufferConfig(i).type) {
			LOG(LUX_ERROR,LUX_SYSTEM) << "Invalid buffer type for buffer " << i << " (expected=" << film->GetBufferConfig(i).type
				<< ", received=" << type << ")";
			return false;
		}
		bufferTypes.push_back(type);
	}
	// Read parameters
	numParams = osReadLittleEndianUInt(isLittleEndian, in);
	if (!in.good()) {
		LOG( LUX_ERROR,LUX_SYSTEM)<< "Error while receiving film";
		return false;
	}
	params.reserve(numParams);
	for(u_int i = 0; i < numParams; ++i) {
		FlmParameter param;
		bool ok = param.Read(in, isLittleEndian, film);
		if (!in.good()) {
			LOG( LUX_ERROR,LUX_SYSTEM)<< "Error while receiving film";
			return false;
		}
		if(!ok) {
			//LOG(LUX_ERROR,LUX_SYSTEM) << "Invalid parameter (id=" << param.id << ", index=" << param.index << ", value=" << param.value << ")";
			return false;
		}
		params.push_back(param);
	}
	return true;
}

void FlmHeader::Write(std::basic_ostream<char> &os, bool isLittleEndian) const
{
	// Write magic number and version
	osWriteLittleEndianInt(isLittleEndian, os, magicNumber);
	osWriteLittleEndianInt(isLittleEndian, os, versionNumber);
	// Write buffer resolution
	osWriteLittleEndianUInt(isLittleEndian, os, xResolution);
	osWriteLittleEndianUInt(isLittleEndian, os, yResolution);
	// Write #buffer groups and buffer configs for verification
	osWriteLittleEndianUInt(isLittleEndian, os, numBufferGroups);
	osWriteLittleEndianUInt(isLittleEndian, os, numBufferConfigs);
	for (u_int i = 0; i < numBufferConfigs; ++i)
		osWriteLittleEndianInt(isLittleEndian, os, bufferTypes[i]);
	// Write parameters
	osWriteLittleEndianUInt(isLittleEndian, os, numParams);
	for(u_int i = 0; i < numParams; ++i) {
		params[i].Write(os, isLittleEndian);
	}
}

double Film::DoTransmitFilm(
		std::basic_ostream<char> &os,
		bool clearBuffers,
		bool transmitParams)
{
	const bool isLittleEndian = osIsLittleEndian();

	LOG(LUX_DEBUG,LUX_NOERROR)<< "Transmitting film (little endian=" <<(isLittleEndian ? "true" : "false") << ")";

	// Write the header
	FlmHeader header;
	header.magicNumber = FLM_MAGIC_NUMBER;
	header.versionNumber = FLM_VERSION;
	header.xResolution = xPixelCount;
	header.yResolution = yPixelCount;
	header.numBufferGroups = bufferGroups.size();
	header.numBufferConfigs = bufferConfigs.size();
	for (u_int i = 0; i < bufferConfigs.size(); ++i)
		header.bufferTypes.push_back(bufferConfigs[i].type);
	// Write parameters
	if (transmitParams) {
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_TONEMAPKERNEL, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_REINHARD_PRESCALE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_REINHARD_POSTSCALE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_REINHARD_BURN, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_LINEAR_SENSITIVITY, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_LINEAR_EXPOSURE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_LINEAR_FSTOP, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_LINEAR_GAMMA, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_CONTRAST_YWA, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_X_WHITE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_Y_WHITE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_X_RED, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_Y_RED, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_X_GREEN, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_Y_GREEN, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_X_BLUE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_Y_BLUE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_GAMMA, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_CAMERA_RESPONSE_ENABLED, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_STRING, LUX_FILM_CAMERA_RESPONSE_FILE, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_UPDATEBLOOMLAYER, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_DELETEBLOOMLAYER, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_BLOOMRADIUS, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_BLOOMWEIGHT, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_VIGNETTING_ENABLED, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_VIGNETTING_SCALE, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_ABERRATION_ENABLED, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_ABERRATION_AMOUNT, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_UPDATEGLARELAYER, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_DELETEGLARELAYER, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_GLARE_AMOUNT, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_GLARE_RADIUS, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_GLARE_BLADES, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_GLARE_THRESHOLD, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_CHIU_ENABLED, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_CHIU_RADIUS, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_CHIU_INCLUDECENTER, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_ENABLED, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_AMPLITUDE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_NBITER, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_SHARPNESS, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_ANISOTROPY, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_ALPHA, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_SIGMA, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_FASTAPPROX, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_GAUSSPREC, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_DL, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_DA, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_INTERP, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_TILE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_BTILE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_THREADS, 0));

		for(u_int i = 0; i < GetNumBufferGroups(); ++i) {
			header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_LG_SCALE, i));
			header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_LG_ENABLE, i));
			header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_LG_SCALE_RED, i));
			header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_LG_SCALE_GREEN, i));
			header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_LG_SCALE_BLUE, i));
			header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_LG_TEMPERATURE, i));

			header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_STRING, LUX_FILM_LG_NAME, i));
		}

		header.numParams = header.params.size();
	} else {
		header.numParams = 0;
	}
	header.Write(os, isLittleEndian);

	// Write each buffer group
	double totNumberOfSamples = 0.;
	for (u_int i = 0; i < bufferGroups.size(); ++i) {
		BufferGroup& bufferGroup = bufferGroups[i];
		// Write number of samples
		osWriteLittleEndianDouble(isLittleEndian, os, bufferGroup.numberOfSamples);

		// Write each buffer
		for (u_int j = 0; j < bufferConfigs.size(); ++j) {
			Buffer* buffer = bufferGroup.getBuffer(j);

			// Write pixels
			const BlockedArray<Pixel>* pixelBuf = buffer->pixels;
			for (u_int y = 0; y < pixelBuf->vSize(); ++y) {
				for (u_int x = 0; x < pixelBuf->uSize(); ++x) {
					const Pixel &pixel = (*pixelBuf)(x, y);
					osWriteLittleEndianFloat(isLittleEndian, os, pixel.L.c[0]);
					osWriteLittleEndianFloat(isLittleEndian, os, pixel.L.c[1]);
					osWriteLittleEndianFloat(isLittleEndian, os, pixel.L.c[2]);
					osWriteLittleEndianFloat(isLittleEndian, os, pixel.alpha);
					osWriteLittleEndianFloat(isLittleEndian, os, pixel.weightSum);
				}
			}

			if (clearBuffers) {
				// Dade - reset the rendering buffer
				buffer->Clear();
			}
		}

		totNumberOfSamples += bufferGroup.numberOfSamples;
		LOG(LUX_DEBUG,LUX_NOERROR) << "Transmitted " << bufferGroup.numberOfSamples << " samples for buffer group " << i <<
			" (buffer config size: " << bufferConfigs.size() << ")";

		if (clearBuffers) {
			// Dade - reset the rendering buffer
			bufferGroup.numberOfSamples = 0;
		}
	}

	return totNumberOfSamples;

}

bool Film::TransmitFilm(
        std::basic_ostream<char> &stream,
        bool clearBuffers,
		bool transmitParams,
		bool useCompression, 
		bool directWrite)
{
	std::streampos stream_startpos = stream.tellp();

	double totNumberOfSamples = 0;

	bool transmitError = true;

	if (directWrite) {
		if (useCompression) {
			filtering_stream<output> fs;
			fs.push(gzip_compressor(9));
			fs.push(stream);
			totNumberOfSamples = DoTransmitFilm(fs, clearBuffers, transmitParams);

			flush(fs);

			transmitError = !fs.good();
		} else {
			totNumberOfSamples = DoTransmitFilm(stream, clearBuffers, transmitParams);
			transmitError = !stream.good();
		}
	} else {
		std::stringstream ss(std::stringstream::in | std::stringstream::out | std::stringstream::binary);
		totNumberOfSamples = DoTransmitFilm(ss, clearBuffers, transmitParams);

		transmitError = !ss.good();
		
		if (!transmitError) {
			if (useCompression) {
				filtering_streambuf<input> in;
				in.push(gzip_compressor(9));
				in.push(ss);
				boost::iostreams::copy(in, stream);
			} else {
				boost::iostreams::copy(ss, stream);
			}
		}
	}

	if (transmitError) {
		LOG(LUX_SEVERE,LUX_SYSTEM) << "Error while preparing film data for transmission";
		return false;
	}

	LOG(LUX_DEBUG,LUX_NOERROR) << "Transmitted a film with " << totNumberOfSamples << " samples";
	
	if (!stream.good()) {
		LOG(LUX_SEVERE,LUX_SYSTEM) << "Error while transmitting film";
		return false;
	}

	std::streamsize size = stream.tellp() - stream_startpos;

	LOG(LUX_INFO,LUX_NOERROR) << "Film transmission done (" << (size / 1024) << " Kbytes sent)";
	return true;
}


double Film::UpdateFilm(std::basic_istream<char> &stream) {
	const bool isLittleEndian = osIsLittleEndian();

	filtering_stream<input> in;
	in.push(gzip_decompressor());
	in.push(stream);

	LOG(LUX_DEBUG,LUX_NOERROR) << "Receiving film (little endian=" << (isLittleEndian ? "true" : "false") << ")";

	// Read header
	FlmHeader header;
	if (!header.Read(in, isLittleEndian, this))
		return 0.f;

	// Read buffer groups
	vector<double> bufferGroupNumSamples(bufferGroups.size());
	vector<BlockedArray<Pixel>*> tmpPixelArrays(bufferGroups.size() * bufferConfigs.size());
	for (u_int i = 0; i < bufferGroups.size(); i++) {
		double numberOfSamples;
		numberOfSamples = osReadLittleEndianDouble(isLittleEndian, in);
		if (!in.good())
			break;
		bufferGroupNumSamples[i] = numberOfSamples;

		// Read buffers
		for(u_int j = 0; j < bufferConfigs.size(); ++j) {
			const Buffer* localBuffer = bufferGroups[i].getBuffer(j);
			// Read pixels
			BlockedArray<Pixel> *tmpPixelArr = new BlockedArray<Pixel>(
				localBuffer->xPixelCount, localBuffer->yPixelCount);
			tmpPixelArrays[i*bufferConfigs.size() + j] = tmpPixelArr;
			for (u_int y = 0; y < tmpPixelArr->vSize(); ++y) {
				for (u_int x = 0; x < tmpPixelArr->uSize(); ++x) {
					Pixel &pixel = (*tmpPixelArr)(x, y);
					pixel.L.c[0] = osReadLittleEndianFloat(isLittleEndian, in);
					pixel.L.c[1] = osReadLittleEndianFloat(isLittleEndian, in);
					pixel.L.c[2] = osReadLittleEndianFloat(isLittleEndian, in);
					pixel.alpha = osReadLittleEndianFloat(isLittleEndian, in);
					pixel.weightSum = osReadLittleEndianFloat(isLittleEndian, in);
				}
			}
			if (!in.good())
				break;
		}
		if (!in.good())
			break;

		LOG( LUX_DEBUG,LUX_NOERROR)
			<< "Received " << bufferGroupNumSamples[i] << " samples for buffer group " << i
			<< " (buffer config size: " << bufferConfigs.size() << ")";
	}

	// Dade - check for errors
	double totNumberOfSamples = 0.;
	double maxTotNumberOfSamples = 0.;
	if (in.good()) {
		// Update parameters
		for (vector<FlmParameter>::iterator it = header.params.begin(); it != header.params.end(); ++it)
			it->Set(this);

		// Dade - add all received data
		for (u_int i = 0; i < bufferGroups.size(); ++i) {
			BufferGroup &currentGroup = bufferGroups[i];
			for (u_int j = 0; j < bufferConfigs.size(); ++j) {
				const BlockedArray<Pixel> *receivedPixels = tmpPixelArrays[ i * bufferConfigs.size() + j ];
				Buffer *buffer = currentGroup.getBuffer(j);

				for (u_int y = 0; y < buffer->yPixelCount; ++y) {
					for (u_int x = 0; x < buffer->xPixelCount; ++x) {
						const Pixel &pixel = (*receivedPixels)(x, y);
						Pixel &pixelResult = (*buffer->pixels)(x, y);
						pixelResult.L.c[0] += pixel.L.c[0];
						pixelResult.L.c[1] += pixel.L.c[1];
						pixelResult.L.c[2] += pixel.L.c[2];
						pixelResult.alpha += pixel.alpha;
						pixelResult.weightSum += pixel.weightSum;
					}
				}
			}

			currentGroup.numberOfSamples += bufferGroupNumSamples[i];
			// Check if we have enough samples per pixel
			if ((haltSamplesPerPixel > 0) &&
				(currentGroup.numberOfSamples >= haltSamplesPerPixel * samplePerPass))
				enoughSamplesPerPixel = true;
			totNumberOfSamples += bufferGroupNumSamples[i];
			maxTotNumberOfSamples = max(maxTotNumberOfSamples, bufferGroupNumSamples[i]);
		}

		numberOfSamplesFromNetwork += maxTotNumberOfSamples;

		LOG( LUX_DEBUG,LUX_NOERROR) << "Received film with " << totNumberOfSamples << " samples";
	} else
		LOG( LUX_ERROR,LUX_SYSTEM)<< "IO error while receiving film buffers";

	// Clean up
	for (u_int i = 0; i < tmpPixelArrays.size(); ++i)
		delete tmpPixelArrays[i];

	return maxTotNumberOfSamples;
}

bool Film::LoadResumeFilm(const string &filename)
{
	// Read the FLM header
	std::ifstream stream(filename.c_str(), std::ios_base::in | std::ios_base::binary);
	filtering_stream<input> in;
	in.push(gzip_decompressor());
	in.push(stream);
	const bool isLittleEndian = osIsLittleEndian();
	FlmHeader header;
	bool headerOk = header.Read(in, isLittleEndian, NULL);
	stream.close();
	if (!headerOk)
		return false;
	xResolution = static_cast<int>(header.xResolution);
	yResolution = static_cast<int>(header.yResolution);
	xPixelStart = 0; // by default use full resolution
	yPixelStart = 0; 
	xPixelCount = xResolution;
	yPixelCount = yResolution;

	// Create the buffers (also loads the FLM file)
	for (u_int i = 0; i < header.numBufferConfigs; ++i)
		RequestBuffer(BufferType(header.bufferTypes[i]), BUF_FRAMEBUFFER, "");

	vector<string> bufferGroups;
	for (u_int i = 0; i < header.numBufferGroups; ++i) {
		std::stringstream ss;
		ss << "lightgroup #" << (i + 1);
		bufferGroups.push_back(ss.str());
	}
	RequestBufferGroups(bufferGroups);
	CreateBuffers();

	return true;
}


void Film::getHistogramImage(unsigned char *outPixels, u_int width, u_int height, int options)
{
    boost::mutex::scoped_lock lock(histMutex);
	if (!histogram)
		histogram = new Histogram();
	histogram->MakeImage(outPixels, width, height, options);
}

// Histogram Function Definitions

Histogram::Histogram()
{
	m_buckets = NULL;
	m_displayGamma = 2.2f; //gamma of the display the histogram is viewed on

	//calculate visible plot range
	float narrowRangeSize = -logf(powf(.5f, 10.f / m_displayGamma)); //size of 10 EV zones, display-gamma corrected
	float scalingFactor = 0.75f;
	m_lowRange = -(1.f + scalingFactor) * narrowRangeSize;
	m_highRange = scalingFactor * narrowRangeSize;

	m_bucketNr = 0;
	m_newBucketNr = 300;
	CheckBucketNr();
	for (u_int i = 0; i < m_bucketNr * 4; ++i)
		m_buckets[i] = 0;
}

Histogram::~Histogram()
{
	delete[] m_buckets;
}

void Histogram::CheckBucketNr()
{
	if (m_newBucketNr > 0) { //if nr of buckets changed recalculate data that depends on it
		m_bucketNr = m_newBucketNr;
		m_newBucketNr = 0;
		delete[] m_buckets;
		m_buckets = new float[m_bucketNr * 4];

		//new bucket size
		m_bucketSize = (m_highRange - m_lowRange) / m_bucketNr;

		//calculate EV zone tick positions
		float zoneValue = 1.f;
		for (u_int i = 0; i < 11; ++i) {
			float value = logf(powf(zoneValue, 1.f / m_displayGamma));
			u_int bucket = Clamp(Round2UInt((value - m_lowRange) / m_bucketSize), 0U, m_bucketNr - 1);
			m_zones[i] = bucket;
			zoneValue /= 2;
		}
	}
}

void Histogram::Calculate(vector<RGBColor> &pixels, u_int width, u_int height)
{
	boost::mutex::scoped_lock lock(this->m_mutex);
	if (pixels.empty() || width == 0 || height == 0)
		return;
	u_int pixelNr = width * height;
	float value;

	CheckBucketNr();

	//empty buckets
	for (u_int i = 0; i < m_bucketNr * 4; ++i)
		m_buckets[i] = 0;

	//fill buckets
	for (u_int i = 0; i < pixelNr; ++i) {
		for (u_int j = 0; j < 3; ++j){ //each color channel
			value = pixels[i].c[j];
			if (value > 0.f) {
				value = logf(value);
				u_int bucket = Clamp(Round2UInt((value - m_lowRange) / m_bucketSize), 0U, m_bucketNr - 1);
				m_buckets[bucket * 4 + j] += 1.f;
			}
		}
		value = pixels[i].Y(); //brightness
		if (value > 0.f) {
			value = logf(value);
			u_int bucket = Clamp(Round2UInt((value - m_lowRange) / m_bucketSize), 0U, m_bucketNr - 1);
			m_buckets[bucket * 4 + 3] += 1.f;
		}
	}
}

void Histogram::MakeImage(unsigned char *outPixels, u_int canvasW, u_int canvasH, int options){
    boost::mutex::scoped_lock lock(this->m_mutex);
	#define PIXELIDX(x,y,w) ((y)*(w)*3+(x)*3)
	#define GETMAX(x,y) ((x)>(y)?(x):(y))
	#define OPTIONS_CHANNELS_MASK (LUX_HISTOGRAM_LOG-1)
	if (canvasW < 50 || canvasH < 25)
		return; //too small
	const u_int borderW = 3; //size of the plot border in pixels
	const u_int guideW = 3; //size of the brightness guide bar in pixels
	const u_int plotH = canvasH - borderW - (guideW + 2) - (borderW - 1);
	const u_int plotW = canvasW - 2 * borderW;
   
	if (canvasW - 2 * borderW != m_bucketNr)
		m_newBucketNr = canvasW - 2 * borderW;

	//clear drawing area
	unsigned char color = 64;
	for (u_int x = 0; x < plotW; ++x) {
		for (u_int y = 0; y < plotH; ++y) {
			const u_int idx = PIXELIDX(x + borderW, y + borderW, canvasW);
			outPixels[idx] = color;
			outPixels[idx + 1] = color;
			outPixels[idx + 2] = color;
		}
	}

	//transform values to log if needed
	float *buckets;
	if (options & LUX_HISTOGRAM_LOG) {
		buckets = new float[m_bucketNr * 4];
		for (u_int i = 0; i < m_bucketNr * 4; ++i)
			buckets[i] = logf(1.f + m_buckets[i]);
	} else
		buckets = m_buckets;

	//draw histogram bars
	u_int channel = 0;
	switch (options & OPTIONS_CHANNELS_MASK) {
		case LUX_HISTOGRAM_RGB: {
			//get maxima for scaling
			float max = 0.f;
			for (u_int i = 0; i < m_bucketNr * 4; ++i) {
				if (i % 4 != 3 && buckets[i] > max)
					max = buckets[i];
			}
			if (max > 0.f) {
				//draw bars
				for (u_int x = 0; x < plotW; ++x) {
					const u_int bucket = Clamp(x * m_bucketNr / (plotW - 1), 0U, m_bucketNr - 1);
					const float scale = plotH / max;
					for (u_int ch = 0; ch < 3; ++ch) {
						const u_int barHeight = Clamp(plotH - Round2UInt(buckets[bucket * 4 + ch] * scale), 0U, plotH);
						for(u_int y = barHeight; y < plotH; ++y)
							outPixels[PIXELIDX(x + borderW, y + borderW, canvasW) + ch] = 255;
					}
				}
			}
		} break;
		case LUX_HISTOGRAM_VALUE: channel++;
		case LUX_HISTOGRAM_BLUE:  channel++;
		case LUX_HISTOGRAM_GREEN: channel++;
		case LUX_HISTOGRAM_RED: {
			//get maxima for scaling
			float max = 0.f;
			for (u_int i = 0; i < m_bucketNr; ++i) {
				if (buckets[i * 4 + channel] > max)
					max = buckets[i * 4 + channel];
			}
			if (max > 0.f) {
				//draw bars
				for (u_int x = 0; x < plotW; ++x) {
					const u_int bucket = Clamp(x * m_bucketNr / (plotW - 1), 0U, m_bucketNr - 1);
					const float scale = plotH / max;
					const u_int barHeight = Clamp(plotH - Round2UInt(buckets[bucket * 4 + channel] * scale), 0U, plotH);
					for(u_int y = barHeight; y < plotH; ++y) {
						const u_int idx = PIXELIDX(x + borderW, y + borderW, canvasW);
						outPixels[idx] = 255;
						outPixels[idx + 1] = 255;
						outPixels[idx + 2] = 255;
					}
				}
			}
			break;
		}
		case LUX_HISTOGRAM_RGB_ADD: {
			//calculate maxima for scaling
			float max = 0.f;
			for (u_int i = 0; i < m_bucketNr; ++i) {
				const float val = buckets[i * 4] + buckets[i * 4 + 1] + buckets[i * 4 + 2];
				if (val > max)
					max = val;
			}
			if (max > 0.f) {
				//draw bars
				for (u_int x = 0; x < plotW; ++x) {
					const u_int bucket = Clamp(x * m_bucketNr / (plotW - 1), 0U, m_bucketNr - 1);
					const float scale = plotH / max;
					u_int barHeight = Clamp(plotH - Round2UInt((buckets[bucket * 4] + buckets[bucket * 4 + 1] + buckets[bucket * 4 + 2]) * scale), 0U, plotH);
					u_int newHeight = barHeight;
					for (u_int ch = 0; ch < 3; ++ch) {
						newHeight += Floor2UInt(buckets[bucket * 4 + ch] * scale + 0.5f);
						for (u_int y = barHeight; y < newHeight; ++y)
							outPixels[PIXELIDX(x + borderW, y + borderW, canvasW) + ch] = 255;
						barHeight = newHeight;
					}
				}
			}
			break;
		}
		default:
			break;
	}

	if (buckets != m_buckets)
		delete[] buckets;

	//draw brightness guide
	for (u_int x = 0; x < plotW; ++x) {
		u_int bucket = Clamp(x * m_bucketNr / (plotW - 1), 0U, m_bucketNr - 1);
		for (u_int y = plotH + 2; y < plotH + 2 + guideW; ++y) {
			const u_int idx = PIXELIDX(x + borderW, y + borderW, canvasW);
			const unsigned char color = static_cast<unsigned char>(Clamp(expf(bucket * m_bucketSize + m_lowRange) * 256.f, 0.f, 255.f)); //no need to gamma-correct, as we're already in display-gamma space
			switch (options & OPTIONS_CHANNELS_MASK) {
				case LUX_HISTOGRAM_RED:
					outPixels[idx] = color;
					outPixels[idx + 1] = 0;
					outPixels[idx + 2] = 0;
					break;
				case LUX_HISTOGRAM_GREEN:
					outPixels[idx] = 0;
					outPixels[idx + 1] = color;
					outPixels[idx + 2] = 0;
					break;
				case LUX_HISTOGRAM_BLUE:
					outPixels[idx] = 0;
					outPixels[idx + 1] = 0;
					outPixels[idx + 2] = color;
					break;
				default:
					outPixels[idx] = color;
					outPixels[idx + 1] = color;
					outPixels[idx + 2] = color;
					break;
			}
		}
	}

	//draw EV zone markers
	for (u_int i = 0; i < 11; ++i) {
		switch (i) {
			case 0:
				color = 128;
				break;
			case 10:
				color = 128;
				break;
			default:
				color = 0;
				break;
		}
		const u_int bucket = m_zones[i];
		const u_int x = Clamp(bucket * plotW / (m_bucketNr - 1), 0U, plotW - 1);
		for (u_int y = plotH + 2; y < plotH + 2 + guideW; ++y) {
			const u_int idx = PIXELIDX(x + borderW, y + borderW, canvasW);
			outPixels[idx] = color;
			outPixels[idx + 1] = color;
			outPixels[idx + 2] = color;
		}
	}

	//draw zone boundaries on the plot
	for (u_int i = 0; i < 2; ++i) {
		u_int bucket;
		switch (i) {
			case 0:
				bucket = m_zones[0];
				break;  //white
			default: // In order to suppress a GCC warning
			case 1:
				bucket = m_zones[10];
				break; //black
		}
		const u_int x = Clamp(bucket * plotW / (m_bucketNr - 1), 0U, plotW - 1);
		for (u_int y = 0; y < plotH; ++y) {
			const u_int idx = PIXELIDX(x + borderW, y + borderW, canvasW);
			if (outPixels[idx] == 255 &&
				outPixels[idx + 1] == 255 &&
				outPixels[idx + 2] == 255) {
				outPixels[idx] = 128;
				outPixels[idx + 1] = 128;
				outPixels[idx + 2] = 128;
			} else {
				if (outPixels[idx] == 64)
					outPixels[idx] = 128;
				if (outPixels[idx + 1] == 64)
					outPixels[idx + 1] = 128;
				if (outPixels[idx + 2] == 64)
					outPixels[idx + 2] = 128;
			}
		}
	}
}


}//namespace lux

