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
#include "filter.h"
#include <boost/thread/xtime.hpp>

namespace lux {

// FlexImageFilm Declarations
class FlexImageFilm : public Film {
public:
	enum OutputChannels { Y, YA, RGB, RGBA };
	enum ZBufNormalization { None, CameraStartEnd, MinMax };

	// FlexImageFilm Public Methods

	FlexImageFilm(int xres, int yres, Filter *filt, const float crop[4],
		const string &filename1, bool premult, int wI, int dI, int cM,
		bool cw_EXR, OutputChannels cw_EXR_channels, bool cw_EXR_halftype, int cw_EXR_compressiontype, bool cw_EXR_applyimaging,
		bool cw_EXR_gamutclamp, bool cw_EXR_ZBuf, ZBufNormalization cw_EXR_ZBuf_normalizationtype,
		bool cw_PNG, OutputChannels cw_PNG_channels, bool cw_PNG_16bit, bool cw_PNG_gamutclamp, bool cw_PNG_ZBuf, ZBufNormalization cw_PNG_ZBuf_normalizationtype,
		bool cw_TGA, OutputChannels cw_TGA_channels, bool cw_TGA_gamutclamp, bool cw_TGA_ZBuf, ZBufNormalization cw_TGA_ZBuf_normalizationtype, 
		bool w_resume_FLM, bool restart_resume_FLM, int haltspp,
		int p_TonemapKernel, float p_ReinhardPreScale, float p_ReinhardPostScale,
		float p_ReinhardBurn, float p_LinearSensitivity, float p_LinearExposure, float p_LinearFStop, float p_LinearGamma,
		float p_ContrastDisplayAdaptionY, float p_Gamma,
		const float cs_red[2], const float cs_green[2], const float cs_blue[2], const float whitepoint[2],
		int reject_warmup, bool debugmode);

	virtual ~FlexImageFilm() {
		delete[] framebuffer;
		if(use_Zbuf && ZBuffer)
			delete ZBuffer;
		delete[] filterTable;
		delete filter;
	}

	virtual void RequestBufferGroups(const vector<string> &bg);
	virtual int RequestBuffer(BufferType type, BufferOutputConfig output, const string& filePostfix);
	virtual void CreateBuffers();
	virtual u_int GetNumBufferConfigs() const { return bufferConfigs.size(); }
	virtual const BufferConfig& GetBufferConfig( u_int index ) const { return bufferConfigs[index]; }
	virtual u_int GetNumBufferGroups() const { return bufferGroups.size(); }
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

	virtual int GetXPixelCount() const { return xPixelCount; }
	virtual int GetYPixelCount() const { return yPixelCount; }

	virtual void GetSampleExtent(int *xstart, int *xend, int *ystart, int *yend) const;
	virtual void AddSample(Contribution *contrib);
	virtual void AddSampleCount(float count);

	virtual void WriteImage(ImageType type);

	// GUI display methods
	virtual void updateFrameBuffer();
	virtual unsigned char* getFrameBuffer();
	virtual void createFrameBuffer();
	virtual float getldrDisplayInterval() {
		return displayInterval;
	}

	// Parameter Access functions
	virtual void SetParameterValue(luxComponentParameters param, double value, int index);
	virtual double GetParameterValue(luxComponentParameters param, int index);
	virtual double GetDefaultParameterValue(luxComponentParameters param, int index);
	virtual void SetStringParameterValue(luxComponentParameters param, const string& value, int index);
	virtual string GetStringParameterValue(luxComponentParameters param, int index);

	virtual void WriteFilm(const string &fname) { WriteResumeFilm(fname); }
	// Dade - method useful for transmitting the samples to a client
	virtual void TransmitFilm(std::basic_ostream<char> &stream,bool clearBuffers = true,bool transmitParams=false);
	virtual float UpdateFilm(std::basic_istream<char> &stream);

	static Film *CreateFilm(const ParamSet &params, Filter *filter);
	/**
	 * Constructs an image film that loads its data from the give FLM file. This film is already initialized with
	 * the necessary buffers. This is currently only used for loading and tonemapping an existing FLM file.
	 */
	static Film *CreateFilmFromFLM(const string &flmFileName);

private:
	static void GetColorspaceParam(const ParamSet &params, const string name, float values[2]);

