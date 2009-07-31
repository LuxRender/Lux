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
#include <boost/thread/mutex.hpp>

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

// XYZColor Pixel
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

// Floating Point Value Pixel 
struct FloatPixel {
	// Dade - serialization here is required by network rendering
	friend class boost::serialization::access;

	template<class Archive> void serialize(Archive & ar, const unsigned int version) {
		ar & V;
		ar & weightSum;
	}

	FloatPixel(): V(0.f), weightSum(0.f) { }
	float V, weightSum;
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

	virtual void GetData(Color *color, float *alpha) const = 0;
	virtual float GetData(int x, int y, Color *color, float *alpha) const = 0;
	bool isFramebuffer;
	int xPixelCount, yPixelCount;
	float scaleFactor;
	BlockedArray<Pixel> *pixels;
};

// Per pixel normalized buffer
class RawBuffer : public Buffer {
public:
	RawBuffer(int x, int y) : Buffer(x, y) { }

	virtual ~RawBuffer() { }

	virtual void GetData(Color *color, float *alpha) const {
		for (int y = 0, offset = 0; y < yPixelCount; ++y) {
			for (int x = 0; x < xPixelCount; ++x, ++offset) {
				const Pixel &pixel = (*pixels)(x, y);
				color[offset] = pixel.L;
				alpha[offset] = pixel.alpha;
			}
		}
	}
	virtual float GetData(int x, int y, Color *color, float *alpha) const {
		const Pixel &pixel = (*pixels)(x, y);
		*color = pixel.L;
		*alpha = pixel.alpha;
		return pixel.weightSum;
	}
};

// Per pixel normalized XYZColor buffer
class PerPixelNormalizedBuffer : public Buffer {
public:
	PerPixelNormalizedBuffer(int x, int y) : Buffer(x, y) { }

	virtual ~PerPixelNormalizedBuffer() { }

	virtual void GetData(Color *color, float *alpha) const {
		for (int y = 0, offset = 0; y < yPixelCount; ++y) {
			for (int x = 0; x < xPixelCount; ++x, ++offset) {
				const Pixel &pixel = (*pixels)(x, y);
				if (pixel.weightSum == 0.f) {
					color[offset] = XYZColor(0.f);
					alpha[offset] = 0.f;
				} else {
					float inv = 1.f / pixel.weightSum;
					color[offset] = pixel.L * inv;
					alpha[offset] = pixel.alpha * inv;
				}
			}
		}
	}
	virtual float GetData(int x, int y, Color *color, float *alpha) const {
		const Pixel &pixel = (*pixels)(x, y);
		if (pixel.weightSum == 0.f) {
			*color = XYZColor(0.f);
			*alpha = 0.f;
		} else {
			*color = pixel.L / pixel.weightSum;
			*alpha = pixel.alpha;
		}
		return pixel.weightSum;
	}
};

// Per pixel normalized floating point buffer
class PerPixelNormalizedFloatBuffer {
public:
	PerPixelNormalizedFloatBuffer(int x, int y) {
		floatpixels = new BlockedArray<FloatPixel>(x, y);
	}

	~PerPixelNormalizedFloatBuffer() {
		delete floatpixels;
	}

	void Add(int x, int y, float value, float wt) {
		FloatPixel &fpixel = (*floatpixels)(x, y);
		fpixel.V += value;
		fpixel.weightSum += wt;
	}

/*	void GetData(Color *color, float *alpha) const {
		for (int y = 0, offset = 0; y < yPixelCount; ++y) {
			for (int x = 0; x < xPixelCount; ++x, ++offset) {
				const Pixel &pixel = (*pixels)(x, y);
				if (pixel.weightSum == 0.f) {
					color[offset] = XYZColor(0.f);
					alpha[offset] = 0.f;
				} else {
					float inv = 1.f / pixel.weightSum;
					color[offset] = pixel.L * inv;
					alpha[offset] = pixel.alpha * inv;
				}
			}
		}
	}
	*/
	float GetData(int x, int y) const {
		const FloatPixel &pixel = (*floatpixels)(x, y);
		if (pixel.weightSum == 0.f) {
			return 0.f;
		}
		return pixel.V / pixel.weightSum;
	} 
private:
	BlockedArray<FloatPixel> *floatpixels;
};

