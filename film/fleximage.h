/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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
#include <boost/thread/mutex.hpp>

namespace lux {

// FlexImageFilm Declarations
class LUX_EXPORT FlexImageFilm : public Film {
public:
	enum OutputChannels { Y, YA, RGB, RGBA };
	enum ZBufNormalization { None, CameraStartEnd, MinMax };
	enum TonemapKernels { TMK_Reinhard = 0, TMK_Linear, TMK_Contrast, TMK_MaxWhite, TMK_AutoLinear, TMK_Colors };

	// FlexImageFilm Public Methods

	FlexImageFilm(u_int xres, u_int yres, Filter *filt, u_int filtRes, const float crop[4],
		const string &filename1, bool premult, int wI, int fwI, int dI, int cM,
		bool cw_EXR, OutputChannels cw_EXR_channels, bool cw_EXR_halftype, int cw_EXR_compressiontype, bool cw_EXR_applyimaging,
		bool cw_EXR_gamutclamp, bool cw_EXR_ZBuf, ZBufNormalization cw_EXR_ZBuf_normalizationtype, bool cw_EXR_straight_colors,
		bool cw_PNG, OutputChannels cw_PNG_channels, bool cw_PNG_16bit, bool cw_PNG_gamutclamp, bool cw_PNG_ZBuf, ZBufNormalization cw_PNG_ZBuf_normalizationtype,
		bool cw_TGA, OutputChannels cw_TGA_channels, bool cw_TGA_gamutclamp, bool cw_TGA_ZBuf, ZBufNormalization cw_TGA_ZBuf_normalizationtype, 
		bool w_resume_FLM, bool restart_resume_FLM, bool write_FLM_direct, int haltspp, int halttime, float haltthreshold,
		int p_TonemapKernel, float p_ReinhardPreScale, float p_ReinhardPostScale,
		float p_ReinhardBurn, float p_LinearSensitivity, float p_LinearExposure, float p_LinearFStop, float p_LinearGamma,
		float p_ContrastDisplayAdaptionY, int p_FalseMethod, int p_FalseColorScale, float p_FalseMaxSat, float p_FalseMinSat, const string &response, float p_Gamma,
		const float cs_red[2], const float cs_green[2], const float cs_blue[2], const float whitepoint[2],
		bool debugmode, int outlierk, int tilecount, const double convstep, const string &samplingmapfilename, const bool disableNoiseMapUpd,
		const string &pupilmap, const string &lashesmap);

	virtual ~FlexImageFilm() {
		if (convUpdateThread) {
			convUpdateThread->interrupt();
			convUpdateThread->join();
		}

		delete[] framebuffer;
		delete[] float_framebuffer;
		delete[] alpha_buffer;
		delete[] z_buffer;
		delete convUpdateThread;
	}	

	virtual void CreateBuffers();
	virtual bool SaveEXR(const string &exrFilename, bool useHalfFloats, bool includeZBuf, int compressionType, bool tonemapped);
	virtual bool WriteImage(ImageType type);
	virtual void CheckWriteOuputInterval();

	// GUI display methods
	virtual void updateFrameBuffer();
	virtual unsigned char* getFrameBuffer();
	virtual float* getFloatFrameBuffer();
	virtual float* getAlphaBuffer();
	virtual float* getZBuffer();
	virtual void createFrameBuffer();
	virtual int getldrDisplayInterval() { return displayInterval; }

	// Parameter Access functions
	virtual void SetParameterValue(luxComponentParameters param, double value, u_int index);
	virtual double GetParameterValue(luxComponentParameters param, u_int index);
	virtual double GetDefaultParameterValue(luxComponentParameters param, u_int index);
	virtual void SetStringParameterValue(luxComponentParameters param, const string& value, u_int index);
	virtual string GetStringParameterValue(luxComponentParameters param, u_int index);

	static Film *CreateFilm(const ParamSet &params, Filter *filter);
	/**
	 * Constructs an image film that loads its data from the give FLM file. This film is already initialized with
	 * the necessary buffers. This is currently only used for loading and tonemapping an existing FLM file.
	 */
	static Film *CreateFilmFromFLM(const string &flmFileName);

private:
	static void GetColorspaceParam(const ParamSet &params, const string name, float values[2]);
	static void ConvUpdateThreadImpl(FlexImageFilm *film, Context *ctx);

	vector<RGBColor>& ApplyPipeline(const ColorSystem &colorSpace, vector<XYZColor> &color);
	bool WriteImage2(ImageType type, vector<XYZColor> &color, vector<float> &alpha, string postfix);
	bool WriteTGAImage(vector<RGBColor> &rgb, vector<float> &alpha, const string &filename);
	bool WritePNGImage(vector<RGBColor> &rgb, vector<float> &alpha, const string &filename);
	bool WriteEXRImage(vector<RGBColor> &rgb, vector<float> &alpha, const string &filename, vector<float> &zbuf);

