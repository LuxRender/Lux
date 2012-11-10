/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
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
#include "api.h"
#include "color.h"
#include "memory.h"
#include "queryable.h"
#include "bsh.h"
#include "luxrays/utils/convtest/convtest.h"
#include "mcdistribution.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/shared_array.hpp>

namespace lux {

enum ImageType {
    IMAGE_NONE = 0, // Don't write anything
    IMAGE_FILEOUTPUT = 1 << 0, // Write image to file
    IMAGE_FLMOUTPUT = 1 << 1, // Write flm file
    IMAGE_FRAMEBUFFER = 1 << 2, // Display image
	IMAGE_FILE_ALL = IMAGE_FILEOUTPUT | IMAGE_FLMOUTPUT, // All filebased output
    IMAGE_ALL = IMAGE_FLMOUTPUT | IMAGE_FILEOUTPUT | IMAGE_FRAMEBUFFER
};

// Buffer types

enum BufferType {
    BUF_TYPE_PER_PIXEL = 0, // Per pixel normalized buffer
    BUF_TYPE_PER_SCREEN, // Per screen normalized buffer
    BUF_TYPE_PER_SCREEN_SCALED, // Per screen with custom normalisation factor. Internal use of SPPM
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
	Buffer(u_int x, u_int y) : pixels(x, y) {
		xPixelCount = pixels.uSize();
		yPixelCount = pixels.vSize();
	}

	virtual ~Buffer() { }

	void Add(u_int x, u_int y, XYZColor L, float alpha, float wt) {
		Pixel &pixel = pixels(x, y);
		pixel.L.AddWeighted(wt, L);
		pixel.alpha += alpha * wt;
		pixel.weightSum += wt;
	}

	void Set(u_int x, u_int y, XYZColor L, float alpha) {
		Pixel &pixel = pixels(x, y);
		pixel.L = L;
		pixel.alpha = alpha;
		pixel.weightSum = 1.f;
	}

	void Clear() {
		for (u_int y = 0, offset = 0; y < yPixelCount; ++y) {
			for (u_int x = 0; x < xPixelCount; ++x, ++offset) {
				Pixel &pixel = pixels(x, y);
				pixel.L.c[0] = 0.0f;
				pixel.L.c[1] = 0.0f;
				pixel.L.c[2] = 0.0f;
				pixel.alpha = 0.0f;
				pixel.weightSum = 0.0f;
			}
		}
	}

	virtual void GetData(XYZColor *color, float *alpha) const = 0;
	virtual float GetData(u_int x, u_int y, XYZColor *color, float *alpha) const = 0;
	u_int xPixelCount, yPixelCount;
	BlockedArray<Pixel> pixels;
	float scaleFactor;
	bool isFramebuffer;
};

// Per pixel normalized buffer
class RawBuffer : public Buffer {
public:
	RawBuffer(u_int x, u_int y) : Buffer(x, y) { }

	virtual ~RawBuffer() { }

	virtual void GetData(XYZColor *color, float *alpha) const {
		for (u_int y = 0, offset = 0; y < yPixelCount; ++y) {
			for (u_int x = 0; x < xPixelCount; ++x, ++offset) {
				const Pixel &pixel = pixels(x, y);
				color[offset] = pixel.L;
				alpha[offset] = pixel.alpha;
			}
		}
	}
	virtual float GetData(u_int x, u_int y, XYZColor *color, float *alpha) const {
		const Pixel &pixel = pixels(x, y);
		*color = pixel.L;
		*alpha = pixel.alpha;
		return pixel.weightSum;
	}
};

// Per pixel normalized XYZColor buffer
class PerPixelNormalizedBuffer : public Buffer {
public:
	PerPixelNormalizedBuffer(u_int x, u_int y) : Buffer(x, y) { }

	virtual ~PerPixelNormalizedBuffer() { }