// Per screen normalized XYZColor buffer
class PerScreenNormalizedBuffer : public Buffer {
public:
	PerScreenNormalizedBuffer(int x, int y, const double *samples) :
		Buffer(x, y), numberOfSamples_(samples) { }

	virtual ~PerScreenNormalizedBuffer() { }

	virtual void GetData(Color *color, float *alpha) const {
		const double inv = xPixelCount * yPixelCount / *numberOfSamples_;
		for (int y = 0, offset = 0; y < yPixelCount; ++y) {
			for (int x = 0; x < xPixelCount; ++x, ++offset) {
				const Pixel &pixel = (*pixels)(x, y);
				color[offset] = pixel.L * inv;
				if (pixel.weightSum > 0.f)
					alpha[offset] = pixel.alpha / pixel.weightSum;
				else
					alpha[offset] = 0.f;
			}
		}
	}
	virtual float GetData(int x, int y, Color *color, float *alpha) const {
		const Pixel &pixel = (*pixels)(x, y);
		if (pixel.weightSum > 0.f) {
			*color = pixel.L * xPixelCount * yPixelCount / *numberOfSamples;
			*alpha = pixel.alpha;
		} else {
			*color = 0.f;
			*alpha = 0.f;
		}
		return pixel.weightSum;
	}
private:
	const double *numberOfSamples_;
};


class BufferGroup {
public:
	BufferGroup(const string &n) : numberOfSamples(0.f), name(n),
		enable(true), globalScale(1.f), temperature(0.f),
		rgbScale(1.f), scale(1.f) { }
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
	string name;
	bool enable;
	float globalScale, temperature;
	RGBColor rgbScale;
	XYZColor scale;
};

// GREYCStoration Noise Reduction Filter Parameter structure
class GREYCStorationParams {
public:
	GREYCStorationParams() {
		Reset();
	}
	void Reset() {
		enabled = false;		 // GREYCStoration is enabled/disabled
		amplitude = 40.0f;		 // Regularization strength for one iteration (>=0)
		nb_iter = 2;			 // Number of regularization iterations (>0)
		sharpness = 0.8f;		 // Contour preservation for regularization (>=0)
		anisotropy = 0.2f;		 // Regularization anisotropy (0<=a<=1)
		alpha = 0.8f;			 // Noise scale(>=0)
		sigma = 1.1f;			 // Geometry regularity (>=0)
		fast_approx = true;		 // Use fast approximation for regularization (0 or 1)
		gauss_prec = 2.0f;		 // Precision of the gaussian function for regularization (>0)
		dl = 0.8f;				 // Spatial integration step for regularization (0<=dl<=1)
		da = 30.0f;				 // Angular integration step for regulatization (0<=da<=90)
		interp = 0;			     // Interpolation type (0=Nearest-neighbor, 1=Linear, 2=Runge-Kutta)
		tile = 0;				 // Use tiled mode (reduce memory usage)
		btile = 4;				 // Size of tile overlapping regions
		threads = 1;		     // Number of threads used
	}

	bool enabled, fast_approx;
	unsigned int nb_iter, interp, tile, btile, threads;
	float amplitude, sharpness, anisotropy, alpha, sigma, gauss_prec, dl, da;
};

// Chiu Noise Reduction Filter Parameter structure
class ChiuParams {
public:
	ChiuParams() {
		Reset();
	}
	void Reset() {
		enabled = false;		 // Chiu noise filter is enabled/disabled
		radius = 3;				 // 
		includecenter = false;	 // 
	}

	bool enabled, includecenter;
	double radius;
};


//Histogram Declarations
class Histogram {
	public:
		Histogram();
		~Histogram();
		void Calculate(vector<Color> &pixels, unsigned int width, unsigned int height);
		void MakeImage(unsigned char *outPixels, unsigned int width, unsigned int height, int options);
	private:
		void CheckBucketNr();
		int m_bucketNr, m_newBucketNr;
		float* m_buckets;
		int m_zones[11];
		float m_lowRange, m_highRange, m_bucketSize;
		float m_displayGamma;
		boost::mutex m_mutex;
};

