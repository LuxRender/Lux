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

#ifndef LUX_FILM_H
#define LUX_FILM_H
// film.h*
#include "lux.h"
#include "color.h"
#include "error.h"
#include "memory.h"

#include <boost/serialization/split_member.hpp>

namespace lux {

enum ImageType {
    IMAGE_NONE = 0, // Don't write anything
    IMAGE_FILEOUTPUT = 1 << 1, // Write image to file
    IMAGE_FRAMEBUFFER = 1 << 2, // Display image
    IMAGE_ALL = IMAGE_FILEOUTPUT | IMAGE_FRAMEBUFFER
};

// Buffer types

enum BufferType {
    BUF_TYPE_PER_PIXEL = 0, // Per pixel normalized buffer
    BUF_TYPE_PER_SCREEN, // Per screen normalized buffer
    BUF_TYPE_RAW, // No normalization
    NUM_OF_BUFFER_TYPES
};

enum BufferOutputConfig {
    BUF_FRAMEBUFFER = 1 << 0, // Buffer is part of the rendered image
    BUF_STANDALONE = 1 << 1, // Buffer is written in its own file
    BUF_RAWDATA = 1 << 2 // Buffer data is written as is
};

class BufferConfig {
public:
	BufferConfig(BufferType t, BufferOutputConfig o, const string& s) :
		type(t), output(o), postfix(s) { }
	BufferType type;
	BufferOutputConfig output;
	string postfix;
};

struct Pixel {
	// Dade - serialization here is required by network rendering
	friend class boost::serialization::access;

	template<class Archive> void serialize(Archive & ar, const unsigned int version) {
		ar & L;
		ar & alpha;
		ar & weightSum;
	}

	Pixel(): L(0.f), alpha(0.f), weightSum(0.f) { }
	XYZColor L;
	float alpha, weightSum;
};

class Buffer {
public:
	Buffer(int x, int y) : xPixelCount(x), yPixelCount(y) {
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

	void Clear() {
		for (int y = 0, offset = 0; y < yPixelCount; ++y) {
			for (int x = 0; x < xPixelCount; ++x, ++offset) {
				Pixel &pixel = (*pixels)(x, y);
				pixel.L.c[0] = 0.0f;
				pixel.L.c[1] = 0.0f;
				pixel.L.c[2] = 0.0f;
				pixel.alpha = 0.0f;
				pixel.weightSum = 0.0f;
			}
		}
	}

	virtual void GetData(float *rgb, float *alpha) const = 0;
	virtual void GetData(int x, int y, Color *color, float *alpha) const = 0;
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

	void GetData(float *rgb, float *alpha) const {
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
	void GetData(int x, int y, Color *color, float *alpha) const {
		*color = (*pixels)(x, y).L;
		*alpha = (*pixels)(x, y).alpha;
	}
};

// Per pixel normalized buffer
class PerPixelNormalizedBuffer : public Buffer {
public:
	PerPixelNormalizedBuffer(int x, int y) : Buffer(x, y) { }

	~PerPixelNormalizedBuffer() { }

	void GetData(float *rgb, float *alpha) const {
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
					alpha[offset] = pixel.alpha * inv;
				}
			}
		}
	}
	void GetData(int x, int y, Color *color, float *alpha) const {
		if ((*pixels)(x, y).weightSum == 0.f) {
			*color = XYZColor(0.f);
			*alpha = 0.f;
		} else {
			const float inv = 1.f / (*pixels)(x, y).weightSum;
			*color = (*pixels)(x, y).L * inv;
			*alpha = (*pixels)(x, y).alpha * inv;
		}
	}
};

// Per screen normalized  buffer
class PerScreenNormalizedBuffer : public Buffer {
public:
	PerScreenNormalizedBuffer(int x, int y, const double *samples) :
		Buffer(x, y), numberOfSamples_(samples) { }

	~PerScreenNormalizedBuffer() { }

	void GetData(float *rgb, float *alpha) const {
		const double inv = xPixelCount * yPixelCount / *numberOfSamples_;
		for (int y = 0, offset = 0; y < yPixelCount; ++y) {
			for (int x = 0; x < xPixelCount; ++x, ++offset) {
				Pixel &pixel = (*pixels)(x, y);
				// Convert pixel XYZ radiance to RGB
				pixel.L.ToRGB(rgb + 3 * offset);
				rgb[3*offset  ] *= inv;
				rgb[3*offset+1] *= inv;
				rgb[3*offset+2] *= inv;
				alpha[offset] = pixel.alpha * inv;
			}
		}
	}
	void GetData(int x, int y, Color *color, float *alpha) const {
		const double inv = xPixelCount * yPixelCount / *numberOfSamples_;
		*color = (*pixels)(x, y).L * inv;
		*alpha = (*pixels)(x, y).alpha * inv;
	}
private:
	const double *numberOfSamples_;
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
	double numberOfSamples;
	vector<Buffer *> buffers;
};

//class FlexImageFilm;

class ArrSample {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & sX;
        ar & sY;
        ar & xyz;
        ar & alpha;
        ar & buf_id;
        ar & bufferGroup;
    }

public:
    void Sample() {
        sX = 0;
        sY = 0;
        xyz = XYZColor(0.);
        alpha = 0;
        buf_id = 0;
        bufferGroup = 0;
    }

    float sX;
    float sY;
    XYZColor xyz;
    float alpha;
    int buf_id;
    int bufferGroup;
};

// Film Declarations

class Film {
public:
    // Film Interface

    Film(int xres, int yres, int haltspp) :
		xResolution(xres), yResolution(yres), haltSamplePerPixel(haltspp),
		enoughSamplePerPixel(false) {
		invSamplePerPass =  1.0 / (xResolution * yResolution);
	}
    virtual ~Film() { }
	virtual void AddSample(Contribution *contrib) = 0;
    virtual void AddSampleCount(float count, int bufferGroup = 0) { }
    virtual void WriteImage(ImageType type) = 0;
    virtual void GetSampleExtent(int *xstart, int *xend, int *ystart, int *yend) const = 0;

    virtual int RequestBuffer(BufferType type, BufferOutputConfig output, const string& filePostfix) {
        return 0;
    }

    virtual void CreateBuffers() {
    }
    virtual unsigned char* getFrameBuffer() = 0;
    virtual void updateFrameBuffer() = 0;
    virtual float getldrDisplayInterval() = 0;

    void SetScene(Scene *scene1) {
        scene = scene1;
    }

    // Film Public Data
    int xResolution, yResolution;

	// Dade - Samplers will check this flag to know if we have enough samples per
	// pixel and it is time to stop
	int haltSamplePerPixel;
	bool enoughSamplePerPixel;

	Scene *scene;

protected:
	// Dade - 1.0 / (xResolution * yResolution)
	double invSamplePerPass;
};

// Image Pipeline Declarations
extern void ApplyImagingPipeline(vector<Color> &pixels, int xResolution, int yResolution,
        ColorSystem &colorSpace, float bloomRadius = .2f,
        float bloomWeight = 0.f, const char *tonemap = NULL,
        const ParamSet *toneMapParams = NULL, float gamma = 2.2,
        float dither = 0.5f);

}//namespace lux;

#endif // LUX_FILM_H