	// FlexImageFilm Private Data
	// mutex is used for protecting the framebuffer pointer
	// not reading/writing to the framebuffer
	boost::mutex framebufferMutex;
	unsigned char *framebuffer;
	float *float_framebuffer;
	float *alpha_buffer;
	float *z_buffer;

	float m_RGB_X_White, d_RGB_X_White;
	float m_RGB_Y_White, d_RGB_Y_White;
	float m_RGB_X_Red, d_RGB_X_Red;
	float m_RGB_Y_Red, d_RGB_Y_Red;
	float m_RGB_X_Green, d_RGB_X_Green;
	float m_RGB_Y_Green, d_RGB_Y_Green;
	float m_RGB_X_Blue, d_RGB_X_Blue;
	float m_RGB_Y_Blue, d_RGB_Y_Blue;
	float m_Gamma, d_Gamma;
	int clampMethod, d_clampMethod;

	int m_TonemapKernel, d_TonemapKernel;
	float m_ReinhardPreScale, d_ReinhardPreScale;
	float m_ReinhardPostScale, d_ReinhardPostScale;
	float m_ReinhardBurn, d_ReinhardBurn;
	float m_LinearSensitivity, d_LinearSensitivity;
	float m_LinearExposure, d_LinearExposure;
	float m_LinearFStop, d_LinearFStop;
	float m_LinearGamma, d_LinearGamma;
	float m_ContrastYwa, d_ContrastYwa;
	int m_FalseMethod, d_FalseMethod;
	int m_FalseColorScale, d_FalseColorScale;
	float m_FalseMax, m_FalseMin;
	float m_FalseMaxSat, d_FalseMaxSat;
	float m_FalseMinSat, d_FalseMinSat;
	float m_FalseAvgLum, m_FalseAvgEmi;

	int writeInterval;
	boost::xtime lastWriteImageTime;
	int flmWriteInterval;
	boost::xtime lastWriteFLMTime;
	int displayInterval;
	int write_EXR_compressiontype;
	ZBufNormalization write_EXR_ZBuf_normalizationtype;
	//OutputChannels write_EXR_channels;
	int write_EXR_channels;
	bool write_EXR_straight_colors;
	ZBufNormalization write_PNG_ZBuf_normalizationtype;
	OutputChannels write_PNG_channels;
	ZBufNormalization write_TGA_ZBuf_normalizationtype;
	OutputChannels write_TGA_channels;

	bool write_EXR, write_EXR_halftype, write_EXR_applyimaging, write_EXR_gamutclamp, write_EXR_ZBuf;
	bool write_PNG, write_PNG_16bit, write_PNG_gamutclamp, write_PNG_ZBuf;
	bool write_TGA,write_TGA_gamutclamp, write_TGA_ZBuf;

	GREYCStorationParams m_GREYCStorationParams, d_GREYCStorationParams;
	ChiuParams m_chiuParams, d_chiuParams;

	bool m_CameraResponseEnabled, d_CameraResponseEnabled;
	string m_CameraResponseFile, d_CameraResponseFile; // Path to the data file
	boost::shared_ptr<CameraResponse> cameraResponse; // Actual data processor

	XYZColor * m_bloomImage; // Persisting bloom layer image 
	float m_BloomRadius, d_BloomRadius;
	float m_BloomWeight, d_BloomWeight;
	bool m_BloomUpdateLayer;
	bool m_BloomDeleteLayer;
	bool m_HaveBloomImage;

	float m_VignettingScale, d_VignettingScale;
	bool m_VignettingEnabled, d_VignettingEnabled;

	float m_AberrationAmount, d_AberrationAmount;
	bool m_AberrationEnabled, d_AberrationEnabled;

	XYZColor * m_glareImage; // Persisting glarelayer image 
	float m_GlareAmount, d_GlareAmount;
	float m_GlareRadius, d_GlareRadius;
	int m_GlareBlades, d_GlareBlades;
	float m_GlareThreshold, d_GlareThreshold;
	bool m_GlareMap, d_GlareMap;
	string m_GlareLashesFilename, m_GlarePupilFilename;
	bool m_GlareUpdateLayer;
	bool m_GlareDeleteLayer;
	bool m_HaveGlareImage;

	bool m_HistogramEnabled, d_HistogramEnabled;
	
	// Thread dedicated to convergence test an noise-aware map update
	boost::thread *convUpdateThread;
	double convUpdateStep; // Number of new samples per pixel required to trigger an update
	bool disableNoiseMapUpdate; // This flag is used by network slaves and it is not intended to be used directly in .lxs files
};

}//namespace lux

#endif //LUX_FLEXIMAGE_H