// Film Declarations
class Film {
public:
    // Film Interface

    Film(int xres, int yres, int haltspp) :
		xResolution(xres), yResolution(yres), EV(0.f),
		haltSamplePerPixel(haltspp), enoughSamplePerPixel(false) {
		samplePerPass = (double)xResolution * (double)yResolution;
	}
    virtual ~Film() { }
	virtual void AddSample(Contribution *contrib) = 0;
    virtual void AddSampleCount(float count) { }
    virtual void WriteImage(ImageType type) = 0;
	virtual void WriteFilm(const string &filename) = 0;
	// Dade - method useful for transmitting the samples to a client
	virtual void TransmitFilm(std::basic_ostream<char> &stream, bool clearBuffers = true, bool transmitParams = false) = 0;
	virtual float UpdateFilm(Scene *scene, std::basic_istream<char> &stream) = 0;
    virtual void GetSampleExtent(int *xstart, int *xend, int *ystart, int *yend) const = 0;

    virtual void RequestBufferGroups(const vector<string> &bg) = 0;
    virtual int RequestBuffer(BufferType type, BufferOutputConfig output, const string& filePostfix) = 0;

    virtual void CreateBuffers() {
    }
	virtual u_int GetNumBufferConfigs() const = 0;
	virtual const BufferConfig& GetBufferConfig(u_int index) const = 0;
    virtual u_int GetNumBufferGroups() const = 0;
    virtual string GetGroupName(u_int index) const = 0;
    virtual void SetGroupEnable(u_int index, bool status) = 0;
    virtual bool GetGroupEnable(u_int index) const = 0;
    virtual void SetGroupScale(u_int index, float value) = 0;
    virtual float GetGroupScale(u_int index) const = 0;
    virtual void SetGroupRGBScale(u_int index, const RGBColor &value) = 0;
    virtual RGBColor GetGroupRGBScale(u_int index) const = 0;
    virtual void SetGroupTemperature(u_int index, float value) = 0;
    virtual float GetGroupTemperature(u_int index) const = 0;
    virtual unsigned char* getFrameBuffer() = 0;
    virtual void updateFrameBuffer() = 0;
    virtual float getldrDisplayInterval() = 0;
	void getHistogramImage(unsigned char *outPixels, int width, int height, int options);

    void SetScene(Scene *scene1) {
        scene = scene1;
    }

	// Parameter Access functions
	virtual void SetParameterValue(luxComponentParameters param, double value, int index) = 0;
	virtual double GetParameterValue(luxComponentParameters param, int index) = 0;
	virtual double GetDefaultParameterValue(luxComponentParameters param, int index) = 0;
	virtual void SetStringParameterValue(luxComponentParameters param, const string& value, int index) = 0;
	virtual string GetStringParameterValue(luxComponentParameters param, int index) = 0;

    // Film Public Data
    int xResolution, yResolution;
    float EV;

	// Dade - Samplers will check this flag to know if we have enough samples per
	// pixel and it is time to stop
	int haltSamplePerPixel;
	bool enoughSamplePerPixel;

	Scene *scene;

	Histogram m_histogram;

protected:
	// Dade - (xResolution * yResolution)
	double samplePerPass;
};

// Image Pipeline Declarations
extern void ApplyImagingPipeline(vector<Color> &pixels, int xResolution, int yResolution, 
        const GREYCStorationParams &GREYCParams, const ChiuParams &chiuParams,
        ColorSystem &colorSpace, Histogram &histogram, bool HistogramEnabled, bool &haveBloomImage, Color *&bloomImage, bool bloomUpdate,
		float bloomRadius, float bloomWeight, bool VignettingEnabled, float VignetScale,
		bool aberrationEnabled, float aberrationAmount,
		bool &haveGlareImage, Color *&glareImage, bool glareUpdate, float glareAmount, float glareRadius, int glareBlades,
		const char *tonemap, const ParamSet *toneMapParams, float gamma,
        float dither);

}//namespace lux;

#endif // LUX_FILM_H
