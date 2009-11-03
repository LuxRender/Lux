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
		float px = 0 - maxRes * 0.5f;
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
	ColorSystem &colorSpace, Histogram *histogram, bool HistogramEnabled,
	bool &haveBloomImage, XYZColor *&bloomImage, bool bloomUpdate,
	float bloomRadius, float bloomWeight,
	bool VignettingEnabled, float VignetScale,
	bool aberrationEnabled, float aberrationAmount,
	bool &haveGlareImage, XYZColor *&glareImage, bool glareUpdate,
	float glareAmount, float glareRadius, u_int glareBlades,
	const char *toneMapName, const ParamSet *toneMapParams,
	float gamma, float dither)
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
			ProgressReporter prog(yResolution, "Bloom filter"); //NOBOOK
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
				prog.Update(); //NOBOOK
			}
			prog.Done(); //NOBOOK
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
			for(u_int i = 0; i < nPix; ++i)
				glareImage[i] = XYZColor(0.f);

			const float radius = maxRes * glareRadius;

			// add rotated versions of the blurred image
			const float invBlades = 1.f / glareBlades;
			float angle = 0.f;
			for (u_int i = 0; i < glareBlades; ++i) {
				rotateImage(xyzpixels, rotatedImage, xResolution, yResolution, angle);
				horizontalGaussianBlur(rotatedImage, blurredImage, maxRes, maxRes, radius);
				rotateImage(blurredImage, rotatedImage, maxRes, maxRes, -angle);

				// add to output
				for(u_int y = 0; y < yResolution; ++y) {
					for(u_int x = 0; x < xResolution; ++x) {
						const u_int sx = x + (maxRes - xResolution) / 2;
						const u_int sy = y + (maxRes - yResolution) / 2;

						glareImage[y*xResolution+x] += rotatedImage[sy*maxRes + sx];
					}
				}
				angle += 2.f * M_PI * invBlades;
			}

			// normalize
			for(u_int i = 0; i < nPix; ++i)
				glareImage[i] *= invBlades;

			rotatedImage.clear();
			blurredImage.clear();
		}

		if (haveGlareImage && glareImage != NULL) {
			for(u_int i = 0; i < nPix; ++i)
				xyzpixels[i] = Lerp(glareAmount, xyzpixels[i], glareImage[i]);
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
	const float invGamma = 1.f / gamma;
	for (u_int i = 0; i < nPix; ++i) {
		rgbpixels[i] = colorSpace.ToRGBConstrained(xyzpixels[i]);
		// Do gamma correction
		rgbpixels[i] = rgbpixels[i].Pow(invGamma);
	}

	// DO NOT USE xyzpixels ANYMORE AFTER THIS POINT

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

	// Calculate histogram
	if (HistogramEnabled) {
		if (!histogram)
			histogram = new Histogram();
		histogram->Calculate(rgbpixels, xResolution, yResolution);
	}

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
}

// Film Function Definitions

void Film::getHistogramImage(unsigned char *outPixels, u_int width, u_int height, int options)
{
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
	if (pixels.empty() || width == 0 || height == 0)
		return;
	u_int pixelNr = width * height;
	float value;
	boost::mutex::scoped_lock lock(m_mutex);

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
	#define PIXELIDX(x,y,w) ((y)*(w)*3+(x)*3)
	#define GETMAX(x,y) ((x)>(y)?(x):(y))
	if (canvasW < 50 || canvasH < 25)
		return; //too small
	const u_int borderW = 3; //size of the plot border in pixels
	const u_int guideW = 3; //size of the brightness guide bar in pixels
	const u_int plotH = canvasH - borderW - (guideW + 2) - (borderW - 1);
	const u_int plotW = canvasW - 2 * borderW;
	boost::mutex::scoped_lock lock(m_mutex);
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
	switch (options) {
		case LUX_HISTOGRAM_RGB: {
			//get maxima for scaling
			float max = 0.f;
			for (u_int i = 4; i < (m_bucketNr - 1) * 4; ++i) {
				if (i % 4 != 3 && buckets[i] > max)
					max = buckets[i];
			}
			if (max > 0.f) {
				//draw bars
				for (u_int x = 0; x < plotW; ++x) {
					const u_int bucket = Clamp(x * m_bucketNr / (plotW - 1U), 0U, m_bucketNr - 1);
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
			for (u_int i = 1; i < m_bucketNr-1; ++i) {
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
			for (u_int i = 1; i < m_bucketNr - 1; ++i) {
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
						newHeight += buckets[bucket * 4 + ch] * scale + 0.5f;
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
			switch (options) {
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

