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
		int xResolution, int yResolution, GREYCStorationParams &GREYCParams, ColorSystem &colorSpace, Histogram &histogram, bool HistogramEnabled,
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
			const float invxRes = 1.f / xResolution;
			const float invyRes = 1.f / yResolution;
			//for each pixel in the source image
			for(int y=0; y<yResolution; ++y)
				for(int x=0; x<xResolution; ++x)
				{
					const float nPx = (float)x * invxRes;
					const float nPy = (float)y * invyRes;
					const float xOffset = nPx - 0.5f;
					const float yOffset = nPy - 0.5f;
					float tOffset = sqrtf(xOffset*xOffset + yOffset*yOffset);
					
					// Vignetting
					if(VignettingEnabled && VignetScale != 0.0f) {
						// normalize to range [0.f - 1.f]
						const float invNtOffset = 1.f - (fabsf(tOffset) * 1.42);
						float vWeight = Lerp(invNtOffset, 1.f - VignetScale, 1.f);
						for(int i=0;i<3;i++)
							pixels[xResolution*y + x].c[i] *= vWeight;
					}
				}
		}

		// Calculate histogram
		if(HistogramEnabled) histogram.Calculate(pixels, xResolution, yResolution);

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

			for (unsigned int iter=0; iter<GREYCParams.nb_iter; iter++) {
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
					1.0f); // gfact
			}

			// Copy data from cimg buffer back to pixels vector
			const float inv_byte = 1.f/255;
			for(int y=0; y<yResolution; y++) {
				for(int x=0; x<xResolution; x++) {
					int index = xResolution * y + x;
					for(int j=0; j<3; j++)
						pixels[index].c[j] = img(x, y, 0, j) * inv_byte;
				}
			}
		}

		// Dither image
		if (dither > 0.f)
			for (int i = 0; i < nPix; ++i)
				pixels[i] += 2.f * dither * (lux::random::floatValueP() - .5f);
	}

// Film Function Definitions

void Film::getHistogramImage(unsigned char *outPixels, int width, int height, int options){
	m_histogram.MakeImage(outPixels, width, height, options);
}

// Histogram Function Definitions

Histogram::Histogram(){
	m_buckets = NULL;
	m_displayGamma = 2.2; //gamma of the display the histogram is viewed on

	//calculate visible plot range
	float narrowRangeSize = -log(pow(1.f/2,10*1.f/m_displayGamma)); //size of 10 EV zones, display-gamma corrected
	float scalingFactor = 0.75;
	m_lowRange = -(1+scalingFactor)*narrowRangeSize;
	m_highRange = scalingFactor*narrowRangeSize;

	m_bucketNr = 0;
	m_newBucketNr = 300;
	CheckBucketNr();
	for(int i=0;i<m_bucketNr*4;i++) m_buckets[i]=0;
}

Histogram::~Histogram(){
	if(m_buckets != NULL) delete[] m_buckets;
}

void Histogram::CheckBucketNr(){
	if(m_newBucketNr>0){ //if nr of buckets changed recalculate data that depends on it
		m_bucketNr=m_newBucketNr;
		m_newBucketNr=-1;
		if(m_buckets != NULL) delete[] m_buckets;
		m_buckets = new float[m_bucketNr*4];

		//new bucket size
		m_bucketSize = (m_highRange-m_lowRange)/m_bucketNr;

		//calculate EV zone tick positions
		int bucket, i;
		float value, zoneValue = 1.0;
		for (i=0;i<11;i++){
			value = log(pow(zoneValue,1.f/m_displayGamma));
			bucket = (value-m_lowRange)/m_bucketSize;
			bucket = Clamp(bucket,0,m_bucketNr-1);
			m_zones[i] = bucket;
			zoneValue = zoneValue/2;
		}
	}
}

void Histogram::Calculate(vector<Color> &pixels, unsigned int width, unsigned int height){
	if(pixels.empty() || width==0 || height==0) return;
	unsigned int i, j;
	unsigned int pixelNr=width*height;
	float value;
	boost::mutex::scoped_lock lock(m_mutex);

	CheckBucketNr();

	//empty buckets
	for(i=0;i<(u_int)m_bucketNr*4;i++) m_buckets[i]=0;

	//fill buckets
	int bucket;
	for(i=0;i<pixelNr;i++){
		for(j=0;j<3;j++){ //each color channel
			value = pixels[i].c[j];
			if(value>0){
				value = log(value);
				bucket = (value-m_lowRange)/m_bucketSize;
				bucket = Clamp(bucket,0,m_bucketNr-1);
				m_buckets[bucket*4+j] += 1;
			}
		}
		value = ( pixels[i].c[0] + pixels[i].c[1] + pixels[i].c[2] )/3; //brightness
		if(value>0){
			value = log(value);
			bucket = (value-m_lowRange)/m_bucketSize;
			bucket = Clamp(bucket,0,m_bucketNr-1);
			m_buckets[bucket*4+3] += 1;
		}
	}

}

