/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#include "lux.h"
#include "film.h"
#include "color.h"
#include "paramset.h"
#include "tonemap.h"
#include "sampling.h"
#include <stdio.h>
#include <boost/timer.hpp>

#define WI_HDR 0
#define WI_LDR 1
#define WI_DISPLAY 2

// ImageFilm Declarations
class MultiImageFilm : public Film {
public:
	// MultiImageFilm Public Methods
	MultiImageFilm(int xres, int yres,
	                     Filter *filt, const float crop[4], bool hdr_out, bool ldr_out, 
		             const string &hdr_filename, const string &ldr_filename, bool premult,
		             int hdr_writeInterval, int ldr_writeInterval, int ldr_displayInterval,
					 const string &toneMapper, float contrast_displayAdaptationY, float nonlinear_MaxY,
					 float reinhard_prescale, float reinhard_postscale, float reinhard_burn,
					 float bloomWidth, float bloomRadius, float gamma, float dither);
	~MultiImageFilm() {
		delete pixels;
		delete filter;
		delete[] filterTable;
	}
	void AddSample(const Sample &sample, const Ray &ray,
	               const Spectrum &L, float alpha);
	void GetSampleExtent(int *xstart, int *xend,
	                     int *ystart, int *yend) const;
	void WriteImage() {};
	void WriteImage(int oType);
	void WriteTGAImage(float *rgb, float *alpha, const string &filename);
	void WriteEXRImage(float *rgb, float *alpha, const string &filename);

	static Film *CreateFilm(const ParamSet &params, Filter *filter);
private:
	// MultiImageFilm Private Data
	Filter *filter;
	int hdrWriteInterval, ldrWriteInterval, ldrDisplayInterval, sampleCount;
	string hdrFilename, ldrFilename;
    bool ldrLock, hdrLock, displayLock;
	bool hdrOut, ldrOut, premultiplyAlpha;
	float cropWindow[4];
	int xPixelStart, yPixelStart, xPixelCount, yPixelCount;
	struct Pixel {
		Pixel() : L(0.f) {
			alpha = 0.f;
			weightSum = 0.f;
		}
		Spectrum L;
		float alpha, weightSum;
	};
	BlockedArray<Pixel> *pixels;
	float *filterTable;
	boost::timer ldrTimer, hdrTimer, displayTimer;

	string toneMapper;
	ParamSet toneParams;
	float contrastDisplayAdaptationY, nonlinearMaxY,
		reinhardPrescale, reinhardPostscale, reinhardBurn,
		bloomWidth, bloomRadius, gamma, dither;
};
