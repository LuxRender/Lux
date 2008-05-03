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
#include <boost/timer.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/serialization/base_object.hpp>

namespace lux {

class BufferConfig {
public:
	BufferConfig(BufferType t, BufferOutputConfig o, const string& s) :
		type(t), output(o), postfix(s) { }
	BufferType type;
	BufferOutputConfig output;
	string postfix;
};
struct Pixel {
	Pixel(): L(0.f), alpha(0.f), weightSum(0.f) { }
	XYZColor L;
	float alpha, weightSum;
};
class Buffer {
public:
	Buffer(int x, int y) {
		xPixelCount = x;
		yPixelCount = y;
		pixels = new BlockedArray<Pixel>(x, y);
	}
	virtual ~Buffer() {
		delete pixels; 
	}
	void Add(int x, int y, XYZColor L, float alpha, float wt) {
		Pixel &pixel = (*pixels)(x, y);
		pixel.L.AddWeighted(wt, L);
		pixel.alpha += alpha * wt;
		pixel.weightSum += wt;
	}
	void Clean() { }
	virtual void GetData(float *rgb, float *alpha) = 0;
	bool isFramebuffer;
	int xPixelCount, yPixelCount;
	float scaleFactor;
	BlockedArray<Pixel> *pixels;
};

// Per pixel normalized buffer
class RawBuffer : public Buffer {
public:
	RawBuffer(int x, int y) : Buffer(x, y) { }

	~RawBuffer() { }

	void GetData(float *rgb, float *alpha) {
		for (int y = 0, offset = 0; y < yPixelCount; ++y) {
			for (int x = 0; x < xPixelCount; ++x, ++offset) {
				Pixel &pixel = (*pixels)(x, y);
				rgb[3*offset  ] = pixel.L.c[0];
				rgb[3*offset+1] = pixel.L.c[1];
				rgb[3*offset+2] = pixel.L.c[2];
				alpha[offset] = pixel.alpha;
			}
		}
	}
};

// Per pixel normalized buffer
class PerPixelNormalizedBuffer : public Buffer {
public:
	PerPixelNormalizedBuffer(int x, int y) : Buffer(x, y) { }

	~PerPixelNormalizedBuffer() { }

	void GetData(float *rgb, float *alpha) {
		for (int y = 0, offset = 0; y < yPixelCount; ++y) {
			for (int x = 0; x < xPixelCount; ++x, ++offset) {
				Pixel &pixel = (*pixels)(x, y);
				if (pixel.weightSum == 0.f) {
					alpha[offset] = 0.f;
					rgb[3*offset  ] = 0.f;
					rgb[3*offset+1] = 0.f;
					rgb[3*offset+2] = 0.f;
				} else {
					float inv = 1.f / pixel.weightSum;
					// Convert pixel XYZ radiance to RGB
					pixel.L.ToRGB(rgb + 3 * offset);
					rgb[3*offset  ] *= inv;
					rgb[3*offset+1] *= inv;
					rgb[3*offset+2] *= inv;
					alpha[offset] = pixel.alpha;
				}
			}
		}
	}
};

// Per screen normalized  buffer
class PerScreenNormalizedBuffer : public Buffer {
public:
	PerScreenNormalizedBuffer(int x, int y, const float *samples) :
		Buffer(x, y), numberOfSamples_(samples) { }

	~PerScreenNormalizedBuffer() { }

	void GetData(float *rgb, float *alpha) {
		float inv = xPixelCount * yPixelCount / *numberOfSamples_;
		for (int y = 0, offset = 0; y < yPixelCount; ++y) {
			for (int x = 0; x < xPixelCount; ++x, ++offset) {
				Pixel &pixel = (*pixels)(x, y);
				// Convert pixel XYZ radiance to RGB
				pixel.L.ToRGB(rgb + 3 * offset);
				rgb[3*offset  ] *= inv;
				rgb[3*offset+1] *= inv;
				rgb[3*offset+2] *= inv;
				alpha[offset] = pixel.alpha;
			}
		}
	}
private:
	const float *numberOfSamples_;
};


class BufferGroup {
public:
	BufferGroup() : numberOfSamples(0.f) { }
	~BufferGroup() {
		for(vector<Buffer *>::iterator buffer = buffers.begin(); buffer != buffers.end(); ++buffer)
			delete *buffer;
	}