	void WriteImage2(ImageType type, vector<XYZColor> &color, vector<float> &alpha, string postfix);
	void WriteTGAImage(vector<RGBColor> &rgb, vector<float> &alpha, const string &filename);
	void WritePNGImage(vector<RGBColor> &rgb, vector<float> &alpha, const string &filename);
	void WriteEXRImage(vector<RGBColor> &rgb, vector<float> &alpha, const string &filename, vector<float> &zbuf);
	void WriteResumeFilm(const string &filename);

	// FlexImageFilm Private Data
	Filter *filter;
	int writeInterval;
	int displayInterval;
	int clampMethod;
	string filename;
	bool premultiplyAlpha, buffersInited;
	float cropWindow[4], *filterTable;
	int xPixelStart, yPixelStart, xPixelCount, yPixelCount;
	//ParamSet toneParams;
	//float gamma;
	double reject_warmup_samples;

	//bool writeTmExr, writeUtmExr, writeTmIgi, writeUtmIgi, writeTmTga;
	bool write_EXR, write_EXR_halftype, write_EXR_applyimaging, write_EXR_gamutclamp, write_EXR_ZBuf;
	bool write_PNG, write_PNG_16bit, write_PNG_gamutclamp, write_PNG_ZBuf;
	bool write_TGA,write_TGA_gamutclamp, write_TGA_ZBuf;
	int write_EXR_compressiontype;
	ZBufNormalization write_EXR_ZBuf_normalizationtype;
	OutputChannels write_EXR_channels;
	ZBufNormalization write_PNG_ZBuf_normalizationtype;
	OutputChannels write_PNG_channels;
	ZBufNormalization write_TGA_ZBuf_normalizationtype;
	OutputChannels write_TGA_channels;
	
	bool use_Zbuf;
	PerPixelNormalizedFloatBuffer *ZBuffer;

	bool writeResumeFlm, restartResumeFlm;

	unsigned char *framebuffer;

	boost::xtime lastWriteImageTime;

	bool debug_mode;

	std::vector<BufferConfig> bufferConfigs;
	std::vector<BufferGroup> bufferGroups;

	float maxY;
	double warmupSamples;
	bool warmupComplete;
	ColorSystem colorSpace;

	int m_TonemapKernel, d_TonemapKernel;
	float m_ReinhardPreScale, d_ReinhardPreScale;
	float m_ReinhardPostScale, d_ReinhardPostScale;
	float m_ReinhardBurn, d_ReinhardBurn;
	float m_LinearSensitivity, d_LinearSensitivity;
	float m_LinearExposure, d_LinearExposure;
	float m_LinearFStop, d_LinearFStop;
	float m_LinearGamma, d_LinearGamma;
	float m_ContrastYwa, d_ContrastYwa;
	float m_RGB_X_White, d_RGB_X_White;
	float m_RGB_Y_White, d_RGB_Y_White;
	float m_RGB_X_Red, d_RGB_X_Red;
	float m_RGB_Y_Red, d_RGB_Y_Red;
	float m_RGB_X_Green, d_RGB_X_Green;
	float m_RGB_Y_Green, d_RGB_Y_Green;
	float m_RGB_X_Blue, d_RGB_X_Blue;
	float m_RGB_Y_Blue, d_RGB_Y_Blue;
	float m_Gamma, d_Gamma;

	GREYCStorationParams m_GREYCStorationParams, d_GREYCStorationParams;
	ChiuParams m_chiuParams, d_chiuParams;

	XYZColor * m_bloomImage; // Persisting bloom layer image 
	bool m_BloomUpdateLayer;
	bool m_BloomDeleteLayer;
	bool m_HaveBloomImage;
	float m_BloomRadius, d_BloomRadius;
	float m_BloomWeight, d_BloomWeight;

	bool m_VignettingEnabled, d_VignettingEnabled;
	float m_VignettingScale, d_VignettingScale;

	bool m_AberrationEnabled, d_AberrationEnabled;
	float m_AberrationAmount, d_AberrationAmount;

	XYZColor * m_glareImage; // Persisting glarelayer image 
	bool m_GlareUpdateLayer;
	bool m_GlareDeleteLayer;
	bool m_HaveGlareImage;
	float m_GlareAmount, d_GlareAmount;
	float m_GlareRadius, d_GlareRadius;
	int m_GlareBlades, d_GlareBlades;

	bool m_HistogramEnabled, d_HistogramEnabled;
};

}//namespace lux

#endif //LUX_FLEXIMAGE_H

