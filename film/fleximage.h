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
#include "scene.h"	// for Scene::GetNumberofSamples()
#include "color.h"
#include "spectrum.h"
#include "paramset.h"
#include "tonemap.h"
#include "sampling.h"
#include <boost/timer.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/serialization/base_object.hpp>

#include <stdio.h>

namespace lux{

class BufferConfig {
public:
	BufferConfig(BufferType t, BufferOutputConfig o, const string& s):
		type(t),output(o),postfix(s){}
	BufferType type;
	BufferOutputConfig output;
	string postfix;
};
struct Pixel {
	Pixel(): L(0.f)	{
		alpha = 0.f;
		weightSum = 0.f;
	}
	XYZColor L;
	float alpha, weightSum;
};
class Buffer {
public:
	Buffer(int x, int y)
	{
		xPixelCount = x; yPixelCount = y;
		pixels = new BlockedArray<Pixel>(x, y);
	}
	virtual ~Buffer()
	{
		delete pixels; 
	}
	virtual void Add(int x, int y, XYZColor L, float alpha, float wt) = 0;
	virtual void Clean() = 0;
	virtual void GetData(float *rgb, float *alpha) = 0;
	bool isFramebuffer;
	int xPixelCount, yPixelCount;
	BlockedArray<Pixel> *pixels;
	float scaleFactor;

	static Scene *scene;
};

// Per pixel normalized buffer
class RawBuffer : public Buffer {
public:

	RawBuffer(int x, int y): Buffer(x,y) {
	}

	~RawBuffer() {}

	void Add(int x, int y, XYZColor L, float alpha, float wt) {
		Pixel &pixel = (*pixels)(x, y);
		pixel.L.AddWeighted(wt, L);
		pixel.alpha += alpha * wt;
		pixel.weightSum += wt;
	}
	void GetData(float *rgb, float *alpha)
	{
		int x,y;
		int offset = 0;
		for (y = 0; y < yPixelCount; ++y)
		{
			for (x = 0; x < xPixelCount; ++x,++offset)
			{
				rgb[3*offset  ] = (*pixels)(x, y).L.c[0];
				rgb[3*offset+1] = (*pixels)(x, y).L.c[1];
				rgb[3*offset+2] = (*pixels)(x, y).L.c[2];
				alpha[offset] = (*pixels)(x, y).alpha;
			}
		}
	}

	void Clean() {
	}
};

// Per pixel normalized buffer
class PerPixelNormalizedBuffer : public Buffer {
public:

	PerPixelNormalizedBuffer(int x, int y): Buffer(x,y) {
	}

	~PerPixelNormalizedBuffer() {}

	void Add(int x, int y, XYZColor L, float alpha, float wt) {
		Pixel &pixel = (*pixels)(x, y);
		pixel.L.AddWeighted(wt, L);
		pixel.alpha += alpha * wt;
		pixel.weightSum += wt;
		//L.Print(stdout);printf("\n");
	}
	void GetData(float *rgb, float *alpha)
	{
		int x,y;
		float inv;
		int offset = 0;
		for (y = 0; y < yPixelCount; ++y)
		{
			for (x = 0; x < xPixelCount; ++x,++offset)
			{
				//printf("%f ",(*pixels)(x, y).weightSum);
				inv = 1.0f / (*pixels)(x, y).weightSum;
				if (isinf(inv))
				{
					alpha[offset] = 0;
					rgb[3*offset  ] = 0;
					rgb[3*offset+1] = 0;
					rgb[3*offset+2] = 0;
				}
				else
				{
					// Convert pixel XYZ radiance to RGB
					(*pixels)(x, y).L.ToRGB(rgb+3*offset);
					alpha[offset] = (*pixels)(x, y).alpha;
					rgb[3*offset  ] *= inv;
					rgb[3*offset+1] *= inv;
					rgb[3*offset+2] *= inv;
				}
			}
		}
	}
	void Clean() {
	}
};

// Per screen normalized  buffer
class PerScreenNormalizedBuffer : public Buffer {
public:

	PerScreenNormalizedBuffer(int x, int y): Buffer(x,y) {
	}

	~PerScreenNormalizedBuffer() {}

