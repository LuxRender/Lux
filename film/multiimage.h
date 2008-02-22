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

#ifndef LUX_MULTIIMAGE_H
#define LUX_MULTIIMAGE_H


#include "lux.h"
#include "film.h"
#include "color.h"
#include "spectrum.h"
#include "paramset.h"
#include "tonemap.h"
#include "sampling.h"
#include "igiio.h"
#include <stdio.h>
#include <boost/timer.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/serialization/base_object.hpp>

namespace lux
{

#define WI_HDR 0
#define WI_LDR 1
#define WI_IGI 3
#define WI_FRAMEBUFFER 2

// ImageFilm Declarations
class MultiImageFilm : public Film {
	friend class boost::serialization::access;
public:
	// MultiImageFilm Public Methods
	MultiImageFilm(int xres, int yres) : Film(xres,yres) { pixels=NULL; filter=NULL; filterTable=NULL; }; 
	//Copy constructor
	MultiImageFilm(const MultiImageFilm &m) : Film(m.xResolution,m.yResolution)
	{
		boost::mutex::scoped_lock lock(m.addSampleMutex);
		pixels=new BlockedArray<Pixel>(*(m.pixels));
		filter=NULL;
		filterTable=NULL;
	}
	MultiImageFilm(int xres, int yres,
	                     Filter *filt, const float crop[4], bool hdr_out, bool igi_out, bool ldr_out, 
		             const string &hdr_filename, const string &igi_filename, const string &ldr_filename, bool premult,
		             int hdr_writeInterval, int igi_writeInterval, int ldr_writeInterval, int ldr_displayInterval,
					 const string &toneMapper, float contrast_displayAdaptationY, float nonlinear_MaxY,
					 float reinhard_prescale, float reinhard_postscale, float reinhard_burn,
					 float bloomWidth, float bloomRadius, float gamma, float dither);
	~MultiImageFilm() {
		if(pixels!=NULL) delete pixels;
		if(filter!=NULL)delete filter;
		if(filterTable!=NULL)delete[] filterTable;
	}

	void AddSample(float sX, float sY, const SWCSpectrum &L, float alpha);
	void AddSample(float sX, float sY, const XYZColor &L, float alpha);

	void GetSampleExtent(int *xstart, int *xend,
	                     int *ystart, int *yend) const;
	void WriteImage() {};
	void WriteImage(int oType);
	void WriteTGAImage(float *rgb, float *alpha, const string &filename);
	void WriteEXRImage(float *rgb, float *alpha, const string &filename);
	void WriteIGIImage(float *rgb, float *alpha, const string &filename);

	// used by gui
	void createFrameBuffer();
	void updateFrameBuffer();
	unsigned char* getFrameBuffer();
	float getldrDisplayInterval() { return ldrDisplayInterval; }
	
	void merge(MultiImageFilm &f);
void clean();

	static Film *CreateFilm(const ParamSet &params, Filter *filter);
private:
	template<class Archive>
				void serialize(Archive & ar, const unsigned int version)
				{
					boost::mutex::scoped_lock lock(addSampleMutex);
					// serialize base class information
					ar & boost::serialization::base_object<Film>(*this);
					//ar & xResolution;
					//ar & yResolution;
					
					//ar & cropWindow;
					ar & xPixelStart;
					ar & yPixelStart;
					ar & xPixelCount;
					ar & yPixelCount;
					ar & pixels;
					//new BlockedArray<Pixel>(xPixelCount, yPixelCount);
				}
	
	
	// MultiImageFilm Private Data
	Filter *filter;
	int hdrWriteInterval, igiWriteInterval, ldrWriteInterval, ldrDisplayInterval, sampleCount;
	string hdrFilename, igiFilename, ldrFilename;
    bool ldrLock, hdrLock, igiLock, ldrDisplayLock;
	bool hdrOut, ldrOut, igiOut, premultiplyAlpha;
	float cropWindow[4];
	int xPixelStart, yPixelStart, xPixelCount, yPixelCount;
	struct Pixel {
		Pixel() : L(0.f) {
			alpha = 0.f;
			weightSum = 0.f;
		}
		XYZColor L;
		float alpha, weightSum;
		
		template<class Archive>
					void serialize(Archive & ar, const unsigned int version)
					{
						ar & L;
						ar & alpha;
						ar & weightSum;
					}
	};
	BlockedArray<Pixel> *pixels;
	float *filterTable;
	boost::timer ldrTimer, hdrTimer, igiTimer, ldrDisplayTimer;

	string toneMapper;
	ParamSet toneParams;
	float contrastDisplayAdaptationY, nonlinearMaxY,
		reinhardPrescale, reinhardPostscale, reinhardBurn,
		bloomWidth, bloomRadius, gamma, dither;
	unsigned char *framebuffer;

	mutable boost::mutex addSampleMutex;
};

}//namespace lux

#endif //LUX_MULTIIMAGE_H