void Histogram::MakeImage(unsigned char *outPixels, unsigned int canvasW, unsigned int canvasH, int options){
	#define PIXELIDX(x,y,w) ((y)*(w)*3+(x)*3)
	#define GETMAX(x,y) ((x)>(y)?(x):(y))
	unsigned int i,x,y,idx;
	int bucket;
	unsigned char color;
	unsigned int borderW = 3; //size of the plot border in pixels
	unsigned int guideW = 3; //size of the brightness guide bar in pixels
	unsigned int plotH = canvasH-borderW-(guideW+2)-(borderW-1);
	unsigned int plotW = canvasW-2*borderW;
	if(canvasW<50 || canvasH<25) return; //too small
	boost::mutex::scoped_lock lock(m_mutex);
	if(canvasW-2*borderW!=(u_int)m_bucketNr) m_newBucketNr=canvasW-2*borderW;
	
	//clear drawing area
	color=64;
	for(x=0;x<plotW;x++)
		for(y=0;y<plotH;y++){
			idx=PIXELIDX(x+borderW,y+borderW,canvasW);
			outPixels[idx]=color;
			outPixels[idx+1]=color;
			outPixels[idx+2]=color;
		}

	//transform values to log if needed
	float *buckets;
	if(options&LUX_HISTOGRAM_LOG){
		buckets = new float[m_bucketNr*4];
		for(i=0;i<(u_int)m_bucketNr*4;i++){
			buckets[i]=log(1.f+m_buckets[i]);
		}
	}else{
		buckets=m_buckets;
	}

	//draw histogram bars
	int barHeight, channel=0;
	float scale, max, maxExtrL, maxExtrR;
	switch( options & ( LUX_HISTOGRAM_RGB | LUX_HISTOGRAM_RED | LUX_HISTOGRAM_GREEN | LUX_HISTOGRAM_BLUE | LUX_HISTOGRAM_VALUE | LUX_HISTOGRAM_RGB_ADD ) ){
		case LUX_HISTOGRAM_RGB: {
			//get maxima for scaling
			for(i=1*4,max=0;i<(u_int)(m_bucketNr-1)*4;i++)
				if(i%4!=3 && buckets[i]>max) max=buckets[i];
			i=0;
			maxExtrL = GETMAX(buckets[i*4],GETMAX(buckets[i*4+1],buckets[i*4+2]));
			i=m_bucketNr-1;
			maxExtrR = GETMAX(buckets[i*4],GETMAX(buckets[i*4+1],buckets[i*4+2]));
			if(max>0 || maxExtrL>0 || maxExtrR>0){
				//draw bars
				for(x=0;x<plotW;x++){
					bucket=Clamp((int)(x*m_bucketNr/(plotW-1)), 0, m_bucketNr-1);
					if(bucket==0 && maxExtrL>max) scale = (float)plotH/maxExtrL;
					else if(bucket==m_bucketNr-1 && maxExtrR>max) scale = (float)plotH/maxExtrR;
					else scale = (float)plotH/max;
					for(channel=0;channel<3;channel++){
						barHeight=plotH-(u_int)(buckets[bucket*4+channel]*scale+0.5);
						barHeight=Clamp(barHeight,0,(int)plotH);
						for(y=barHeight;y<plotH;y++)
							outPixels[PIXELIDX(x+borderW,y+borderW,canvasW)+channel]=255;
					}
				}
			}
		} break;
		case LUX_HISTOGRAM_VALUE: channel++;
		case LUX_HISTOGRAM_BLUE:  channel++;
		case LUX_HISTOGRAM_GREEN: channel++;
		case LUX_HISTOGRAM_RED: {
			//get maxima for scaling
			for(i=1,max=0;i<(u_int)m_bucketNr-1;i++)
				if(buckets[i*4+channel]>max) max=buckets[i*4+channel];
			i=0;
			maxExtrL = buckets[i*4+channel];
			i=m_bucketNr-1;
			maxExtrR = buckets[i*4+channel];
			if(max>0 || maxExtrL>0 || maxExtrR>0){
				//draw bars
				for(x=0;x<plotW;x++){
					bucket=Clamp((int)(x*m_bucketNr/(plotW-1)), 0, m_bucketNr-1);
					if(bucket==0 && maxExtrL>max) scale = (float)plotH/maxExtrL;
					else if(bucket==m_bucketNr-1 && maxExtrR>max) scale = (float)plotH/maxExtrR;
					else scale = (float)plotH/max;
					barHeight=plotH-(u_int)(buckets[bucket*4+channel]*scale+0.5);
					barHeight=Clamp(barHeight,0,(int)plotH);
					for(y=barHeight;y<plotH;y++){
						idx=PIXELIDX(x+borderW,y+borderW,canvasW);
						outPixels[idx]=255;
						outPixels[idx+1]=255;
						outPixels[idx+2]=255;
					}
				}
			}
		} break;
		case LUX_HISTOGRAM_RGB_ADD: {
			//calculate maxima for scaling
			for(i=1,max=0; i<(u_int)m_bucketNr-1; i++)
				if( buckets[i*4]+buckets[i*4+1]+buckets[i*4+2] > max ) max=buckets[i*4]+buckets[i*4+1]+buckets[i*4+2];
			i=0;
			maxExtrL = buckets[i*4]+buckets[i*4+1]+buckets[i*4+2];
			i=m_bucketNr-1;
			maxExtrR = buckets[i*4]+buckets[i*4+1]+buckets[i*4+2];
			if(max>0 || maxExtrL>0 || maxExtrR>0){
				//draw bars
				for(x=0;x<plotW;x++){
					bucket=Clamp((int)(x*m_bucketNr/(plotW-1)), 0, m_bucketNr-1);
					if(bucket==0 && maxExtrL>max) scale = (float)plotH/maxExtrL;
					else if(bucket==m_bucketNr-1 && maxExtrR>max) scale = (float)plotH/maxExtrR;
					else scale = (float)plotH/max;
					barHeight=plotH-(u_int)(buckets[bucket*4]*scale+0.5)-(u_int)(buckets[bucket*4+1]*scale+0.5)-(u_int)(buckets[bucket*4+2]*scale+0.5);
					barHeight=Clamp(barHeight,0,(int)plotH);
					int newHeight;
					for(channel=0;channel<3;channel++){
						newHeight=barHeight+(u_int)(buckets[bucket*4+channel]*scale+0.5);
						for(y=barHeight;y<(u_int)newHeight;y++)
							outPixels[PIXELIDX(x+borderW,y+borderW,canvasW)+channel]=255;
						barHeight=newHeight;
					}
				}
			}
		} break;
		default: break;
	}

	if(buckets!=m_buckets) delete [] buckets;

	//draw brightness guide
	for(x=0;x<plotW;x++){
		bucket=Clamp((int)(x*m_bucketNr/(plotW-1)), 0, m_bucketNr-1);
		for(y=plotH+2;y<plotH+2+guideW;y++){
			idx=PIXELIDX(x+borderW,y+borderW,canvasW);
			color=Clamp( expf((bucket*m_bucketSize+m_lowRange))*256.f, 0.f, 255.f ); //no need to gamma-correct, as we're already in display-gamma space
			if(options&LUX_HISTOGRAM_RED){
				outPixels[idx]=color;
				outPixels[idx+1]=0;
				outPixels[idx+2]=0;
			}else if(options&LUX_HISTOGRAM_GREEN){
				outPixels[idx]=0;
				outPixels[idx+1]=color;
				outPixels[idx+2]=0;
			}else if(options&LUX_HISTOGRAM_BLUE){
				outPixels[idx]=0;
				outPixels[idx+1]=0;
				outPixels[idx+2]=color;
			}else{
				outPixels[idx]=color;
				outPixels[idx+1]=color;
				outPixels[idx+2]=color;
			}
		}
	}

	//draw EV zone markers
	for(i=0;i<11;i++){
		switch(i){
			case 0:  color=128; break;
			case 10: color=128; break;
			default: color=0; break;
		}
		bucket=m_zones[i];
		x=Clamp((int)(bucket*plotW/(m_bucketNr-1)), 0, (int)plotW-1);
		for(y=plotH+2;y<plotH+2+guideW;y++){
			idx=PIXELIDX(x+borderW,y+borderW,canvasW);
			outPixels[idx]=color;
			outPixels[idx+1]=color;
			outPixels[idx+2]=color;
		}
	}

	//draw zone boundaries on the plot
	for(i=0;i<2;i++){
		switch(i){
			case 0: bucket=m_zones[0]; break;  //white
			case 1: bucket=m_zones[10]; break; //black
		}
		x=Clamp((int)(bucket*plotW/(m_bucketNr-1)), 0, (int)plotW-1);
		for(y=0;y<plotH;y++){
			idx=PIXELIDX(x+borderW,y+borderW,canvasW);
			if(outPixels[idx]==255 && outPixels[idx+1]==255 && outPixels[idx+2]==255){
				outPixels[idx]=128;
				outPixels[idx+1]=128;
				outPixels[idx+2]=128;
			}else{
				if(outPixels[idx]==64) outPixels[idx]=128;
				if(outPixels[idx+1]==64) outPixels[idx+1]=128;
				if(outPixels[idx+2]==64) outPixels[idx+2]=128;
			}
		}
	}

}


}//namespace lux

