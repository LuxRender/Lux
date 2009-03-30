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

namespace lux {

// FlexImageFilm Declarations
class FlexImageFilm : public Film {
public:
	// FlexImageFilm Public Methods
	// Lotus - not used and not safe (lots of stuff not initialized)
	/*FlexImageFilm(int xres, int yres) :
		Film(xres, yres, 0), filter(NULL), filterTable(NULL),
		framebuffer(NULL), colorSpace(0.63f, 0.34f, 0.31f, 0.595f, 0.155f, 0.07f, 0.314275f, 0.329411f, 1.f) { }*/

	FlexImageFilm(int xres, int yres, Filter *filt, const float crop[4],
		const string &filename1, bool premult, int wI, int dI,
		bool w_tonemapped_EXR, bool w_untonemapped_EXR, bool w_tonemapped_IGI,
		bool w_untonemapped_IGI, bool w_tonemapped_TGA, bool w_resume_FLM, bool restart_resume_FLM, int haltspp,
		int p_TonemapKernel, float p_ReinhardPreScale, float p_ReinhardPostScale,
		float p_ReinhardBurn, float p_LinearSensitivity, float p_LinearExposure, float p_LinearFStop, float p_LinearGamma,
		float p_ContrastDisplayAdaptionY, float p_Gamma,
		const float cs_red[2], const float cs_green[2], const float cs_blue[2], const float whitepoint[2],
		int reject_warmup, bool debugmode);

	~FlexImageFilm() {
		delete[] framebuffer;
	}

	void RequestBufferGroups(const vector<string> &bg);
	int RequestBuffer(BufferType type, BufferOutputConfig output, const string& filePostfix);
	void CreateBuffers();
	u_int GetNumBufferConfigs() const { return bufferConfigs.size(); }
	const BufferConfig& GetBufferConfig( u_int index ) const { return bufferConfigs[index]; }
	u_int GetNumBufferGroups() const { return bufferGroups.size(); }
	string GetGroupName(u_int index) const;
	void SetGroupEnable(u_int index, bool status);
	bool GetGroupEnable(u_int index) const;
	void SetGroupScale(u_int index, float value);
	float GetGroupScale(u_int index) const;
	void SetGroupRGBScale(u_int index, const RGBColor &value);
	RGBColor GetGroupRGBScale(u_int index) const;
	void SetGroupTemperature(u_int index, float value);
	float GetGroupTemperature(u_int index) const;
	void ComputeGroupScale(u_int index);

	int GetXPixelCount() const { return xPixelCount; }
	int GetYPixelCount() const { return yPixelCount; }

	void GetSampleExtent(int *xstart, int *xend, int *ystart, int *yend) const;
	void AddSample(Contribution *contrib);
	void AddSampleCount(float count);

	void WriteImage(ImageType type);

	// GUI display methods
	void updateFrameBuffer();
	unsigned char* getFrameBuffer();
	void createFrameBuffer();
	float getldrDisplayInterval() {
		return displayInterval;
	}

	// Parameter Access functions
	void SetParameterValue(luxComponentParameters param, double value, int index);
	double GetParameterValue(luxComponentParameters param, int index);
	double GetDefaultParameterValue(luxComponentParameters param, int index);
	string GetStringParameterValue(luxComponentParameters param, int index);

	void WriteFilm(const string &filename) { WriteResumeFilm(filename); }
	// Dade - method useful for transmitting the samples to a client
	void TransmitFilm(std::basic_ostream<char> &stream,bool clearBuffers = true,bool transmitParams=false);
	float UpdateFilm(Scene *scene, std::basic_istream<char> &stream);

	static Film *CreateFilm(const ParamSet &params, Filter *filter);
	/**
	 * Constructs an image film that loads its data from the give FLM file. This film is already initialized with
	 * the necessary buffers. This is currently only used for loading and tonemapping an existing FLM file.
	 */
	static FlexImageFilm *CreateFilmFromFLM(const string &flmFileName);

private:
	static void GetColorspaceParam(const ParamSet &params, const string name, float values[2]);

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
	//ParamSet toneParams;
	//float gamma;
	double reject_warmup_samples;
	bool writeTmExr, writeUtmExr, writeTmIgi, writeUtmIgi, writeTmTga, writeResumeFlm, restartResumeFlm;

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

	Color * m_bloomImage; // Persisting bloom layer image 
	bool m_BloomUpdateLayer;
	bool m_BloomDeleteLayer;
	bool m_HaveBloomImage;
	float m_BloomRadius, d_BloomRadius;
	float m_BloomWeight, d_BloomWeight;

	bool m_VignettingEnabled, d_VignettingEnabled;
	float m_VignettingScale, d_VignettingScale;

	bool m_AberrationEnabled, d_AberrationEnabled;
	float m_AberrationAmount, d_AberrationAmount;

	Color * m_glareImage; // Persisting glarelayer image 
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