	virtual void GetData(XYZColor *color, float *alpha) const {
		for (u_int y = 0, offset = 0; y < yPixelCount; ++y) {
			for (u_int x = 0; x < xPixelCount; ++x, ++offset) {
				const Pixel &pixel = pixels(x, y);
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
	virtual float GetData(u_int x, u_int y, XYZColor *color, float *alpha) const {
		const Pixel &pixel = pixels(x, y);
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
	PerPixelNormalizedFloatBuffer(u_int x, u_int y) : floatpixels(x, y) { }

	~PerPixelNormalizedFloatBuffer() { }

	void Add(u_int x, u_int y, float value, float wt) {
		FloatPixel &fpixel = floatpixels(x, y);
		fpixel.V += value;
		fpixel.weightSum += wt;
	}

	void Set(u_int x, u_int y, float value, float wt) {
		FloatPixel &fpixel = floatpixels(x, y);
		fpixel.V = value;
		fpixel.weightSum = 1.f;
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
	float GetData(u_int x, u_int y) const {
		const FloatPixel &pixel = floatpixels(x, y);
		if (pixel.weightSum == 0.f) {
			return 0.f;
		}
		return pixel.V / pixel.weightSum;
	} 
	BlockedArray<FloatPixel> floatpixels;
};

// Per screen normalized XYZColor buffer
class PerScreenNormalizedBuffer : public Buffer {
public:
	PerScreenNormalizedBuffer(u_int x, u_int y, const double *samples) :
		Buffer(x, y), numberOfSamples_(samples) { }

	virtual ~PerScreenNormalizedBuffer() { }

	virtual void GetData(XYZColor *color, float *alpha) const {
		const float inv = static_cast<float>(xPixelCount * yPixelCount / *numberOfSamples_);
		for (u_int y = 0, offset = 0; y < yPixelCount; ++y) {
			for (u_int x = 0; x < xPixelCount; ++x, ++offset) {
				const Pixel &pixel = pixels(x, y);
				color[offset] = pixel.L * inv;
				if (pixel.weightSum > 0.f)
					alpha[offset] = pixel.alpha / pixel.weightSum;
				else
					alpha[offset] = 0.f;
			}
		}
	}
	virtual float GetData(u_int x, u_int y, XYZColor *color, float *alpha) const {
		const Pixel &pixel = pixels(x, y);
		if (pixel.weightSum > 0.f) {
			*color = pixel.L * static_cast<float>(xPixelCount * yPixelCount / *numberOfSamples_);
			*alpha = pixel.alpha;
		} else {
			*color = XYZColor(0.f);
			*alpha = 0.f;
		}
		return pixel.weightSum;
	}
private:
	const double *numberOfSamples_;
};

// this buffer is used by SPPM
// It is a copyPaste of PerScreenNormalizedBuffer, but without the x*y
// normalization facter and with added a custom normalisation factor.
// TODO scale is initialized to 1.0 but this is not correct for AMC
class PerScreenNormalizedBufferScaled : public Buffer {
public:
	PerScreenNormalizedBufferScaled(u_int x, u_int y, const double *samples) :
		Buffer(x, y), numberOfSamples_(samples), scaleUpdate(NULL), scale(1.0) { }

	virtual ~PerScreenNormalizedBufferScaled() {}

	virtual void GetData(XYZColor *color, float *alpha) const {
		scale = scaleUpdate->GetScaleFactor(*numberOfSamples_);
		for (u_int y = 0, offset = 0; y < yPixelCount; ++y) {
			for (u_int x = 0; x < xPixelCount; ++x, ++offset) {
				const Pixel &pixel = pixels(x, y);
				if (pixel.weightSum > 0.f) {
					color[offset] = pixel.L * scale;
					alpha[offset] = pixel.alpha;
				} else {
					color[offset] = 0.f;
					alpha[offset] = 0.f;
				}
			}
		}
	}
	virtual float GetData(u_int x, u_int y, XYZColor *color, float *alpha) const {
		if(x == 0 && y == 0 && scaleUpdate != NULL)
			scale = scaleUpdate->GetScaleFactor(*numberOfSamples_);

		const Pixel &pixel = pixels(x, y);
		if (pixel.weightSum > 0.f) {
			*color = pixel.L * static_cast<float>(scale);
			*alpha = pixel.alpha;
		} else {
			*color = XYZColor(0.f);
			*alpha = 0.f;
		}
		return pixel.weightSum;
	}

	class ScaleUpdateInterface
	{
		public:
			virtual float GetScaleFactor(const double nos) = 0;
	};
	const double *numberOfSamples_;

	ScaleUpdateInterface *scaleUpdate;
private:
	mutable float scale;
};


class BufferGroup {
public:
	BufferGroup(const string &n) : numberOfSamples(0.f), name(n),
		globalScale(1.f), temperature(0.f),
		rgbScale(1.f), convert(XYZColor(1.f), XYZColor(1.f)),
		enable(true) { }
	~BufferGroup() {
		for(vector<Buffer *>::iterator buffer = buffers.begin(); buffer != buffers.end(); ++buffer)
			delete *buffer;
	}

	void CreateBuffers(const vector<BufferConfig> &configs, u_int x, u_int y);

	Buffer *getBuffer(u_int index) const {
		return buffers[index];
	}
	double numberOfSamples;
	vector<Buffer *> buffers;
	string name;
	float globalScale, temperature;
	RGBColor rgbScale;
	ColorAdaptator convert;
	bool enable;
};

// GREYCStoration Noise Reduction Filter Parameter structure
class GREYCStorationParams {
public:
	GREYCStorationParams() { Reset(); }
	void Reset() {
		enabled = false;		 // GREYCStoration is enabled/disabled
		amplitude = 60.0f;		 // Regularization strength for one iteration (>=0)
		nb_iter = 1;			 // Number of regularization iterations (>0)
		sharpness = 0.91f;		 // Contour preservation for regularization (>=0)
		anisotropy = 0.3f;		 // Regularization anisotropy (0<=a<=1)
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

	unsigned int nb_iter, interp, tile, btile, threads;
	float amplitude, sharpness, anisotropy, alpha, sigma, gauss_prec, dl, da;
	bool enabled, fast_approx;
};

// Chiu Noise Reduction Filter Parameter structure
class ChiuParams {
public:
	ChiuParams() { Reset(); }
	void Reset() {
		enabled = false;		 // Chiu noise filter is enabled/disabled
		radius = 3;				 // 
		includecenter = false;	 // 
	}

	float radius;
	bool enabled, includecenter;
};


//Histogram Declarations
class Histogram {
public:
	Histogram();
	~Histogram();
	void Calculate(vector<RGBColor> &pixels, unsigned int width, unsigned int height);
	void MakeImage(unsigned char *outPixels, unsigned int width, unsigned int height, int options);
private:
	void CheckBucketNr();
	u_int m_bucketNr, m_newBucketNr;
	float *m_buckets;
	u_int m_zones[11];
	float m_lowRange, m_highRange, m_bucketSize;
	float m_displayGamma;
	boost::mutex m_mutex;
};

//------------------------------------------------------------------------------
// Used to compute variance
//------------------------------------------------------------------------------

struct VariancePixel {
	VariancePixel() : Sn(0.f), mean(0.f), weightSum(0.f) { }

	float Sn, mean, weightSum;
};

class VarianceBuffer {
public:
	VarianceBuffer(u_int x, u_int y) : pixels(x, y) {
	}

	~VarianceBuffer() { }

	void Add(u_int x, u_int y, XYZColor L, float wt) {
		if (wt == 0.f)
			return;

		VariancePixel &pixel = pixels(x, y);

		const float v = L.Y();
		const float newWeightSum = pixel.weightSum + wt;

		// Incremental computation of weighted mean:
		// mean_n = mean_n-1 + (weight_n / weightSum_n)(x_n − mean_n−1 )
		const float newMean = pixel.mean + (wt / newWeightSum) * (v - pixel.mean);

		// Incremental computation of weighted variance:
		//  S_n = S_n−1 + weight_n (x_n − mean_n−1)(x_n − mean_n)
		//  Var = S_n / weightSum_n
		const float newSn = pixel.Sn + wt * (v - pixel.mean) * (v - newMean);

		pixel.Sn = newSn;
		pixel.mean = newMean;
		pixel.weightSum = newWeightSum;
	}

	void Clear() {
		for (u_int y = 0; y < pixels.vSize(); ++y) {
			for (u_int x = 0; x < pixels.uSize(); ++x) {
				VariancePixel &pixel = pixels(x, y);
				pixel.Sn = 0.f;
				pixel.mean = 0.f;
				pixel.weightSum = 0.f;
			}
		}
	}

	float GetVariance(u_int x, u_int y) const {
		const VariancePixel &pixel = pixels(x, y);

		// Var = S_n / weightSum_n
		if (pixel.weightSum > 0.f)
			return fabs(pixel.Sn / pixel.weightSum);
		else
			return -1.f; // -1 means a pixel that have yet to be sampled
	}

	BlockedArray<VariancePixel> pixels;
};

//------------------------------------------------------------------------------
// Filter Look Up Table
//------------------------------------------------------------------------------

class FilterLUT {
public:
	FilterLUT() : lut() { }

	FilterLUT(Filter *filter, const float offsetX, const float offsetY);

	~FilterLUT() { }

	const u_int GetWidth() const { return lutWidth; }
	const u_int GetHeight() const { return lutHeight; }

	const float *GetLUT() const {
		return &lut.front();
	}

private:
	u_int lutWidth, lutHeight;
	std::vector<float> lut;
};

class FilterLUTs {
public:
	FilterLUTs(Filter *filter, const u_int size);

	~FilterLUTs() {	}

	const FilterLUT &GetLUT(const float x, const float y) const {
		const int ix = max<int>(0, min<int>(Floor2Int(lutsSize * (x + 0.5f)), lutsSize - 1));
		const int iy = max<int>(0, min<int>(Floor2Int(lutsSize * (y + 0.5f)), lutsSize - 1));

		return luts[ix + iy * lutsSize];
	}

private:
	unsigned int lutsSize;
	float step;
	std::vector<FilterLUT> luts;
};

class OutlierDataXYRGB {
public:
	typedef PointN<5> Point_t;

	OutlierDataXYRGB() : p() {
	}

	OutlierDataXYRGB(float x, float y, const XYZColor &color) {
		RGBColor rgb = cs.ToRGBConstrained(color);
		p.x[0] = x;
		p.x[1] = y;
		//p.x[2] = rgb.c[0] * (1.f / 255.f);
		//p.x[3] = rgb.c[1] * (1.f / 255.f);
		//p.x[4] = rgb.c[2] * (1.f / 255.f);
		p.x[2] = logf(1.f + rgb.c[0]);
		p.x[3] = logf(1.f + rgb.c[1]);
		p.x[4] = logf(1.f + rgb.c[2]);
	}

	Point_t p;

	static ColorSystem cs;
};

class OutlierDataRGB {
public:
	typedef PointN<3> Point_t;

	OutlierDataRGB() : p() {
	}

	OutlierDataRGB(float x, float y, const XYZColor &color) {
		RGBColor rgb = cs.ToRGBConstrained(color);
		p.x[0] = rgb.c[0] * (1.f / 255.f);
		p.x[1] = rgb.c[1] * (1.f / 255.f);
		p.x[2] = rgb.c[2] * (1.f / 255.f);
	}

	Point_t p;

	static ColorSystem cs;
};

class OutlierDataXYLY {
public:
	typedef PointN<3> Point_t;

	OutlierDataXYLY() : p() {
	}

	OutlierDataXYLY(float x, float y, const XYZColor &color) {
		p.x[0] = x;
		p.x[1] = y;
		p.x[2] = logf(1.f + color.Y());
	}

	Point_t p;

	static ColorSystem cs;
};

//typedef OutlierDataXYRGB OutlierData;
typedef OutlierDataXYLY OutlierData;

// Film Declarations
class LUX_EXPORT Film : public Queryable {
public:
	// Film Interface
	Film(u_int xres, u_int yres, Filter *filt, u_int filtRes, const float crop[4],
		const string &filename1, bool premult, bool useZbuffer,
		bool w_resume_FLM, bool restart_resume_FLM, bool write_FLM_direct,
		int haltspp, int halttime, float haltthreshold, bool debugmode, int outlierk,
		int tilecount, const string &samplingmapfilename);

	virtual ~Film();

	/*
	 * Adds a contribution to the film.
	 * Not thread-safe!
	 */
	virtual void AddSample(Contribution *contrib);
	/*
	 * Adds contributions to a given tile of the film.
	 * This method is thread-safe for different tiles.
	 * @param contribs Array of contributions to add
	 * @param num_contribs Number of contributions in the contribs array
	 * @param tileIndex Index of the tile the contributions should be added to
	 */
	virtual void AddTileSamples(const Contribution* const contribs, u_int num_contribs,
		u_int tileIndex);
	virtual void SetSample(const Contribution *contrib);
	virtual void AddSampleCount(float count);
	virtual void GetSampleExtent(int *xstart, int *xend, int *ystart, int *yend) const;

	virtual void SaveEXR(const string &exrFilename, bool useHalfFloats, bool includeZBuf, int compressionType, bool tonemapped) {
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SaveEXR not implemented";
	}
	virtual void WriteImage(ImageType type) = 0;
	virtual void CheckWriteOuputInterval() { }

	virtual bool WriteFilmToFile(const string &filename);
	virtual bool WriteFilmToStream(std::basic_ostream<char> &stream, bool clearBuffers = true, bool transmitParams = false, bool directWrite = false);
	virtual double MergeFilmFromFile(const std::string& filename);
	virtual double MergeFilmFromStream(std::basic_istream<char> &stream);
	virtual bool LoadResumeFilm(const string &filename);

	virtual void RequestBufferGroups(const vector<string> &bg);
	virtual u_int RequestBuffer(BufferType type, BufferOutputConfig output, const string& filePostfix);

	virtual void CreateBuffers();
	virtual u_int GetNumBufferConfigs() const { return bufferConfigs.size(); }
	virtual const BufferConfig& GetBufferConfig(u_int index) const { return bufferConfigs[index]; }
	virtual u_int GetNumBufferGroups() const { return bufferGroups.size(); }
	virtual const BufferGroup& GetBufferGroup(u_int index) const { return bufferGroups[index]; }
	virtual void ClearBuffers();

	/**
	 * Get the indexes that the current contribution spans.
	 * Current implementation is limited to at most two tiles, ie the tiles are slabs.
	 * @param tileIndex0 First tile index, always set.
	 * @param tileIndex1 Second tile index if the contribution spans more than one tile, otherwise undefined.
	 * @return Number of tiles that the contribution spans, 1 or 2.
	 */
	virtual u_int GetTileIndexes(const Contribution &contrib, u_int *tile0, u_int *tile1) const;
	/*
	 * Returns the total number of tiles in the film.
	 * @return Total number of tiles in the film.
	 */
	virtual u_int GetTileCount() const;

	virtual void SetGroupName(u_int index, const string& name);
	virtual string GetGroupName(u_int index) const;
	virtual void SetGroupEnable(u_int index, bool status);
	virtual bool GetGroupEnable(u_int index) const;
	virtual void SetGroupScale(u_int index, float value);
	virtual float GetGroupScale(u_int index) const;
	virtual void SetGroupRGBScale(u_int index, const RGBColor &value);
	virtual RGBColor GetGroupRGBScale(u_int index) const;
	virtual void SetGroupTemperature(u_int index, float value);
	virtual float GetGroupTemperature(u_int index) const;
	virtual void ComputeGroupScale(u_int index);

	virtual unsigned char* getFrameBuffer() = 0;
	virtual float* getFloatFrameBuffer() = 0;
	virtual float* getAlphaBuffer() = 0;
	virtual float* getZBuffer() = 0;
	virtual void updateFrameBuffer() = 0;
	virtual int getldrDisplayInterval() = 0;
	void getHistogramImage(unsigned char *outPixels, u_int width, u_int height, int options);

	// Parameter Access functions
	virtual void SetParameterValue(luxComponentParameters param, double value, u_int index) = 0;
	virtual double GetParameterValue(luxComponentParameters param, u_int index) = 0;
	virtual double GetDefaultParameterValue(luxComponentParameters param, u_int index) = 0;
	virtual void SetStringParameterValue(luxComponentParameters param, const string& value, u_int index) = 0;
	virtual string GetStringParameterValue(luxComponentParameters param, u_int index) = 0;

	virtual void EnableNoiseAwareMap();
	virtual const bool GetNoiseAwareMap(u_int &version, boost::shared_array<float> &map,
		boost::shared_ptr<Distribution2D> &distrib);
	// Using a check on userSamplingMapVersion in order to avoid the usage of userSamplingMapMutex
	virtual const bool HasUserSamplingMap() const { return (userSamplingMapVersion > 0); }
	virtual const bool GetUserSamplingMap(u_int &version, boost::shared_array<float> &map,
		boost::shared_ptr<Distribution2D> &distrib);
	// NOTE: returns a copy of the map, it is up to the caller to free the allocated memory !
	virtual float *GetUserSamplingMap();
	virtual void SetUserSamplingMap(const float *map);

	// Return noise-aware map * user sampling map
	virtual const bool GetSamplingMap(u_int &naMapVersion, u_int &usMapVersion,
		boost::shared_array<float> &map, boost::shared_ptr<Distribution2D> &distrib);

	/*
	 * Accessor for samplePerPass
	 * It is only used by SPPM and may disappears once the Buffer API allows for
	 * different numberOfSamples per buffer
	 *
	 * Please, avoid using this function.
	 */
	double GetSamplePerPass() const
	{
		return samplePerPass;
	}

protected:
	bool WriteFilmDataToStream(std::basic_ostream<char> &stream, bool clearBuffers = true, bool transmitParams = false);
	// Reject outliers for a tile. Rejected contributions get their variance set to -1.
	void RejectTileOutliers(const Contribution &contrib, u_int tileIndex, int yTilePixelStart, int yTilePixelEnd);
	// Gets the extents of a tile, interval is [start, end).
	void GetTileExtent(u_int tileIndex, int *xstart, int *xend, int *ystart, int *yend) const;
	void UpdateSamplingMap();
	void UpdateConvergenceInfo(const float *frameBuffer);
	void GenerateNoiseAwareMap();

public:
	// Film Public Data
	u_int GetXResolution();
	u_int GetYResolution();

	u_int GetXPixelStart();
	u_int GetYPixelStart();

	u_int GetXPixelCount();
	u_int GetYPixelCount();

	u_int GetXPixelCount() const { return xPixelCount; }
	u_int GetYPixelCount() const { return yPixelCount; }

	u_int xResolution, yResolution;

	// Statistics
	float EV;
	float averageLuminance;
	double numberOfSamplesFromNetwork;
	double numberOfLocalSamples;
	double numberOfResumedSamples;

	ContributionPool *contribPool;

protected: // Put it here for better data alignment
	// Dade - (xResolution + filter->xWidth) * (yResolution + filter->yWidth)
	double samplePerPass;
	// Film creation time
	boost::xtime creationTime;

	float cropWindow[4];

	Filter *filter;
	float *filterTable;
	FilterLUTs *filterLUTs;

	string filename;

	u_int xPixelStart, yPixelStart, xPixelCount, yPixelCount;
	u_int tileCount, tileHeight;
	float invTileHeight, tileOffset, tileOffset2;
	ColorSystem colorSpace; // needed here for ComputeGroupScale()

	std::vector<BufferConfig> bufferConfigs;
	std::vector<BufferGroup> bufferGroups;

	boost::mutex write_mutex; // WriteImage/ConvergenceTest (i.e. image pipeline) synchronization

	// Enabled by haltthreshold
	luxrays::utils::ConvergenceTest *convTest;

	// May be enabled by the sampler
	VarianceBuffer *varianceBuffer; // Used to build the noise map
	// Using boost::shared_array in order to have a garbage collector-like
	// behavior (i.e. the map is really de-allocated only when all reference are
	// gone)
	boost::shared_array<float> noiseAwareMap;
	u_int noiseAwareMapVersion;
	boost::shared_ptr<Distribution2D> noiseAwareDistribution2D;

	// May be enabled by the user
	string userSamplingMapFileName;
	boost::shared_array<float> userSamplingMap;
	u_int userSamplingMapVersion;
	boost::shared_ptr<Distribution2D> userSamplingDistribution2D;
	
	// Noise-aware map * user sampling map
	boost::shared_array<float> samplingMap;
	boost::shared_ptr<Distribution2D> samplingDistribution2D;
	boost::mutex samplingMapMutex;

	PerPixelNormalizedFloatBuffer *ZBuffer;
	bool use_Zbuf;

	bool debug_mode;
	bool premultiplyAlpha;


	bool writeResumeFlm, restartResumeFlm;
	bool writeFlmDirect;

	// density-based outlier rejection
	int outlierRejection_k;
	u_int outlierCellWidth, outlierCellHeight;
	float outlierInvCellWidth, outlierInvCellHeight;
	typedef BSH<OutlierData::Point_t, NearSetPointProcess<OutlierData::Point_t>, 9 > OutlierAccel;
	std::vector<std::vector<OutlierAccel> > outliers;
	// contains the outliers that lies on the overlap between tiles
	std::vector<std::vector<OutlierAccel> > tileborder_outliers; 

public:
	// Samplers will check this flag to know if we have enough samples per
	// pixel and it is time to stop
	int haltSamplesPerPixel;
	// Seconds to wait before to stop. Any value <= 0 will never stop the rendering
	int haltTime;
	// Convergence threshold to reach before to stop the rendering
	float haltThreshold;
	float haltThresholdComplete;

	Histogram *histogram;
	bool enoughSamplesPerPixel; // At the end to get better data alignment

private:
	// Gets a reference to the appropriate outlier row data for a given position and tile index.
	std::vector<OutlierAccel>& GetOutlierAccelRow(u_int oY, u_int tileIndex, u_int tileStart, u_int tileEnd);
	
	boost::mutex histMutex;
};

// Image Pipeline Declarations
void ApplyImagingPipeline(vector<XYZColor> &pixels, u_int xResolution, u_int yResolution, 
	const GREYCStorationParams &GREYCParams, const ChiuParams &chiuParams,
	const ColorSystem &colorSpace, Histogram *histogram, bool HistogramEnabled,
	bool &haveBloomImage, XYZColor *&bloomImage, bool bloomUpdate,
	float bloomRadius, float bloomWeight,
	bool VignettingEnabled, float VignetScale,
	bool aberrationEnabled, float aberrationAmount,
	bool &haveGlareImage, XYZColor *&glareImage, bool glareUpdate,
	float glareAmount, float glareRadius, u_int glareBlades, float glareThreshold,
	const char *tonemap, const ParamSet *toneMapParams,
	const CameraResponse *response, float dither);

}//namespace lux;

#endif // LUX_FILM_H