	void Add(int x, int y, XYZColor L, float alpha, float wt) {
		Pixel &pixel = (*pixels)(x, y);
		pixel.L.AddWeighted(wt, L);
		pixel.alpha += alpha * wt;
	}
	void GetData(float *rgb, float *alpha)
	{
		int x,y;
		assert(scene);
		float inv = 1.0f / scene->GetNumberOfSamples();
		int offset = 0;
		for (y = 0; y < yPixelCount; ++y)
		{
			for (x = 0; x < xPixelCount; ++x,++offset)
			{
				// Convert pixel XYZ radiance to RGB
				(*pixels)(x, y).L.ToRGB(rgb+3*offset);
				alpha[offset] = (*pixels)(x, y).alpha;
				rgb[3*offset  ] *= inv;
				rgb[3*offset+1] *= inv;
				rgb[3*offset+2] *= inv;
			}
		}
	}
	void Clean() {
	}
};


class BufferGroup {
public:
	BufferGroup() {
	}
	~BufferGroup() {
		for(int i=0; i < buffers.size(); i++)
			delete buffers[i];
	}

	void CreateBuffers(const std::vector<BufferConfig> & config, int x, int y) {
		int i;
		Buffer* buf;
		for(i=0;i<config.size();++i)
		{
			if (config[i].type==BUF_TYPE_PER_PIXEL)
				buf = new PerPixelNormalizedBuffer(x,y);
			else if (config[i].type==BUF_TYPE_PER_SCREEN)
				buf = new PerScreenNormalizedBuffer(x,y);
			else if (config[i].type==BUF_TYPE_RAW)
				buf = new RawBuffer(x,y);
			else
				assert(0);
			buffers.push_back(buf);
		}
	}

	Buffer* getBuffer(int index)
	{
		return buffers[index];
	}
	std::vector<Buffer *> buffers;
};

// FlexImageFilm Declarations
class FlexImageFilm : public Film {
public:
	// FlexImageFilm Public Methods
	FlexImageFilm(int xres, int yres) : Film(xres,yres) { filter=NULL; filterTable=NULL; }; 

	FlexImageFilm::FlexImageFilm(int xres, int yres, Filter *filt, const float crop[4],
		bool outhdr, bool outldr,
		const string &filename1, bool premult, int wI,
		const string &tm, float c_dY, float n_MY,
		float reinhard_prescale, float reinhard_postscale, float reinhard_burn,
		float bw, float br, float g, float d);
	~FlexImageFilm(){
		delete[] framebuffer;
		delete[] factor;
		//while(buffers.size()>0)
		//{
		//	delete buffers.end();
		//	buffers.pop_back();
		//}
	}
	int RequestBuffer(BufferType type, BufferOutputConfig output, const string& filePostfix);
	void CreateBuffers();
	void GetSampleExtent(int *xstart, int *xend, int *ystart, int *yend) const;
	void AddSample(float sX, float sY, const XYZColor &L, float alpha, int id=0);
	void WriteImage(ImageType type);
	void WriteImage2(ImageType type, float* rgb, float* alpha, string postfix);
	void WriteTGAImage(float *rgb, float *alpha, const string &filename);
	void WriteEXRImage(float *rgb, float *alpha, const string &filename);
	void ScaleOutput(float *rgb, float *alpha, float *scale);

	// GUI display methods
	void updateFrameBuffer();
	unsigned char* getFrameBuffer();
	void clean();
	void merge(FlexImageFilm &f);
	void createFrameBuffer();
	float getldrDisplayInterval()
	{
		return writeInterval;
	}


	static Film *CreateFilm(const ParamSet &params, Filter *filter);

private:
	// FlexImageFilm Private Data
	Filter *filter;
	int writeInterval;
	string filename;
	ImageType outputType;
	bool premultiplyAlpha, buffersInited;
	float cropWindow[4], *filterTable;
	int xPixelStart, yPixelStart, xPixelCount, yPixelCount;
	string toneMapper;
	ParamSet toneParams;
	float contrastDisplayAdaptationY, nonlinearMaxY,
		reinhardPrescale, reinhardPostscale, reinhardBurn,
		bloomWidth, bloomRadius, gamma, dither;

	unsigned char *framebuffer;
	boost::timer timer;
	bool imageLock;
	float *factor;

	std::vector<BufferConfig> bufferConfigs;
	std::vector<BufferGroup> bufferGroups;

	mutable boost::mutex addSampleMutex;
};

}//namespace lux

#endif //LUX_FLEXIMAGE_H