	void CreateBuffers(const vector<BufferConfig> &configs, int x, int y) {
		for(vector<BufferConfig>::const_iterator config = configs.begin(); config != configs.end(); ++config) {
			switch ((*config).type) {
			case BUF_TYPE_PER_PIXEL:
				buffers.push_back(new PerPixelNormalizedBuffer(x, y));
				break;
			case BUF_TYPE_PER_SCREEN:
				buffers.push_back(new PerScreenNormalizedBuffer(x, y, &numberOfSamples));
				break;
			case BUF_TYPE_RAW:
				buffers.push_back(new RawBuffer(x, y));
				break;
			default:
				assert(0);
			}
		}
	}

	Buffer *getBuffer(int index) {
		return buffers[index];
	}
	float numberOfSamples;
	vector<Buffer *> buffers;
};

// FlexImageFilm Declarations
class FlexImageFilm : public Film {
	// TODO add serialization
public:
	// FlexImageFilm Public Methods
	FlexImageFilm(int xres, int yres) :
		Film(xres, yres), filter(NULL), filterTable(NULL),
		framebuffer(NULL), factor(NULL) { }

	FlexImageFilm(int xres, int yres, Filter *filt, const float crop[4],
		const string &filename1, bool premult, int wI, int dI,
		bool w_tonemapped_EXR, bool w_untonemapped_EXR, bool w_tonemapped_IGI,
		bool w_untonemapped_IGI, bool w_tonemapped_TGA,
		float reinhard_prescale, float reinhard_postscale, float reinhard_burn, float g);
	~FlexImageFilm() {
		delete[] framebuffer;
		delete[] factor;
	}
	//Copy constructor
	FlexImageFilm(const FlexImageFilm &m) : Film(m.xResolution,m.yResolution)
	{
		// TODO write copy constructor for network rendering
	}
	int RequestBuffer(BufferType type, BufferOutputConfig output, const string& filePostfix);
	void CreateBuffers();

	void GetSampleExtent(int *xstart, int *xend, int *ystart, int *yend) const;
	void AddSample(float sX, float sY, const XYZColor &L, float alpha, int buf_id = 0, int bufferGroup = 0);
	void AddSampleCount(float count, int bufferGroup = 0) {
		if (!bufferGroups.empty())
			bufferGroups[bufferGroup].numberOfSamples += count;
	}
	void MergeSampleArray();

	void WriteImage(ImageType type);
	void WriteImage2(ImageType type, float* rgb, float* alpha, string postfix);
	void WriteTGAImage(float *rgb, float *alpha, const string &filename);
	void WriteEXRImage(float *rgb, float *alpha, const string &filename);
	void WriteIGIImage(float *rgb, float *alpha, const string &filename);
	void ScaleOutput(float *rgb, float *alpha, float *scale);

	// GUI display methods
	void updateFrameBuffer();
	unsigned char* getFrameBuffer();
	void clean();
	void merge(FlexImageFilm &f);
	void createFrameBuffer();
	float getldrDisplayInterval() {
		return displayInterval;
	}

	static Film *CreateFilm(const ParamSet &params, Filter *filter);

private:
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

	bool writeTmExr, writeUtmExr, writeTmIgi, writeUtmIgi, writeTmTga;

	unsigned char *framebuffer;
	boost::timer timer;
	bool imageLock;
	float *factor;

	std::vector<BufferConfig> bufferConfigs;
	std::vector<BufferGroup> bufferGroups;

	mutable boost::mutex addSampleMutex;

	float maxY;
	u_int warmupSamples;
	bool warmupComplete;
	ArrSample *SampleArrptr;
	ArrSample *SampleArr2ptr;
	int curSampleArrId, maxSampleArrId;
};

}//namespace lux

#endif //LUX_FLEXIMAGE_H

