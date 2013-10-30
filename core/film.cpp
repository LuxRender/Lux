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

// film.cpp*
#include "film.h"
#include "randomgen.h"
#include "dynload.h"
#include "paramset.h"
#include "tonemap.h"
#include "cameraresponse.h"
#include "filter.h"
#include "contribution.h"
#include "blackbodyspd.h"
#include "osfunc.h"
#include "streamio.h"
#include "exrio.h"

#include <algorithm>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
//#include <boost/math/special_functions/bessel.hpp>
#include <complex>

#include <fftw3.h>

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

#define cimg_use_fftw 1

#define cimg_debug 0     // Disable modal window in CImg exceptions.
// Include the CImg Library, with the GREYCstoration plugin included
#define cimg_plugin "greycstoration.h"

#include "cimg.h"
#include "imagereader.h"
using namespace cimg_library;
#if cimg_OS!=2
#include <pthread.h>
#include <X11/X.h>
#endif

using namespace lux;


template<typename T> 
static T bilinearSampleImage(const vector<T> &pixels,
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
static void horizontalGaussianBlur(const vector<XYZColor> &in, vector<XYZColor> &out,
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

	for (u_int x = 0; x <= pixel_rad; ++x)
		filter_weights[x] *= sweight;

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

static void rotateImage(const vector<XYZColor> &in, vector<XYZColor> &out,
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

namespace lux {

struct BloomFilter
{
	u_int const xResolution;
	u_int const yResolution;
	u_int const bloomWidth;
	vector<float> const &bloomFilter;
	XYZColor * const bloomImage;
	vector<XYZColor> const &xyzpixels;

	BloomFilter(
		u_int const xResolution_,
		u_int const yResolution_,
		u_int const bloomWidth_,
		vector<float> const &bloomFilter_,
		XYZColor * const bloomImage_,
		vector<XYZColor> const &xyzpixels_
	):
		xResolution(xResolution_),
		yResolution(yResolution_),
		bloomWidth(bloomWidth_),
		bloomFilter(bloomFilter_),
		bloomImage(bloomImage_),
		xyzpixels(xyzpixels_)
	{}

	void operator()()
	{
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
						// Accumulate bloom from pixel $(bx,by)$
						const u_int dist2 = (x - bx) * (x - bx) + (y - by) * (y - by);
						u_int bloomOffset = bx + by * xResolution;
						const float wt = bloomFilter[dist2];
						if (wt == 0.f)
							continue;
						sumWt += wt;
						bloomImage[offset].AddWeighted(wt, xyzpixels[bloomOffset]);
					}
				}
				bloomImage[offset] /= sumWt;
			}
	//				prog.Update(); //NOBOOK //intermediate crashfix until imagepipelinerefactor is done - Jens
		}
	}
};

struct BloomFilterX
{
	u_int const xResolution;
	u_int const yResolution;
	u_int const bloomWidth;
	vector<float> const &bloomFilter;
	XYZColor * const bloomImage;
	XYZColor * const xyzpixels;

	BloomFilterX(
		u_int const xResolution_,
		u_int const yResolution_,
		u_int const bloomWidth_,
		vector<float> const &bloomFilter_,
		XYZColor * const bloomImage_,
		XYZColor * const xyzpixels_
	):
		xResolution(xResolution_),
		yResolution(yResolution_),
		bloomWidth(bloomWidth_),
		bloomFilter(bloomFilter_),
		bloomImage(bloomImage_),
		xyzpixels(xyzpixels_)
	{}

	void operator()()
	{
		// working row
		std::vector<XYZColor> row(xResolution, XYZColor(0.f));
		// Apply bloom filter to image pixels
		for (u_int y = 0; y < yResolution; ++y) {
			for (u_int x = 0; x < xResolution; ++x) {
				// Compute bloom for pixel _(x,y)_
				// Compute extent of pixels contributing bloom
				const u_int x0 = max(x, bloomWidth) - bloomWidth;
				const u_int x1 = min(x + bloomWidth, xResolution - 1);
				//const u_int y0 = max(y, bloomWidth) - bloomWidth;
				//const u_int y1 = min(y + bloomWidth, yResolution - 1);
				//const u_int offset = y * xResolution + x;
				float sumWt = 0.f;
				const u_int by = y;
				XYZColor &pixel(row[x]);
				for (u_int bx = x0; bx <= x1; ++bx) {
					// Accumulate bloom from pixel $(bx,by)$
					const u_int dist2 = (x - bx) * (x - bx) + (y - by) * (y - by);
					const float wt = bloomFilter[dist2];
					if (wt == 0.f)
						continue;
					u_int bloomOffset = bx + by * xResolution;
					sumWt += wt;
					pixel.AddWeighted(wt, xyzpixels[bloomOffset]);
				}
				pixel /= sumWt;
			}
			// copy working row back into bloomImage
			for (u_int x = 0; x < xResolution; ++x) {
				bloomImage[y * xResolution + x] = row[x]; 
			}
		}
	}
};

struct BloomFilterY
{
	u_int const xResolution;
	u_int const yResolution;
	u_int const bloomWidth;
	vector<float> const &bloomFilter;
	XYZColor * const bloomImage;
	XYZColor * const xyzpixels;

	BloomFilterY(
		u_int const xResolution_,
		u_int const yResolution_,
		u_int const bloomWidth_,
		vector<float> const &bloomFilter_,
		XYZColor * const bloomImage_,
		XYZColor * const xyzpixels_
	):
		xResolution(xResolution_),
		yResolution(yResolution_),
		bloomWidth(bloomWidth_),
		bloomFilter(bloomFilter_),
		bloomImage(bloomImage_),
		xyzpixels(xyzpixels_)
	{}

	void operator()()
	{
		// working column
		std::vector<XYZColor> col(yResolution, XYZColor(0.f));
		// Apply bloom filter to image pixels
		for (u_int x = 0; x < xResolution; ++x) {
			for (u_int y = 0; y < yResolution; ++y) {
				// Compute bloom for pixel _(x,y)_
				// Compute extent of pixels contributing bloom
				//const u_int x0 = max(x, bloomWidth) - bloomWidth;
				//const u_int x1 = min(x + bloomWidth, xResolution - 1);
				const u_int y0 = max(y, bloomWidth) - bloomWidth;
				const u_int y1 = min(y + bloomWidth, yResolution - 1);
				//const u_int offset = y * xResolution + x;
				float sumWt = 0.f;
				XYZColor &pixel(col[y]);
				for (u_int by = y0; by <= y1; ++by) {
					const u_int bx = x;
					// Accumulate bloom from pixel $(bx,by)$
					const u_int dist2 = (x - bx) * (x - bx) + (y - by) * (y - by);
					const float wt = bloomFilter[dist2];
					if (wt == 0.f)
						continue;
					u_int bloomOffset = bx + by * xResolution;
					sumWt += wt;
					pixel.AddWeighted(wt, xyzpixels[bloomOffset]);
				}
				pixel /= sumWt;
			}
			// copy working column back into bloomImage
			for (u_int y = 0; y < yResolution; ++y) {
				bloomImage[y * xResolution + x] = col[y]; 
			}
		}
	}
};

struct VignettingFilter
{
	u_int xResolution;
	u_int yResolution;
	bool aberrationEnabled;
	float aberrationAmount;
	RGBColor * outp;
	vector<RGBColor>& rgbpixels;;
	bool VignettingEnabled;
	float VignetScale;

	const float invxRes, invyRes;


	VignettingFilter(
		u_int xResolution_,
		u_int yResolution_,
		bool aberrationEnabled_,
		float aberrationAmount_,
		RGBColor * outp_,
		vector<RGBColor>& rgbpixels_,
		bool VignettingEnabled_,
		float VignetScale_
	):
		xResolution(xResolution_),
		yResolution(yResolution_),
		aberrationEnabled(aberrationEnabled_),
		aberrationAmount(aberrationAmount_),
		outp(outp_),
		rgbpixels(rgbpixels_),
		VignettingEnabled(VignettingEnabled_),
		VignetScale(VignetScale_),
		invxRes(1.f / xResolution_),
		invyRes(1.f / yResolution_)
	{}

	void operator()()
	{
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
	}
};
// Is this planned in further use ? Silence "unused function" warning for now
/*
static float Blackman(float nx) {
	const float a = 0.16f;
	const float a0 = (1.f - a) / 2.f;
	const float a1 = 1.f / 2.f;
	const float a2 = a / 2.f;

	return a0 - a1 * cosf(2.f * M_PI * nx) + a2 * cosf(4.f * M_PI * nx);
}
*/
static float Hanning(float nx) {
	const float a = 0.5f;
	const float b = 0.5f;

	return a - b * cosf(2.f * M_PI * nx);
}


// pointwise complex multiplication between F and G, result in R
static void fft_complex_mult(fftw_complex *F, fftw_complex *G, u_int width, u_int height, fftw_complex *R) {
	// frequency-space product
	const u_int c_width = width / 2 + 1; // based on how FFTW stores complex data
	for (u_int y = 0; y < height; ++y) {
		for (u_int x = 0; x < c_width; ++x) {
			const size_t idx = x + y * c_width;
			const std::complex<double> f(F[idx][0], F[idx][1]);
			const std::complex<double> g(G[idx][0], G[idx][1]);
			const std::complex<double> p = f * g;
			R[idx][0] = p.real();
			R[idx][1] = p.imag();
		}
	}
}

