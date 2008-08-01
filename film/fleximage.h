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

#ifndef LUX_FLEXIMAGE_H
#define LUX_FLEXIMAGE_H

#include "lux.h"
#include "film.h"
#include "color.h"
#include "paramset.h"
#include "tonemap.h"
#include "sampling.h"
#include <boost/thread/xtime.hpp>
#include <boost/thread/recursive_mutex.hpp>

namespace lux {

// FlexImageFilm Declarations
class FlexImageFilm : public Film {
public:
	// FlexImageFilm Public Methods
	FlexImageFilm(int xres, int yres) :
		Film(xres, yres, 0), filter(NULL), filterTable(NULL),
		framebuffer(NULL), factor(NULL), colorSpace(0.63f, 0.34f, 0.31f, 0.595f, 0.155f, 0.07f, 0.314275f, 0.329411f, 1.f) { }

	FlexImageFilm(int xres, int yres, Filter *filt, const float crop[4],
		const string &filename1, bool premult, int wI, int dI,
		bool w_tonemapped_EXR, bool w_untonemapped_EXR, bool w_tonemapped_IGI,
		bool w_untonemapped_IGI, bool w_tonemapped_TGA, bool w_resume_FLM, bool restart_resume_FLM,
		int haltspp, float reinhard_prescale, float reinhard_postscale,	float reinhard_burn, 
		const float cs_red[2], const float cs_green[2], const float cs_blue[2], const float whitepoint[2],
		float g, int reject_warmup, bool debugmode);
	~FlexImageFilm() {
		delete[] framebuffer;
		delete[] factor;
	}

	int RequestBuffer(BufferType type, BufferOutputConfig output, const string& filePostfix);
	void CreateBuffers();

	void GetSampleExtent(int *xstart, int *xend, int *ystart, int *yend) const;
	void AddSample(float sX, float sY, const XYZColor &L, float alpha, int buf_id = 0, int bufferGroup = 0);
	void AddSampleCount(float count, int bufferGroup = 0) {
		// Dade - This fix (bug #361) is not thread safe and always lead to a crash
		// on MacOS. I comment out the fix until we have a better one.
		/*if (bufferGroups.empty()) {
			RequestBuffer(BUF_TYPE_PER_SCREEN, BUF_FRAMEBUFFER, "");
			CreateBuffers();
		}*/

		if (!bufferGroups.empty()) {
			bufferGroups[bufferGroup].numberOfSamples += count;

			// Dade - check if we have enough samples per pixel
			if ((haltSamplePerPixel > 0) &&
				(bufferGroups[bufferGroup].numberOfSamples * invSamplePerPass >= 
						haltSamplePerPixel))
				enoughSamplePerPixel = true;
		}
	}

	void WriteImage(ImageType type);

	// GUI display methods
	void updateFrameBuffer();
	unsigned char* getFrameBuffer();
	void createFrameBuffer();
	float getldrDisplayInterval() {
		return displayInterval;
	}

	// Dade - method useful for transmitting the samples to a client
	void TransmitFilm(std::basic_ostream<char> &stream,
		int buf_id = 0, int bufferGroup = 0, bool clearBuffer = true);
	float UpdateFilm(Scene *scene, std::basic_istream<char> &stream,
		int buf_id = 0, int bufferGroup = 0);

	static Film *CreateFilm(const ParamSet &params, Filter *filter);

private:
	static void GetColorspaceParam(const ParamSet &params, const string name, float values[2]);

	void FlushSampleArray();
	// Dade - using this method requires to lock arrSampleMutex
	void MergeSampleArray();

	void WriteImage2(ImageType type, vector<Color> &color, vector<float> &alpha, string postfix);
	void WriteTGAImage(vector<Color> &rgb, vector<float> &alpha, const string &filename);
	void WriteEXRImage(vector<Color> &rgb, vector<float> &alpha, const string &filename);
	void WriteIGIImage(vector<Color> &rgb, vector<float> &alpha, const string &filename);
	void WriteResumeFilm(const string &filename);
	void ScaleOutput(vector<Color> &color, vector<float> &alpha, float *scale);

	// FlexImageFilm Private Data
	Filter *filter;
	int writeInterval;
	int displayInterval;
	string filename;
	bool premultiplyAlpha, buffersInited;
	float cropWindow[4], *filterTable;
	int xPixelStart, yPixelStart, xPixelCount, yPixelCount;
	ParamSet toneParams;
	float gamma;
	float reject_warmup_samples;
	bool writeTmExr, writeUtmExr, writeTmIgi, writeUtmIgi, writeTmTga, writeResumeFlm, restartResumeFlm;

	unsigned char *framebuffer;
	// Dade - timer is broken under Linux when using multiple threads, using
	// instead
	boost::xtime lastWriteImageTime;

	bool debug_mode;
	float *factor;

	std::vector<BufferConfig> bufferConfigs;
	std::vector<BufferGroup> bufferGroups;

	mutable boost::recursive_mutex addSampleMutex;
	// Dade - this mutex is used to lock SampleArrptr/SampleArr2ptr pointers.
	// Be aware of potential dealock with addSampleMutex mutex. Always lock 
	// addSampleMutex first and then arrSampleMutex.
	mutable boost::recursive_mutex arrSampleMutex;
	// Dade - used by the WriteImage method
	mutable boost::recursive_mutex imageMutex;

	float maxY;
	u_int warmupSamples;
	bool warmupComplete;
	ArrSample *SampleArrptr;
	ArrSample *SampleArr2ptr;
	int curSampleArrId, curSampleArr2Id, maxSampleArrId;
	ColorSystem colorSpace;
};

}//namespace lux

#endif //LUX_FLEXIMAGE_H