class fft_convolution_2d {
public:
	u_int src_w;
	u_int src_h;
	u_int kernel_w;
	u_int kernel_h;
	u_int fft_w;
	u_int fft_h;
	fftw_complex *src_f;
	fftw_complex *kernel_f;	
	cimg_library::CImg<double> src_fft;
	cimg_library::CImg<double> dst_fft;
	fftw_plan plan_forward_src;
	fftw_plan plan_backward;
	
	fft_convolution_2d(u_int src_width, u_int src_height, u_int kernel_width, u_int kernel_height)
		: src_w(src_width), src_h(src_height), kernel_w(kernel_width), kernel_h(kernel_height),
		fft_w(src_width + kernel_width / 2), fft_h(src_height + kernel_height / 2),
		src_fft(fft_w, fft_h), dst_fft(fft_w, fft_h)
	{
		// allocate storage for transformed src and kernel
		src_f = (fftw_complex *)fftw_alloc_complex(fft_h * (fft_w / 2 + 1));
		kernel_f = (fftw_complex *)fftw_alloc_complex(fft_h * (fft_w / 2 + 1));

		// initialize plans
		plan_forward_src = fftw_plan_dft_r2c_2d(fft_h, fft_w, src_fft.data, src_f, FFTW_ESTIMATE);

		// the backward FFT takes src_f as input, as it is overwritten during backward transform
		plan_backward = fftw_plan_dft_c2r_2d(fft_h, fft_w, src_f, dst_fft.data, FFTW_ESTIMATE);
	}

	~fft_convolution_2d() {
		fftw_free(kernel_f);
		fftw_free(src_f);

		fftw_destroy_plan(plan_forward_src);
		fftw_destroy_plan(plan_backward);
	}

	void init_kernel(const cimg_library::CImg<double> &kernel) {
		// compose periodic signal
		cimg_library::CImg<double> kernel_fft(fft_w, fft_h);
		kernel_fft.fill(0.0);
		kernel_fft.draw_image(kernel, 0, 0);

		// transform kernel
		fftw_plan plan_forward_kernel = fftw_plan_dft_r2c_2d(fft_h, fft_w, kernel_fft.data, kernel_f, FFTW_ESTIMATE);
		fftw_execute(plan_forward_kernel);
		fftw_destroy_plan(plan_forward_kernel);
	}

	// convolves src with the kernel, output in dst
	// both have same dimensions and both can be the same image
	void convolve(const cimg_library::CImg<double> &src, cimg_library::CImg<double> &dst) {
		src_fft.fill(0.0);
		dst_fft.fill(0.0);

		// compose periodic signal
		src_fft.draw_image(src, 0, 0);

		// transform src
		fftw_execute(plan_forward_src);

		// frequency-space product, put result in src_f
		// as it will be overwritten by the backwards transform anyway
		fft_complex_mult(src_f, kernel_f, fft_w, fft_h, src_f);

		fftw_execute(plan_backward);

		const u_int offset_w = kernel_w / 2;
		const u_int offset_h = kernel_h / 2;

		const double norm = 1.0 / (fft_w * fft_h);

		// blank output just in case
		dst.fill(0.0);

		for(u_int y = 0; y < src_h; ++y) {
			for(u_int x = 0 ; x < src_w; ++x) {
				double v = norm * dst_fft(x + offset_w, y + offset_h);
				dst(x, y) = (v > 0.0) ? v : 0.0;
			}
		}
	}
};

// Image Pipeline Function Definitions
void ApplyImagingPipeline(vector<XYZColor> &xyzpixels, u_int xResolution, u_int yResolution,
	const GREYCStorationParams &GREYCParams, const ChiuParams &chiuParams,
	const ColorSystem &colorSpace, Histogram *histogram, bool HistogramEnabled,
	bool &haveBloomImage, XYZColor *&bloomImage, bool bloomUpdate,
	float bloomRadius, float bloomWeight,
	bool VignettingEnabled, float VignetScale,
	bool aberrationEnabled, float aberrationAmount,
	bool &haveGlareImage, XYZColor *&glareImage, bool glareUpdate,
	float glareAmount, float glareRadius, u_int glareBlades,
	float glareThreshold, bool glareMap, const string &glarePupilFilename,
	const string &glareLashesFilename,
	const char *toneMapName, const ParamSet *toneMapParams,
	const CameraResponse *response, float dither)
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
			vector<float> bloomFilter(2*bloomWidth * bloomWidth+1);
			for (u_int i = 0; i < bloomWidth * bloomWidth; ++i) {
				// zeros of J_1
				const float z0 = 3.8317f;
				//const float z1 = 7.0156f;
				//const float z2 = 10.1735;
				const float dist = z0 * sqrtf(i) / bloomWidth;
				if (dist == 0.f)
					bloomFilter[i] = 1.f;
				else if (dist >= z0)
					bloomFilter[i] = 0.f;
				else {
					// airy function
					//const float b = boost::math::cyl_bessel_j(1, dist);
					//bloomFilter[i] = powf(2*b/dist, 2.f);

					// gaussian approximation
					// best-fit sigma^2 for above airy function, based on RMSE
					// depends on choice of zero
					const float sigma2 = 1.698022698724f; 
					bloomFilter[i] = exp(-dist*dist/sigma2);
				}
			}

			// Allocate persisting bloom image layer if unallocated
			if(!haveBloomImage) {
				bloomImage = new XYZColor[nPix];
				haveBloomImage = true;
			}

			for (u_int i = 0; i < nPix; ++i)
				bloomImage[i] = XYZColor(0.f);

			//BloomFilter(xResolution, yResolution, bloomWidth, bloomFilter, bloomImage, xyzpixels)();

			// apply separable filter
			BloomFilterX(xResolution, yResolution, bloomWidth, bloomFilter, bloomImage, &xyzpixels[0])();
			BloomFilterY(xResolution, yResolution, bloomWidth, bloomFilter, bloomImage, bloomImage)();
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
			for(u_int i = 0; i < nPix; ++i)
				glareImage[i] = XYZColor(0.f);

			// Search for the brightest pixel in the image
			float maxY = 0;
			for(u_int i = 1; i < nPix; ++i) {
				if (xyzpixels[i].Y() > maxY)
					maxY = xyzpixels[i].Y();
			}
			
			// glareThreshold ranges between 0-1,
			// but this relative value has to be converted to
			//an absolute value fitting the image being processed
			float glareAbsoluteThreshold = maxY * glareThreshold;

			cimg_library::CImg<double> pupil;
			cimg_library::CImg<double> eyelashes;
			if (glareMap) {
				try {
					pupil.assign(glarePupilFilename.c_str());
					eyelashes.assign(glareLashesFilename.c_str());
				} catch(CImgException &e) {
					LOG(LUX_WARNING, LUX_BADFILE) << "Error loading glare files, falling to classic mode." << e.message;
					glareMap = false;
				}
			}
			if (glareMap) {
				const int nc = 512;
				const int nr = 512;
				const int nout = nr * (nc / 2 + 1);
				// Resize the pupil and eye lashes pictures
				// for easier FFT computation
				pupil.resize(nc, nr);
				eyelashes.resize(nc, nr);
				// Compose the pupil and eye lashes pictures
				cimg_library::CImg<double> composition(pupil);
				for (u_int i = 0; i < composition.size(); ++i) {
					if (pupil[i] == 0.0)
						composition[i] = 0.0;
					else
						composition[i] = eyelashes[i];
				}
				// Compute the 2D FFT of the composed map
				fftw_complex *out = (fftw_complex *)fftw_alloc_complex(nout);
				fftw_plan p = fftw_plan_dft_r2c_2d(nr, nc, composition.data, out, FFTW_ESTIMATE);
				fftw_execute(p);
				fftw_destroy_plan(p);

				// Compute the spectrum of the FFT
				cimg_library::CImg<double> spect(nc / 2, nr - 1);
				for (u_int y = 0; y < nr - 1; ++y) {
					for (u_int x = 0; x < nc / 2; ++x) {
						const fftw_complex &c = out[x + 1 + (y + 1) * (nc / 2 + 1)];
						spect(x, y) = c[0] * c[0] + c[1] * c[1];
					}
				}

				// Recompose the glare spectrum
				cimg_library::CImg<double> spectr(nc + 1, nr + 1);
				for (u_int y = 0; y < nr + 1; ++y) {
					for (u_int x = 0; x < nc + 1; ++x) {
						if (y < nr / 2) {
							if (x < nc / 2)
								spectr(x, y) = spect(nc / 2 - 1 - x, nr / 2 - 1 - y);
							else if (x > nc / 2)
								spectr(x, y) = spect(x - 1 - nc / 2, nr / 2 - 1 + y);
							else {
								const fftw_complex &c = out[(nr / 2 + y) * (nc / 2 + 1)];
								spectr(x, y) = c[0] * c[0] + c[1] * c[1];
							}
						} else if (y > nr / 2 ) {
							if (x < nc / 2)
								spectr(x, y) = spect(nc / 2 - 1 - x, nr + nr / 2 - 1 - y);
							else if (x > nc / 2)
								spectr(x, y) = spect(x - 1 - nc / 2, y - nr / 2 - 1);
							else {
								const fftw_complex &c = out[(y - nr / 2) * (nc / 2 + 1)];
								spectr(x, y) = c[0] * c[0] + c[1] * c[1];
							}
						} else {
							if (x < nc / 2) {
								const fftw_complex &c = out[nc / 2 - x];
								spectr(x, y) = c[0] * c[0] + c[1] * c[1];
							} else if (x > nc / 2) {
								const fftw_complex &c = out[x - nc / 2];
								spectr(x, y) = c[0] * c[0] + c[1] * c[1];
							} else {
								const fftw_complex &c = out[0];
								spectr(x, y) = c[0] * c[0] + c[1] * c[1];
							}
						}
					}
				}
				fftw_free(out);

				// windowing
				for (u_int y = 0; y < nr + 1; ++y) {
					const float wy = Hanning(y * (1.f / nr));
					for (u_int x = 0; x < nc + 1; ++x) {
						const float wx = Hanning(x * (1.f / nc));
						spectr(x, y) = wy * wx * spectr(x, y);
					}
				}

				// Normalize the spectrum, this is now our kernel so it should sum to one
				spectr /= spectr.sum();

				// convolve with glare kernel

				fft_convolution_2d fft_conv(xResolution, yResolution, spectr.width, spectr.height);

				fft_conv.init_kernel(spectr);

				cimg_library::CImg<double> glare_tmp(xResolution, yResolution);

				for (u_int channel = 0; channel < 3; channel++) {
					// initialize glare_src
					for (u_int y = 0; y < yResolution; ++y) {
						for (u_int x = 0; x < xResolution; ++x) {
							const XYZColor &pix = xyzpixels[x + y * xResolution];
							const int d = (pix.Y() >= glareAbsoluteThreshold) ? 1 : 0;
							glare_tmp(x, y) = pix.c[channel] * d;
						}
					}

					// normalize source, should increase precision
					double glare_scale = glare_tmp.max();
					glare_tmp *= 1.0 / glare_scale;

					// perform convolution
					fft_conv.convolve(glare_tmp, glare_tmp);

					// Fill the glare layer
					for (u_int y = 0; y < yResolution; ++y) {
						for (u_int x = 0; x < xResolution; ++x) {
							const double v = glare_scale * glare_tmp(x, y);
							glareImage[x + y * xResolution].c[channel] = v;
						}
					}

				}
			} else {
				u_int maxRes = max(xResolution, yResolution);
				u_int nPix2 = maxRes * maxRes;

				std::vector<XYZColor> rotatedImage(nPix2);
				std::vector<XYZColor> blurredImage(nPix2);
				std::vector<XYZColor> darkenedImage(nPix);

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
			glareUpdate = false;
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

		// VignettingFilter
		VignettingFilter(xResolution, yResolution, aberrationEnabled, aberrationAmount, outp, rgbpixels, VignettingEnabled, VignetScale)();

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
}


// Filter Look Up Table Definitions

FilterLUT::FilterLUT(Filter *filter, const float offsetX, const float offsetY) {
	const int x0 = Ceil2Int(offsetX - filter->xWidth);
	const int x1 = Floor2Int(offsetX + filter->xWidth);
	const int y0 = Ceil2Int(offsetY - filter->yWidth);
	const int y1 = Floor2Int(offsetY + filter->yWidth);
	lutWidth = max(1, x1 - x0 + 1);
	lutHeight = max(1, y1 - y0 + 1);
	//lut = new float[lutWidth * lutHeight];
	lut.resize(lutWidth * lutHeight);

	float totalWeight = 0.f;
	unsigned int index = 0;
	// if the whole filter support lies within the pixel
	// x0>x1 or y0>y1, this requires special treatment to avoid blackness
	if (y1 >= y0) {
		for (int iy = y0; iy <= y1; ++iy) {
			if (x1 >= x0) {
				for (int ix = x0; ix <= x1; ++ix) {
					const float filterVal = filter->Evaluate(fabsf(ix - offsetX), fabsf(iy - offsetY));
					totalWeight += filterVal;
					lut[index++] = filterVal;
				}
			} else {
				const float filterVal = filter->Evaluate(0.f, fabsf(iy - offsetY));
				totalWeight += filterVal;
				lut[index++] = filterVal;
			}
		}
	} else {
		if (x1 >= x0) {
			for (int ix = x0; ix <= x1; ++ix) {
				const float filterVal = filter->Evaluate(fabsf(ix - offsetX), 0.f);
				totalWeight += filterVal;
				lut[index++] = filterVal;
			}
		} else {
			const float filterVal = filter->Evaluate(0.f, 0.f);
			totalWeight += filterVal;
			lut[index++] = filterVal;
		}
	}

	// Normalize LUT
	if (totalWeight > 0.f) {
		for (u_int i = 0; i < lut.size(); ++i)
			lut[i] /= totalWeight;
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

// OutlierData Definitions
ColorSystem OutlierData::cs(0.63f, 0.34f, 0.31f, 0.595f, 0.155f, 0.07f, 0.314275f, 0.329411f);

void BufferGroup::CreateBuffers(const vector<BufferConfig> &configs, u_int x, u_int y) {
	for(vector<BufferConfig>::const_iterator config = configs.begin(); config != configs.end(); ++config) {
		Buffer *buffer;
		switch ((*config).type) {
		case BUF_TYPE_PER_PIXEL:
			buffer = new PerPixelNormalizedBuffer(x, y);
			break;
		case BUF_TYPE_PER_SCREEN:
			buffer = new PerScreenNormalizedBuffer(x, y, &numberOfSamples);
			break;
		case BUF_TYPE_PER_SCREEN_SCALED:
			buffer = new PerScreenNormalizedBufferScaled(x, y, &numberOfSamples);
			break;
		case BUF_TYPE_RAW:
			buffer = new RawBuffer(x, y);
			break;
		default:
			buffer = NULL;
			assert(0);
		}
		if (buffer && buffer->xPixelCount && buffer->yPixelCount)
			buffers.push_back(buffer);
		else {
			LOG(LUX_SEVERE, LUX_NOMEM) << "Couldn't allocate film buffers, aborting";
			assert(0);
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

u_int Film::GetXPixelStart()
{
	return xPixelStart;
}

u_int Film::GetYPixelStart()
{
	return yPixelStart;
}

u_int Film::GetXPixelCount()
{
	return xPixelCount;
}

u_int Film::GetYPixelCount()
{
	return yPixelCount;
}

Film::Film(u_int xres, u_int yres, Filter *filt, u_int filtRes, const float crop[4], 
		   const string &filename1, bool premult, bool useZbuffer,
		   bool w_resume_FLM, bool restart_resume_FLM, bool write_FLM_direct,
		   int haltspp, int halttime, float haltthreshold,
		   bool debugmode, int outlierk, int tilec, const string &samplingmapfilename) :
	Queryable("film"),
	xResolution(xres), yResolution(yres),
	EV(0.f), averageLuminance(0.f),
	numberOfSamplesFromNetwork(0), numberOfLocalSamples(0), numberOfResumedSamples(0),
	contribPool(NULL), filter(filt), filterTable(NULL), filterLUTs(NULL),
	filename(filename1),
	colorSpace(0.63f, 0.34f, 0.31f, 0.595f, 0.155f, 0.07f, 0.314275f, 0.329411f), // default is SMPTE
	convTest(NULL), varianceBuffer(NULL),
	noiseAwareMapVersion(0),
	userSamplingMapFileName(samplingmapfilename), userSamplingMapVersion(0),
	ZBuffer(NULL), use_Zbuf(useZbuffer),
	debug_mode(debugmode), premultiplyAlpha(premult),
	writeResumeFlm(w_resume_FLM), restartResumeFlm(restart_resume_FLM), writeFlmDirect(write_FLM_direct),
	outlierRejection_k(outlierk), haltSamplesPerPixel(haltspp),
	haltTime(halttime), haltThreshold(haltthreshold), haltThresholdComplete(0.f),
	histogram(NULL), enoughSamplesPerPixel(false)
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

	boost::xtime_get(&creationTime, boost::TIME_UTC_);

	//Queryable parameters
	AddIntAttribute(*this, "xResolution", "Horizontal resolution (pixels)", &Film::GetXResolution);
	AddIntAttribute(*this, "yResolution", "Vertical resolution (pixels)", &Film::GetYResolution);
	AddIntAttribute(*this, "xPixelCount", "Horizontal pixel count (after cropping)", &Film::GetXPixelCount);
	AddIntAttribute(*this, "yPixelCount", "Vertical pixel count (after cropping)", &Film::GetYPixelCount);
	AddBoolAttribute(*this, "premultiplyAlpha", "Premultiplied alpha enabled", &Film::premultiplyAlpha);
	AddStringAttribute(*this, "filename", "Output filename", filename, &Film::filename, Queryable::ReadWriteAccess);
	AddFloatAttribute(*this, "EV", "Exposure value", &Film::EV);
	AddFloatAttribute(*this, "averageLuminance", "Average Image Luminance", &Film::averageLuminance);
	AddDoubleAttribute(*this, "numberOfLocalSamples", "Number of samples contributed to film on the local machine", &Film::numberOfLocalSamples);
	AddDoubleAttribute(*this, "numberOfSamplesFromNetwork", "Number of samples contributed from network slaves", &Film::numberOfSamplesFromNetwork);
	AddDoubleAttribute(*this, "numberOfResumedSamples", "Number of samples loaded from saved film", &Film::numberOfResumedSamples);
	AddBoolAttribute(*this, "enoughSamples", "Indicates if the halt condition been reached", &Film::enoughSamplesPerPixel);
	AddIntAttribute(*this, "haltSamplesPerPixel", "Halt Samples per Pixel", haltSamplesPerPixel, &Film::haltSamplesPerPixel, Queryable::ReadWriteAccess);
	AddIntAttribute(*this, "haltTime", "Halt time in seconds", haltTime, &Film::haltTime, Queryable::ReadWriteAccess);
	AddFloatAttribute(*this, "haltThreshold", "Halt threshold", haltThreshold, &Film::haltThreshold, Queryable::ReadWriteAccess);
	AddFloatAttribute(*this, "haltThresholdComplete", "Halt threshold complete", &Film::haltThresholdComplete);
	AddBoolAttribute(*this, "writeResumeFlm", "Write resume file", writeResumeFlm, &Film::writeResumeFlm, Queryable::ReadWriteAccess);
	AddBoolAttribute(*this, "restartResumeFlm", "Restart (overwrite) resume file", restartResumeFlm, &Film::restartResumeFlm, Queryable::ReadWriteAccess);
	AddBoolAttribute(*this, "writeFlmDirect", "Write resume file directly to disk", writeFlmDirect, &Film::writeFlmDirect, Queryable::ReadWriteAccess);	
	AddFloatAttribute(*this, "cropWindow.0", "Crop window 0", &Film::GetCropWindow0);
	AddFloatAttribute(*this, "cropWindow.1", "Crop window 1", &Film::GetCropWindow1);
	AddFloatAttribute(*this, "cropWindow.2", "Crop window 2", &Film::GetCropWindow2);
	AddFloatAttribute(*this, "cropWindow.3", "Crop window 3", &Film::GetCropWindow3);

	// Precompute filter tables
	filterLUTs = new FilterLUTs(filt, max(min(filtRes, 64u), 2u));

	outlierCellWidth = max(1U, Floor2UInt(2 * filter->xWidth));
	outlierInvCellWidth = 1.f / outlierCellWidth;
	outlierCellHeight = max(1U, Floor2UInt(2 * filter->yWidth));
	outlierInvCellHeight = 1.f / outlierCellHeight;

	const u_int thread_count = boost::thread::hardware_concurrency();
	// base min tile size on outlier cell height, as it's a good measure anyway
	const u_int minTileHeight = outlierCellHeight;
	if (tilec > 0)
		tileCount = tilec;
	else
		// TODO - thread_count * 2 is fairly arbitrary, find better choice?
		tileCount = thread_count * (tilec < 0 ? -tilec : 2);
	
	LOG(LUX_DEBUG, LUX_NOERROR) << "Requested film tile count: " << tileCount;

	tileCount = Clamp(tileCount, 1u, yRealHeight / minTileHeight);
	//tileCount = 2;

	tileHeight = max(Ceil2UInt(static_cast<float>(yRealHeight) / tileCount), minTileHeight);
	if (outlierRejection_k > 0) {
		// if outlier rejection is enabled, tileHeight must be multiple of outlierCellHeight
		// increase tileHeight to ensure this
		tileHeight = outlierCellHeight * max(Ceil2UInt(static_cast<float>(tileHeight) / outlierCellHeight), 1u);
		tileCount = max(Ceil2UInt(static_cast<float>(yRealHeight) / tileHeight), 1u);
	}

	LOG(LUX_DEBUG, LUX_NOERROR) << "Actual film tile count: " << tileCount;

	invTileHeight = 1.f / tileHeight;
	tileOffset = -0.5f - filter->yWidth - yPixelStart;
	tileOffset2 = 2 * filter->yWidth * invTileHeight;

	if (outlierRejection_k > 0) {
		const u_int outliers_width = xRealWidth / outlierCellWidth;
		const u_int outliers_height = yRealHeight / outlierCellHeight;
		outliers.resize(outliers_height);
		for (size_t i = 0; i < outliers.size(); ++i)
			outliers[i].resize(outliers_width);
		// tiles need duplicate data for row above and below tile
		tileborder_outliers.resize(2 * tileCount);
		for (size_t i = 0; i < tileborder_outliers.size(); ++i)
			tileborder_outliers[i].resize(outliers_width);
	}

	//--------------------------------------------------------------------------
	// Check if there is a user sampling map to load
	//--------------------------------------------------------------------------

	if (userSamplingMapFileName != "") {
		if (boost::filesystem::exists(userSamplingMapFileName)) {
			LOG(LUX_DEBUG, LUX_NOERROR) << "Loading user sampling map from file: " << userSamplingMapFileName;

			ImageData *imgData = ReadImage(userSamplingMapFileName);
			bool validImageData = true;
			if ((imgData->getWidth() != xPixelCount) || (imgData->getHeight() != yPixelCount)) {
				LOG(LUX_WARNING, LUX_BADFILE) << "User sampling map size doesn't match Film size";
				validImageData = false;
			}
			
			if (imgData->getChannels() != 1) {
				LOG(LUX_WARNING, LUX_BADFILE) << "User sampling map must be a single channel image";
				validImageData = false;
			}

			if (imgData->getPixelDataType() != ImageData::FLOAT_TYPE) {
				LOG(LUX_WARNING, LUX_BADFILE) << "User sampling map must be an image with float data";
				validImageData = false;
			}
			
			if (validImageData)
				SetUserSamplingMap((float *)imgData->getData());	
			else {
				// Ignore the map and avoid to overwrite the file
				userSamplingMapFileName = "";
			}

			delete imgData;
		} else {
			LOG(LUX_DEBUG, LUX_NOERROR) << "Setting user sampling map to file: " << userSamplingMapFileName;
		}
	}
}

Film::~Film()
{
	delete filterLUTs;
	delete filter;
	delete ZBuffer;
	delete convTest;
	delete varianceBuffer;
	delete histogram;
	delete contribPool;
}

void Film::EnableNoiseAwareMap() {
	varianceBuffer = new VarianceBuffer(xPixelCount, yPixelCount);
	varianceBuffer->Clear();

	noiseAwareMap.reset(new float[xPixelCount * yPixelCount]);
	std::fill(noiseAwareMap.get(), noiseAwareMap.get() + xPixelCount * yPixelCount, 1.f);
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
		bufferGroups[i].CreateBuffers(bufferConfigs, xPixelCount, yPixelCount);

	// Allocate ZBuf buffer if needed
	if (use_Zbuf)
		ZBuffer = new PerPixelNormalizedFloatBuffer(xPixelCount, yPixelCount);

	// initialize the contribution pool
	// needs to be done before anyone tries to lock it
	contribPool = new ContributionPool(this);

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
		} else
			numberOfResumedSamples = MergeFilmFromFile(fname);
    }

	// Enable convergence test if needed
	// NOTE: TVI is a side product of convergence test so I need to run the
	// test even if halttreshold is not used
	if ((haltThreshold >= 0.f) || noiseAwareMap) {
		convTest = new slg::ConvergenceTest(xPixelCount, yPixelCount);

		if (noiseAwareMap)
			convTest->NeedTVI();
	}

	// DEBUG: for testing the user sampling map functionality
	/*float *map = (float *)alloca(xPixelCount * yPixelCount * sizeof(float));
	for (u_int x = 0; x < xPixelCount; ++x) {
		for (u_int y = 0; y < yPixelCount; ++y) {
			const u_int index = x + y * xPixelCount;

			const float xx = (x / (float)xPixelCount) - 0.5f;
			const float yy = (y / (float)yPixelCount) - 0.5f;
			map[index] = (xx * xx + yy * yy < .25f * .25f) ? 1.f : 0.1f;

			//const float xx = ((x % 80) / 80.f) - 0.5f;
			//const float yy = ((y % 80) / 80.f) - 0.5f;
			//map[index] = (xx * xx + yy * yy < .4f * .4f) ? 1.f : 0.1f;
			
			//map[index] = (x > xPixelCount * 7 / 8 && y > yPixelCount * 7 / 8) ? 1.f : 0.01f;
		}
	}
	SetUserSamplingMap(map);*/
}

void Film::ClearBuffers()
{
	for (u_int i = 0; i < bufferGroups.size(); ++i) {

		BufferGroup& bufferGroup = bufferGroups[i];

		for (u_int j = 0; j < bufferConfigs.size(); ++j) {
			Buffer* buffer = bufferGroup.getBuffer(j);

			buffer->Clear();
		}

		bufferGroup.numberOfSamples = 0;
	}
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

	// Reset the convergence test
	if (convTest) {
		boost::mutex::scoped_lock(write_mutex);
		convTest->Reset();
	}
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

	// Reset the convergence test
	if (convTest) {
		boost::mutex::scoped_lock(write_mutex);
		convTest->Reset();
	}
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

	// Reset the convergence test
	if (convTest) {
		boost::mutex::scoped_lock(write_mutex);
		convTest->Reset();
	}
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

	// Reset the convergence test
	if (convTest) {
		boost::mutex::scoped_lock(write_mutex);
		convTest->Reset();
	}
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

void Film::AddSampleCount(const double count) {
	if (haltTime > 0) {
		// Check if we have met the enough rendering time condition
		boost::xtime t;
		boost::xtime_get(&t, boost::TIME_UTC_);
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


std::vector<Film::OutlierAccel>& Film::GetOutlierAccelRow(u_int oY, u_int tileIndex, u_int tileStart, u_int tileEnd)
{
	if (oY < tileStart) {
		// above currrent tile
		return tileborder_outliers[2 * tileIndex];
	} else if (oY >= tileEnd) {
		// below current tile
		return tileborder_outliers[2 * tileIndex + 1];
	}

	// inside current tile
	return outliers[oY];
}

void Film::RejectTileOutliers(const Contribution &contrib, u_int tileIndex, int yTilePixelStart, int yTilePixelEnd)
{
	// outlier rejection
	const float fnormTileStart = (yTilePixelStart + filter->yWidth) * outlierInvCellHeight;
	const float fnormTileEnd   = (yTilePixelEnd   + filter->yWidth) * outlierInvCellHeight;

	const u_int tileStart = static_cast<u_int>(max(0, min(Floor2Int(fnormTileStart), static_cast<int>(outliers.size() - 1))));
	const u_int tileEnd =   static_cast<u_int>(max(0, min(Floor2Int(fnormTileEnd),   static_cast<int>(outliers.size() - 1))));

	// filter-normalized pixel coordinates
	const float fnormX = (contrib.imageX - 0.5f + filter->xWidth) * outlierInvCellWidth;
	const float fnormY = (contrib.imageY - 0.5f + filter->yWidth) * outlierInvCellHeight;

	OutlierData sd(fnormX, fnormY, contrib.color);

	// perform lookup based on original position
	// constrain to tile only if we need to add the outlier
	const int oY = max(0, min(Floor2Int(fnormY), static_cast<int>(outliers.size() - 1)));

	std::vector<OutlierAccel> &outlierRow = GetOutlierAccelRow(oY, tileIndex, tileStart, tileEnd);
	const int oX = max(0, min(Floor2Int(fnormX), static_cast<int>(outlierRow.size() - 1)));
	OutlierAccel &outlierAccel = outlierRow[oX];

	NearSetPointProcess<OutlierData::Point_t> proc(outlierRejection_k);
	vector<ClosePoint<OutlierData::Point_t> > closest(outlierRejection_k);
	proc.points = &closest[0];

	float maxDist = INFINITY;

	outlierAccel.Lookup(sd.p, proc, maxDist);

	float kmeandist = 0.f;
	for (u_int i = 0; i < proc.foundPoints; ++i)
		kmeandist += proc.points[i].distance;
	
	//kmeandist /= proc.foundPoints;
		
	if (proc.foundPoints < 1 || kmeandist > proc.foundPoints) { // kmeandist > 1.f
		// add outlier and return
		// include surrounding cells so we don't have to
		// traverse multiple cells for each lookup
		const u_int oLeft = static_cast<u_int>(max(0, oX - 1));
		const u_int oRight = static_cast<u_int>(min(static_cast<int>(outliers[0].size() - 1), oX + 1));
		const u_int oTop = static_cast<u_int>(max(0, oY - 1));
		const u_int oBottom = static_cast<u_int>(min(static_cast<int>(outliers.size() - 1), oY + 1));

		if (oTop < tileStart || oBottom >= tileEnd) {
			// outlier spans tile borders
			for (u_int i = oTop; i <= oBottom; ++i) {
				std::vector<OutlierAccel> &row = GetOutlierAccelRow(i, tileIndex, tileStart, tileEnd);
				for (u_int j = oLeft; j <= oRight; ++j) {
					row[j].AddNode(sd.p);
				}
			}
		} else {
			// we're all inside one tile
			for (u_int i = oTop; i <= oBottom; ++i) {
				std::vector<OutlierAccel> &row = outliers[i];
				for (u_int j = oLeft; j <= oRight; ++j) {
					row[j].AddNode(sd.p);
				}
			}
		}
		// outlier, reject
		contrib.variance = -1.f;
	}
	// not an outlier, splat
}

u_int Film::GetTileCount() const {
	return tileCount;
}

u_int Film::GetTileIndexes(const Contribution &contrib, u_int *tile0, u_int *tile1) const {
	const float imageY = contrib.imageY + tileOffset;

	const float tileY = imageY * invTileHeight;

	*tile0 = static_cast<u_int>(Clamp(static_cast<int>(tileY), 0, static_cast<int>(tileCount-1)));
	*tile1 = *tile0 + 1;

	if (*tile1 >= tileCount || tileY + tileOffset2 < *tile1)
		return 1u;

	return 2u;
}

void Film::GetTileExtent(u_int tileIndex, int *xstart, int *xend, int *ystart, int *yend) const {
	*xstart = xPixelStart;
	*xend = xPixelStart + xPixelCount;
	*ystart = yPixelStart + min(tileIndex * tileHeight, yPixelCount);
	*yend = yPixelStart + min((tileIndex+1) * tileHeight, yPixelCount);
}

void Film::AddTileSamples(const Contribution* const contribs, u_int num_contribs,
		u_int tileIndex) {
	int xTilePixelStart, xTilePixelEnd;
	int yTilePixelStart, yTilePixelEnd;
	GetTileExtent(tileIndex, &xTilePixelStart, &xTilePixelEnd, &yTilePixelStart, &yTilePixelEnd);

	for (u_int ci = 0; ci < num_contribs; ci++) {
		const Contribution &contrib(contribs[ci]);

		XYZColor xyz = contrib.color;
		const float alpha = contrib.alpha;

		// Issue warning if unexpected radiance value returned
		if (!(xyz.Y() >= 0.f) || isinf(xyz.Y())) {
			if(debug_mode) {
				LOG(LUX_WARNING,LUX_LIMIT) << "Out of bound intensity in Film::AddTileSamples: "
				   << xyz.Y() << ", sample discarded";
			}
			continue;
		}

		if (!(alpha >= 0.f) || isinf(alpha)) {
			if(debug_mode) {
				LOG(LUX_WARNING,LUX_LIMIT) << "Out of bound  alpha in Film::AddTileSamples: "
				   << alpha << ", sample discarded";
			}
			continue;
		}
	
		if (outlierRejection_k > 0) {
			// reject outliers by setting their weight (variance field) to -1
			RejectTileOutliers(contrib, tileIndex, yTilePixelStart, yTilePixelEnd);
		}

		const float weight = contrib.variance;

		// negative weight means sample was rejected
		if (!(weight >= 0.f) || isinf(weight)) {
			if(debug_mode && (weight >= 0.f)) {
				LOG(LUX_WARNING,LUX_LIMIT) << "Out of bound  weight in Film::AddTileSamples: "
				   << weight << ", sample discarded";
			}
			continue;
		}

		if (premultiplyAlpha)
			xyz *= alpha;

		BufferGroup &currentGroup = bufferGroups[contrib.bufferGroup];
		Buffer *buffer = currentGroup.getBuffer(contrib.buffer);

		// Compute sample's raster extent
		float dImageX = contrib.imageX - 0.5f;
		float dImageY = contrib.imageY - 0.5f;

		// Get filter coefficients
		const FilterLUT &filterLUT = 
			filterLUTs->GetLUT(dImageX - Floor2Int(contrib.imageX), dImageY - Floor2Int(contrib.imageY));
		const float *lut = filterLUT.GetLUT();

		int x0 = Ceil2Int (dImageX - filter->xWidth);
		int x1 = x0 + filterLUT.GetWidth();
		int y0 = Ceil2Int (dImageY - filter->yWidth);
		int y1 = y0 + filterLUT.GetHeight();
		if (x1 < x0 || y1 < y0 || x1 < 0 || y1 < 0)
			continue;

		const u_int xStart = static_cast<u_int>(max(x0, xTilePixelStart));
		const u_int yStart = static_cast<u_int>(max(y0, yTilePixelStart));
		const u_int xEnd = static_cast<u_int>(min(x1, xTilePixelEnd));
		const u_int yEnd = static_cast<u_int>(min(y1, yTilePixelEnd));

		for (u_int y = yStart; y < yEnd; ++y) {
			const int yoffset = (y - y0) * filterLUT.GetWidth();
			const u_int yPixel = y - yPixelStart;
			for (u_int x = xStart; x < xEnd; ++x) {
				// Evaluate filter value at $(x,y)$ pixel
				const int xoffset = x-x0;
				const float filterWt = lut[yoffset + xoffset];

				// Update pixel values with filtered sample contribution
				const u_int xPixel = x - xPixelStart;
				const float w = filterWt * weight;
				buffer->Add(xPixel, yPixel, xyz, alpha, w);

				// Update ZBuffer values with filtered zdepth contribution
				if(use_Zbuf && contrib.zdepth != 0.f)
					ZBuffer->Add(xPixel, yPixel, contrib.zdepth, 1.0f);

				// Update variance information
				if (varianceBuffer)
					varianceBuffer->Add(xPixel, yPixel, xyz, w);
			}
		}
	}
}

void Film::AddSample(Contribution *contrib) {
	u_int tileIndex0, tileIndex1;
	u_int tiles = GetTileIndexes(*contrib, &tileIndex0, &tileIndex1);
	AddTileSamples(contrib, 1, tileIndex0);
	if (tiles > 1)
		AddTileSamples(contrib, 1, tileIndex1);
}

void Film::SetSample(const Contribution *contrib) {
	XYZColor xyz = contrib->color;
	const float alpha = contrib->alpha;
	const float weight = contrib->variance;
	const int x = static_cast<int>(contrib->imageX);
	const int y = static_cast<int>(contrib->imageY);

	if (x < static_cast<int>(xPixelStart) || x >= static_cast<int>(xPixelStart + xPixelCount) ||
		y < static_cast<int>(yPixelStart) || y >= static_cast<int>(yPixelStart + yPixelCount)) {
		if(debug_mode) {
			LOG(LUX_WARNING, LUX_LIMIT) << "Out of bound pixel coordinates in Film::SetSample: ("
					<< x << ", " << y << "), sample discarded";
		}
		return;
	}

	// Issue warning if unexpected radiance value returned
	if (!(xyz.Y() >= 0.f) || isinf(xyz.Y())) {
		if(debug_mode) {
			LOG(LUX_WARNING, LUX_LIMIT) << "Out of bound intensity in Film::SetSample: "
			   << xyz.Y() << ", sample discarded";
		}
		return;
	}

	if (!(alpha >= 0.f) || isinf(alpha)) {
		if(debug_mode) {
			LOG(LUX_WARNING, LUX_LIMIT) << "Out of bound  alpha in Film::SetSample: "
			   << alpha << ", sample discarded";
		}
		return;
	}

	if (!(weight >= 0.f) || isinf(weight)) {
		if(debug_mode) {
			LOG(LUX_WARNING, LUX_LIMIT) << "Out of bound  weight in Film::SetSample: "
			   << weight << ", sample discarded";
		}
		return;
	}

/*FIXME restore the functionality
	// Reject samples higher than max Y() after warmup period
	if (warmupComplete) {
		if (xyz.Y() > maxY)
			return;
	} else {
		maxY = max(maxY, xyz.Y());
		++warmupSamples;
		if (warmupSamples >= reject_warmup_samples)
			warmupComplete = true;
	}
*/

	if (premultiplyAlpha)
		xyz *= alpha;

	BufferGroup &currentGroup = bufferGroups[contrib->bufferGroup];
	Buffer *buffer = currentGroup.getBuffer(contrib->buffer);

	buffer->Set(x - xPixelStart, y - yPixelStart, xyz, alpha, weight);

	// Update ZBuffer values with filtered zdepth contribution
	if(use_Zbuf && contrib->zdepth != 0.f)
		ZBuffer->Set(x - xPixelStart, y - yPixelStart,
			contrib->zdepth, 1.0f);
}

// This is used to add a sample without pixel filtering. It is mostly used by
// SLGRender to update Luxrender Film
void Film::AddSampleNoFiltering(const Contribution *contrib) {
	XYZColor xyz = contrib->color;
	const float alpha = contrib->alpha;
	const float weight = contrib->variance;
	const int x = static_cast<int>(contrib->imageX);
	const int y = static_cast<int>(contrib->imageY);

	if (x < static_cast<int>(xPixelStart) || x >= static_cast<int>(xPixelStart + xPixelCount) ||
		y < static_cast<int>(yPixelStart) || y >= static_cast<int>(yPixelStart + yPixelCount)) {
		if(debug_mode) {
			LOG(LUX_WARNING, LUX_LIMIT) << "Out of bound pixel coordinates in Film::SetSample: ("
					<< x << ", " << y << "), sample discarded";
		}
		return;
	}

	// Issue warning if unexpected radiance value returned
	if (!(xyz.Y() >= 0.f) || isinf(xyz.Y())) {
		if(debug_mode) {
			LOG(LUX_WARNING, LUX_LIMIT) << "Out of bound intensity in Film::SetSample: "
			   << xyz.Y() << ", sample discarded";
		}
		return;
	}

	if (!(alpha >= 0.f) || isinf(alpha)) {
		if(debug_mode) {
			LOG(LUX_WARNING, LUX_LIMIT) << "Out of bound  alpha in Film::SetSample: "
			   << alpha << ", sample discarded";
		}
		return;
	}

	if (!(weight >= 0.f) || isinf(weight)) {
		if(debug_mode) {
			LOG(LUX_WARNING, LUX_LIMIT) << "Out of bound  weight in Film::SetSample: "
			   << weight << ", sample discarded";
		}
		return;
	}

/*FIXME restore the functionality
	// Reject samples higher than max Y() after warmup period
	if (warmupComplete) {
		if (xyz.Y() > maxY)
			return;
	} else {
		maxY = max(maxY, xyz.Y());
		++warmupSamples;
		if (warmupSamples >= reject_warmup_samples)
			warmupComplete = true;
	}
*/

	if (premultiplyAlpha)
		xyz *= alpha;

	BufferGroup &currentGroup = bufferGroups[contrib->bufferGroup];
	Buffer *buffer = currentGroup.getBuffer(contrib->buffer);

	buffer->Add(x - xPixelStart, y - yPixelStart, xyz, alpha, weight);

	// Update ZBuffer values with filtered zdepth contribution
	if(use_Zbuf && contrib->zdepth != 0.f)
		ZBuffer->Add(x - xPixelStart, y - yPixelStart,
			contrib->zdepth, 1.0f);
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
	FLM_PARAMETER_TYPE_STRING = 1,
	FLM_PARAMETER_TYPE_DOUBLE = 2
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
			case FLM_PARAMETER_TYPE_DOUBLE:
				size = 8;
				floatValue = static_cast<double>(aFilm->GetParameterValue(aParam, aIndex));
				break;
			case FLM_PARAMETER_TYPE_STRING:
				stringValue = aFilm->GetStringParameterValue(aParam, aIndex);
				size = stringValue.size();
				break;
			default: {
				LOG(LUX_ERROR,LUX_SYSTEM) << "Invalid parameter type (expected value in [0,2], got=" << type << ")";
				break;
			}
		}
	}

	void Set(Film *aFilm) {
		switch (type) {
			case FLM_PARAMETER_TYPE_FLOAT:
				aFilm->SetParameterValue(id, floatValue, index);
				break;
			case FLM_PARAMETER_TYPE_DOUBLE:
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
			case FLM_PARAMETER_TYPE_DOUBLE:
				if (size != 8) {
					LOG(LUX_ERROR,LUX_SYSTEM) << "Invalid parameter size (expected value for double is 8, received=" << size << ")";
					return false;
				}
				floatValue = osReadLittleEndianDouble(isLittleEndian, is);
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
			case FLM_PARAMETER_TYPE_DOUBLE:
				osWriteLittleEndianDouble(isLittleEndian, os, floatValue);
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
		
	double floatValue;
	string stringValue;
};

class FlmHeader {
public:
	FlmHeader() {}
	bool Read(boost::iostreams::filtering_stream<boost::iostreams::input> &in, bool isLittleEndian, Film *film );
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

bool FlmHeader::Read(boost::iostreams::filtering_stream<boost::iostreams::input> &in, bool isLittleEndian, Film *film ) {
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

bool Film::WriteFilmToFile(const string &filename)
{
	const string tempFilename = filename + ".temp";

	LOG(LUX_INFO, LUX_NOERROR) << "Writing resume film file";

    std::ofstream ofs(tempFilename.c_str(), std::ios_base::out | std::ios_base::binary);
	if(!ofs.good())
	{
		LOG(LUX_ERROR, LUX_SYSTEM) << "Cannot open file '" << tempFilename << "' for writing resume film";
		return false;
	}

	bool writeSuccessful = WriteFilmToStream(ofs, false, true, writeFlmDirect);
	ofs.close();

	if (writeSuccessful)
	{
		try {
			std::string fullFilename = boost::filesystem::system_complete(filename).string();
			//boost::filesystem::path fullFilenamePath(boost::filesystem::system_complete(filename).string());
			//std::string fullFilename = fullFilenamePath.replace_extension("").string();
			//fullFilename.append("-"+boost::lexical_cast<std::string>((int)luxGetDoubleAttribute("renderer_statistics", "elapsedTime"))+"s"+".flm");

			boost::filesystem::rename(tempFilename, fullFilename);
			LOG(LUX_INFO, LUX_NOERROR) << "Resume film written to '" << fullFilename << "'";
		} catch (std::runtime_error &e) {
			LOG(LUX_ERROR, LUX_SYSTEM) << "Failed to rename new resume film, leaving new resume film as '" << tempFilename << "' (" << e.what() << ")";
		}
	}

	return writeSuccessful;
}

bool Film::WriteFilmToStream(
        std::basic_ostream<char> &stream,
        bool clearBuffers,
		bool transmitParams,
		bool directWrite)
{
	bool writeSuccess;

	if (!directWrite) {
		//std::stringstream ss(std::stringstream::in | std::stringstream::out | std::stringstream::binary);
		multibuffer_device mbdev;
		boost::iostreams::stream<multibuffer_device> ms(mbdev);

		writeSuccess = WriteFilmDataToStream(ms, clearBuffers, transmitParams);
		if (writeSuccess)
		{
			ms.seekg(0, BOOST_IOS::beg);
			boost::iostreams::copy(ms, stream);
		}
		else
			LOG(LUX_SEVERE,LUX_SYSTEM) << "Error while preparing film data for transmission, retrying without buffering.";
	}

	// if the memory buffered method fails it's most likely due
	// to low memory conditions, so fall back to direct writing
	if (directWrite || !writeSuccess)
		writeSuccess = WriteFilmDataToStream(stream, clearBuffers, transmitParams);
	
	if (!writeSuccess || !stream.good())
	{
		LOG(LUX_SEVERE, LUX_SYSTEM) << "Error while writing film to stream";
		return false;
	}

	return true;
}

double Film::MergeFilmFromFile(const std::string& filename)
{
	std::ifstream ifs(filename.c_str(), std::ios_base::in | std::ios_base::binary);
	if (!ifs.good())
		return 0;

	LOG(LUX_INFO, LUX_NOERROR) << "Reading resume film from file " << filename;
	return MergeFilmFromStream(ifs);
}

double Film::MergeFilmFromStream(std::basic_istream<char> &stream) {
	const bool isLittleEndian = osIsLittleEndian();
	LOG(LUX_DEBUG, LUX_NOERROR) << "Receiving film (little endian=" << boost::lexical_cast<std::string>(isLittleEndian) << ")";

	// Enable compression
	// TODO Move this below header when implementing FILM VERSION 2
	boost::iostreams::filtering_stream<boost::iostreams::input> in;
	in.push(boost::iostreams::gzip_decompressor());
	in.push(stream);

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

		// lock the pool
		ScopedPoolLock poolLock(contribPool);

		// Dade - add all received data
		for (u_int i = 0; i < bufferGroups.size(); ++i) {
			BufferGroup &currentGroup = bufferGroups[i];
			for (u_int j = 0; j < bufferConfigs.size(); ++j) {
				const BlockedArray<Pixel> *receivedPixels = tmpPixelArrays[ i * bufferConfigs.size() + j ];
				Buffer *buffer = currentGroup.getBuffer(j);

				for (u_int y = 0; y < buffer->yPixelCount; ++y) {
					for (u_int x = 0; x < buffer->xPixelCount; ++x) {
						const Pixel &pixel = (*receivedPixels)(x, y);
						Pixel &pixelResult = buffer->pixels(x, y);
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

		LOG( LUX_DEBUG,LUX_NOERROR) << "Received film with " << totNumberOfSamples << " samples";
	} else
		LOG( LUX_ERROR,LUX_SYSTEM)<< "IO error while receiving film buffers";

	// Clean up
	for (u_int i = 0; i < tmpPixelArrays.size(); ++i)
		delete tmpPixelArrays[i];

	return maxTotNumberOfSamples;
}

bool Film::WriteFilmDataToStream(
		std::basic_ostream<char> &os,
		bool clearBuffers,
		bool transmitParams)
{
	const bool isLittleEndian = osIsLittleEndian();
	LOG(LUX_DEBUG, LUX_NOERROR) << "Transmitting film (little endian=" << boost::lexical_cast<std::string>(isLittleEndian) << ")";

	std::streampos osStartPosition = os.tellp();

	ScopedPoolLock lock(contribPool);

	// Enable compression
	// TODO Move this below header when implementing FILM VERSION 2
	boost::iostreams::filtering_stream<boost::iostreams::output> fs;
	fs.push(boost::iostreams::gzip_compressor(4));
	fs.push(os);

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

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_LDR_CLAMP_METHOD, 0));		

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
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_GLARE_MAP, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_STRING, LUX_FILM_GLARE_PUPIL, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_STRING, LUX_FILM_GLARE_LASHES, 0));

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
	header.Write(fs, isLittleEndian);

	// Write each buffer group
	double totNumberOfSamples = 0.;
	for (u_int i = 0; i < bufferGroups.size(); ++i) {
		BufferGroup& bufferGroup = bufferGroups[i];
		// Write number of samples
		osWriteLittleEndianDouble(isLittleEndian, fs, bufferGroup.numberOfSamples);

		// Write each buffer
		for (u_int j = 0; j < bufferConfigs.size(); ++j) {
			Buffer* buffer = bufferGroup.getBuffer(j);

			// Write pixels
			const BlockedArray<Pixel>* pixelBuf = &(buffer->pixels);
			for (u_int y = 0; y < pixelBuf->vSize(); ++y) {
				for (u_int x = 0; x < pixelBuf->uSize(); ++x) {
					const Pixel &pixel = (*pixelBuf)(x, y);
					osWriteLittleEndianFloat(isLittleEndian, fs, pixel.L.c[0]);
					osWriteLittleEndianFloat(isLittleEndian, fs, pixel.L.c[1]);
					osWriteLittleEndianFloat(isLittleEndian, fs, pixel.L.c[2]);
					osWriteLittleEndianFloat(isLittleEndian, fs, pixel.alpha);
					osWriteLittleEndianFloat(isLittleEndian, fs, pixel.weightSum);
				}
				if (!fs.good())
					// error during transmission, abort
					return false;
			}
		}

		totNumberOfSamples += bufferGroup.numberOfSamples;
		LOG(LUX_DEBUG,LUX_NOERROR) << "Transmitted " << bufferGroup.numberOfSamples << " samples for buffer group " << i <<
			" (buffer config size: " << bufferConfigs.size() << ")";
	}

	flush(fs);
	int size = os.tellp() - osStartPosition;

	LOG(LUX_DEBUG, LUX_NOERROR) << "Transmitted film with " << totNumberOfSamples << " samples";
	LOG(LUX_INFO, LUX_NOERROR) << "Film transmission done (" << (size / 1024) << " Kbytes sent)";

	// Clear buffers here if requested,
	// because the saved contribPool will unlock at end of scope
	if (clearBuffers)
		ClearBuffers();

	return true;
}

bool Film::LoadResumeFilm(const string &filename)
{
	const bool isLittleEndian = osIsLittleEndian();
	LOG(LUX_DEBUG,LUX_NOERROR) << "Loading film (little endian=" << boost::lexical_cast<std::string>(isLittleEndian) << ")";
	std::ifstream is(filename.c_str(), std::ios_base::in | std::ios_base::binary);

	// Enable compression
	// TODO Move this below header when implementing FILM VERSION 2
	boost::iostreams::filtering_stream<boost::iostreams::input> fs;
	fs.push(boost::iostreams::gzip_decompressor());
	fs.push(is);

	FlmHeader header;
	if (!header.Read(fs, isLittleEndian, NULL))
		return false;
	is.close();

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

//------------------------------------------------------------------------------
// From Sfera source, for fast filtering
//------------------------------------------------------------------------------

static void ApplyBoxFilterX(const float *src, float *dest,
	const unsigned int width, const unsigned int height, const unsigned int radius) {
    const float scale = 1.0f / (float)((radius << 1) + 1);

    // Do left edge
    float t = src[0] * radius;
    for (unsigned int x = 0; x < (radius + 1); ++x)
        t += src[x];
    dest[0] = t * scale;

    for (unsigned int x = 1; x < (radius + 1); ++x) {
        t += src[x + radius];
        t -= src[0];
        dest[x] = t * scale;
    }

    // Main loop
    for (unsigned int x = (radius + 1); x < width - radius; ++x) {
        t += src[x + radius];
        t -= src[x - radius - 1];
        dest[x] = t * scale;
    }

    // Do right edge
    for (unsigned int x = width - radius; x < width; ++x) {
        t += src[width - 1];
        t -= src[x - radius - 1];
        dest[x] = t * scale;
    }
}

static void ApplyBoxFilterY(const float *src, float *dst,
	const unsigned int width, const unsigned int height, const unsigned int radius) {
    const float scale = 1.0f / (float)((radius << 1) + 1);

    // Do left edge
    float t = src[0] * radius;
    for (unsigned int y = 0; y < (radius + 1); ++y) {
        t += src[y * width];
    }
    dst[0] = t * scale;

    for (unsigned int y = 1; y < (radius + 1); ++y) {
        t += src[(y + radius) * width];
        t -= src[0];
        dst[y * width] = t * scale;
    }

    // Main loop
    for (unsigned int y = (radius + 1); y < (height - radius); ++y) {
        t += src[(y + radius) * width];
        t -= src[((y - radius) * width) - width];
        dst[y * width] = t * scale;
    }

    // Do right edge
    for (unsigned int y = height - radius; y < height; ++y) {
        t += src[(height - 1) * width];
        t -= src[((y - radius) * width) - width];
        dst[y * width] = t * scale;
    }
}

static void ApplyBoxFilterXR1(const float *src, float *dest,
	const unsigned int width, const unsigned int height) {
    const float scale = 1.f / 3.f;

    // Do left edge
    float t = 2.f * src[0];
	t += src[1];
    dest[0] = t * scale;

	t += src[2];
	t -= src[0];
	dest[1] = t * scale;

    // Main loop
    for (unsigned int x = 2; x < width - 1; ++x) {
        t += src[x + 1];
        t -= src[x - 2];
        dest[x] = t * scale;
    }

    // Do right edge
	t += src[width - 1];
	t -= src[width - 3];
	dest[width - 1] = t * scale;
}

static void ApplyBoxFilterYR1(const float *src, float *dst,
	const unsigned int width, const unsigned int height) {
    const float scale = 1.f / 3.f;

    // Do left edge
	float t = 2.f * src[0];
	t += src[width];
	dst[0] = t * scale;

	t += src[2 * width];
	t -= src[0];
	dst[width] = t * scale;

    // Main loop
    for (unsigned int y = 2; y < height - 1; ++y) {
        t += src[(y + 1) * width];
        t -= src[((y - 1) * width) - width];
        dst[y * width] = t * scale;
    }

    // Do right edge
	t += src[(height - 1) * width];
	t -= src[((height - 2) * width) - width];
	dst[(height - 1) * width] = t * scale;
}

static void ApplyBoxFilter(float *frameBuffer, float *tmpFrameBuffer,
	const unsigned int width, const unsigned int height, const unsigned int radius) {
	if (radius == 1) {
		for (unsigned int i = 0; i < height; ++i)
			ApplyBoxFilterXR1(&frameBuffer[i * width], &tmpFrameBuffer[i * width], width, height);

		for (unsigned int i = 0; i < width; ++i)
			ApplyBoxFilterYR1(&tmpFrameBuffer[i], &frameBuffer[i], width, height);
	} else {
		for (unsigned int i = 0; i < height; ++i)
			ApplyBoxFilterX(&frameBuffer[i * width], &tmpFrameBuffer[i * width], width, height, radius);

		for (unsigned int i = 0; i < width; ++i)
			ApplyBoxFilterY(&tmpFrameBuffer[i], &frameBuffer[i], width, height, radius);
	}
}

//------------------------------------------------------------------------------
// Update the noise-aware map
//------------------------------------------------------------------------------

void Film::UpdateConvergenceInfo(const float *framebuffer) {
	// Compare the new buffer with the old one
	const u_int failedPixels = convTest->Test(framebuffer);

	// Check if we can stop the rendering
	const u_int nPix = xPixelCount * yPixelCount;
	const float failedPercentage = failedPixels / (float)nPix;
	if (failedPercentage <= haltThreshold)
		enoughSamplesPerPixel = true;

	// NOTE: TVI is a side product of convergence test so this code can be
	// executed even if halttreshold is not used
	if (enoughSamplesPerPixel && (haltThreshold >= 0.f))
		haltThresholdComplete = 1.f - haltThreshold;
	else
		haltThresholdComplete = (nPix - failedPixels) / (float)nPix;
}

void Film::GenerateNoiseAwareMap() {
	fast_mutex::scoped_lock lock(samplingMapMutex);

	const u_int nPix = xPixelCount * yPixelCount;

	// Free the reference to the old one and allocate a new one
	noiseAwareMap.reset(new float[nPix]);

	const float *convergenceTVI = convTest->GetTVI();
	bool hasPixelsToSample = false;
	bool allZeroVariance = true;
	bool allZeroTVI = true;
	u_int index = 0;
	for (u_int y = 0; y < yPixelCount; ++y) {
		for (u_int x = 0; x < xPixelCount; ++x, ++index) {
			const float variance = varianceBuffer->GetVariance(x, y);
			// -1 means a pixel that have yet to be sampled
			if (variance == -1.f) {
				hasPixelsToSample = true;
				break;
			}
			
			if (variance > 0.f)
				allZeroVariance = false;
			if (convergenceTVI[index] > 0.f)
				allZeroTVI = false;

			noiseAwareMap[index] = sqrtf(variance);
		}
	}

	if (hasPixelsToSample || allZeroVariance || allZeroTVI) {
		LOG(LUX_DEBUG, LUX_NOERROR) << "Noise aware map based on: uniform distribution";

		// Just use a uniform distribution
		std::fill(noiseAwareMap.get(), noiseAwareMap.get() + nPix, 1.f);
	} else {
		++noiseAwareMapVersion;
		LOG(LUX_DEBUG, LUX_NOERROR) << "Noise aware map based on: noise information (version: " <<
				noiseAwareMapVersion << ")";

		// First step, build Standard Error / TVI map
		float minValue = std::numeric_limits<double>::infinity();
		float maxValue = 0.f;
		for (u_int i = 0; i < nPix; ++i) {
			const float standardError = noiseAwareMap[i];
			//const float value = (convergenceTVI[i] == 0.f) ? 0.f : logf(1.f + (standardError / convergenceTVI[i]));
			const float value = (convergenceTVI[i] == 0.f) ? 0.f : (standardError / convergenceTVI[i]);

			minValue = min(minValue, value);
			maxValue = max(maxValue, value);
			noiseAwareMap[i] = value;
		}

		// Than build an histogram of the map
		const float valueRange = maxValue - minValue;
		const u_int histogramSize = 1000000;
		float *histogram = new float[histogramSize * sizeof(float)];
		std::fill(histogram, histogram + histogramSize, 0.f);
		for (u_int i = 0; i < nPix; ++i) {
			// Map the value between 0.0 and 1.0
			const float v = (noiseAwareMap[i] - minValue) / valueRange;

			const u_int binIndex = min(Floor2UInt(v * histogramSize), histogramSize - 1);
			histogram[binIndex] += 1;
		}

		// Clamp of the map between 5th percentile value and 95th
		u_int minIndex = 0;
		float count = 0;
		for (u_int i = 0; i < histogramSize; ++i) {
			count += histogram[i];
		
			if (count > 5.f * histogramSize / 100.f) {
				minIndex = i;
				break;
			}
		}

		u_int maxIndex = minIndex;
		count = 0;
		for (u_int i = histogramSize - 1; i > minIndex; --i) {
			count += histogram[i];
		
			if (count > 5.f * histogramSize / 100.f) {
				maxIndex = i;
				break;
			}
		}

		delete[] histogram;

		if (maxIndex <= minIndex) {
			LOG(LUX_DEBUG, LUX_NOERROR) << "Noise aware map based on: uniform distribution (unable to auto-stretch the map)";

			// Just use a uniform distribution
			std::fill(noiseAwareMap.get(), noiseAwareMap.get() + nPix, 1.f);
		} else {
			const float minAllowedValue = valueRange * minIndex / histogramSize + minValue;
			const float maxAllowedValue = valueRange * maxIndex / histogramSize + minValue;

			for (u_int i = 0; i < nPix; ++i) {
				// Clamp the map in the [minAllowedValue, maxAllowedValue] range
				// and scale between 0.1 and 1.0
				noiseAwareMap[i] = .8f * (Clamp(noiseAwareMap[i], minAllowedValue, maxAllowedValue) - minAllowedValue) /
						(maxAllowedValue - minAllowedValue) + .2f;
			}

			// Apply an heavy filter to smooth the map
			float *tmpMap = new float[nPix];
			ApplyBoxFilter(noiseAwareMap.get(), tmpMap, xPixelCount, yPixelCount, 6);
			delete []tmpMap;
		}
	}

	noiseAwareDistribution2D.reset(new Distribution2D(noiseAwareMap.get(), xPixelCount, yPixelCount));

	UpdateSamplingMap();
}

const bool Film::GetNoiseAwareMap(u_int &version, boost::shared_array<float> &map,
		boost::shared_ptr<Distribution2D> &distrib) {
	fast_mutex::scoped_lock lock(samplingMapMutex);

	if (noiseAwareMapVersion > version) {
		map = noiseAwareMap;
		version = noiseAwareMapVersion;
		distrib = noiseAwareDistribution2D;

		return true;
	} else
		return false;
}

// NOTE: returns a copy of the map, it is up to the caller to free the allocated memory !
float *Film::GetNoiseAwareMap() {
	fast_mutex::scoped_lock lock(samplingMapMutex);

	if (noiseAwareMapVersion == 0)
		return NULL;
	
	const u_int nPix = xPixelCount * yPixelCount;
	float *map = new float[nPix];
	std::copy(noiseAwareMap.get(), noiseAwareMap.get() + nPix, map);

	return map;
}

void Film::SetNoiseAwareMap(const float *map) {
	fast_mutex::scoped_lock lock(samplingMapMutex);

	const u_int nPix = xPixelCount * yPixelCount;
	noiseAwareMap.reset(new float[nPix]);

	std::copy(map, map + nPix, noiseAwareMap.get());
	++noiseAwareMapVersion;

	noiseAwareDistribution2D.reset(new Distribution2D(noiseAwareMap.get(), xPixelCount, yPixelCount));

	UpdateSamplingMap();
}

const bool Film::GetUserSamplingMap(u_int &version, boost::shared_array<float> &map,
		boost::shared_ptr<Distribution2D> &distrib) {
	fast_mutex::scoped_lock lock(samplingMapMutex);

	if (userSamplingMapVersion > version) {
		map = userSamplingMap;
		version = userSamplingMapVersion;
		distrib = userSamplingDistribution2D;
		return true;
	} else
		return false;
}

// NOTE: returns a copy of the map, it is up to the caller to free the allocated memory !
float *Film::GetUserSamplingMap() {
	fast_mutex::scoped_lock lock(samplingMapMutex);

	if (userSamplingMapVersion == 0)
		return NULL;
	
	const u_int nPix = xPixelCount * yPixelCount;
	float *map = new float[nPix];
	std::copy(userSamplingMap.get(), userSamplingMap.get() + nPix, map);

	return map;
}

void Film::SetUserSamplingMap(const float *map) {
	fast_mutex::scoped_lock lock(samplingMapMutex);

	// TODO: reject the map if all values are 0.0
	const u_int nPix = xPixelCount * yPixelCount;
	userSamplingMap.reset(new float[nPix]);

	std::copy(map, map + nPix, userSamplingMap.get());
	++userSamplingMapVersion;

	userSamplingDistribution2D.reset(new Distribution2D(userSamplingMap.get(), xPixelCount, yPixelCount));

	UpdateSamplingMap();

	// Check if I have to write the new map to the file
	if (userSamplingMapFileName != "") {
		// Write a single channel EXR file
		LOG(LUX_DEBUG, LUX_NOERROR) << "Saving user sampling map to file: " << userSamplingMapFileName;
		WriteOpenEXRImage(userSamplingMapFileName, xPixelCount, yPixelCount, map);
	}
}

void Film::UpdateSamplingMap() {	
	// Update noise-aware map * user sampling map

	const u_int nPix = xPixelCount * yPixelCount;
	if (noiseAwareMapVersion > 0) {
		samplingMap.reset(new float[nPix]);

		if (userSamplingMapVersion > 0) {
			for (u_int i = 0; i < nPix; ++i)
				samplingMap[i] = noiseAwareMap[i] * userSamplingMap[i];
		} else
			std::copy(noiseAwareMap.get(), noiseAwareMap.get() + nPix, samplingMap.get());

		samplingDistribution2D.reset(new Distribution2D(samplingMap.get(), xPixelCount, yPixelCount));
	} else {
		if (userSamplingMapVersion > 0) {
			samplingMap.reset(new float[nPix]);
			std::copy(userSamplingMap.get(), userSamplingMap.get() + nPix, samplingMap.get());
			samplingDistribution2D.reset(new Distribution2D(samplingMap.get(), xPixelCount, yPixelCount));
		}
	}
}

const bool Film::GetSamplingMap(u_int &naMapVersion, u_int &usMapVersion,
		boost::shared_array<float> &map, boost::shared_ptr<Distribution2D> &distrib) {
	fast_mutex::scoped_lock lock(samplingMapMutex);

	if ((noiseAwareMapVersion > naMapVersion) || (userSamplingMapVersion > usMapVersion)) {
		naMapVersion = noiseAwareMapVersion;
		usMapVersion = userSamplingMapVersion;
		map = samplingMap;
		distrib = samplingDistribution2D;

		return true;
	} else
		return false;
}

} // namespace lux
