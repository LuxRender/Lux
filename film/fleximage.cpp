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

/*
 * FlexImage Film class
 *
 */

// Those includes must come first (before lux.h)
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#include "fleximage.h"
#include "error.h"
#include "scene.h"		// for Scene
#include "filter.h"
#include "exrio.h"
#include "tgaio.h"
#include "pngio.h"
#include "blackbodyspd.h"
#include "osfunc.h"
#include "dynload.h"

#include <iostream>
#include <fstream>

#include <boost/thread/xtime.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace boost::iostreams;
using namespace lux;

// FlexImageFilm Method Definitions
FlexImageFilm::FlexImageFilm(int xres, int yres, Filter *filt, const float crop[4],
	const string &filename1, bool premult, int wI, int dI, int cM,
	bool cw_EXR, OutputChannels cw_EXR_channels, bool cw_EXR_halftype, int cw_EXR_compressiontype, bool cw_EXR_applyimaging,
	bool cw_EXR_gamutclamp, bool cw_EXR_ZBuf, ZBufNormalization cw_EXR_ZBuf_normalizationtype,
	bool cw_PNG, OutputChannels cw_PNG_channels, bool cw_PNG_16bit, bool cw_PNG_gamutclamp, bool cw_PNG_ZBuf, ZBufNormalization cw_PNG_ZBuf_normalizationtype,
	bool cw_TGA, OutputChannels cw_TGA_channels, bool cw_TGA_gamutclamp, bool cw_TGA_ZBuf, ZBufNormalization cw_TGA_ZBuf_normalizationtype, 
	bool w_resume_FLM, bool restart_resume_FLM, int haltspp,
	int p_TonemapKernel, float p_ReinhardPreScale, float p_ReinhardPostScale,
	float p_ReinhardBurn, float p_LinearSensitivity, float p_LinearExposure, float p_LinearFStop, float p_LinearGamma,
	float p_ContrastYwa, float p_Gamma,
	const float cs_red[2], const float cs_green[2], const float cs_blue[2], const float whitepoint[2],
	int reject_warmup, bool debugmode) :
	Film(xres, yres, haltspp), filter(filt), writeInterval(wI), displayInterval(dI),
	filename(filename1), premultiplyAlpha(premult), buffersInited(false),
	writeResumeFlm(w_resume_FLM), restartResumeFlm(restart_resume_FLM),
	framebuffer(NULL), debug_mode(debugmode),
	colorSpace(cs_red[0], cs_red[1], cs_green[0], cs_green[1], cs_blue[0], cs_blue[1], whitepoint[0], whitepoint[1], 1.f)
{
	// Compute film image extent
	memcpy(cropWindow, crop, 4 * sizeof(float));
	xPixelStart = Ceil2Int(xResolution * cropWindow[0]);
	xPixelCount = max(1, Ceil2Int(xResolution * cropWindow[1]) - xPixelStart);
	yPixelStart = Ceil2Int(yResolution * cropWindow[2]);
	yPixelCount = max(1, Ceil2Int(yResolution * cropWindow[3]) - yPixelStart);
	int xRealWidth = Floor2Int(xPixelStart + .5f + xPixelCount + filter->xWidth) - Floor2Int(xPixelStart + .5f - filter->xWidth);
	int yRealHeight = Floor2Int(yPixelStart + .5f + yPixelCount + filter->yWidth) - Floor2Int(yPixelStart + .5f - filter->yWidth);
	samplePerPass = xRealWidth * yRealHeight;

	// Set Image Output parameters
	clampMethod = cM;
	write_EXR = cw_EXR;
	write_EXR_halftype = cw_EXR_halftype;
	write_EXR_applyimaging = cw_EXR_applyimaging;
	write_EXR_gamutclamp = cw_EXR_gamutclamp;
	write_EXR_ZBuf = cw_EXR_ZBuf;
	write_PNG = cw_PNG;
	write_PNG_16bit = cw_PNG_16bit;
	write_PNG_gamutclamp = cw_PNG_gamutclamp;
	write_PNG_ZBuf = cw_PNG_ZBuf;
	write_TGA = cw_TGA;
	write_TGA_gamutclamp = cw_TGA_gamutclamp;
	write_TGA_ZBuf = cw_TGA_ZBuf;
	write_EXR_channels = cw_EXR_channels;
	write_EXR_compressiontype = cw_EXR_compressiontype;
	write_EXR_ZBuf_normalizationtype = cw_EXR_ZBuf_normalizationtype;
	write_PNG_ZBuf_normalizationtype = cw_PNG_ZBuf_normalizationtype;
	write_PNG_channels = cw_PNG_channels;
	write_TGA_channels = cw_TGA_channels;
	write_TGA_ZBuf_normalizationtype = cw_TGA_ZBuf_normalizationtype;

	// Determing if we need a Zbuf buffer
	if(write_EXR_ZBuf || write_PNG_ZBuf || write_TGA_ZBuf)
		use_Zbuf = true;
	else
		use_Zbuf = false;

	// Set use and default runtime changeable parameters
	m_TonemapKernel = d_TonemapKernel = p_TonemapKernel;

	m_ReinhardPreScale = d_ReinhardPreScale = p_ReinhardPreScale;
	m_ReinhardPostScale = d_ReinhardPostScale = p_ReinhardPostScale;
	m_ReinhardBurn = d_ReinhardBurn = p_ReinhardBurn;

	m_LinearSensitivity = d_LinearSensitivity = p_LinearSensitivity;
	m_LinearExposure = d_LinearExposure = p_LinearExposure;
	m_LinearFStop = d_LinearFStop = p_LinearFStop;
	m_LinearGamma = d_LinearGamma = p_LinearGamma;

	m_ContrastYwa = d_ContrastYwa = p_ContrastYwa;

	m_RGB_X_White = d_RGB_X_White = whitepoint[0];
	m_RGB_Y_White = d_RGB_Y_White = whitepoint[1];
	m_RGB_X_Red = d_RGB_X_Red = cs_red[0];
	m_RGB_Y_Red = d_RGB_Y_Red = cs_red[1];
	m_RGB_X_Green = d_RGB_X_Green = cs_green[0];
	m_RGB_Y_Green = d_RGB_Y_Green = cs_green[1];
	m_RGB_X_Blue = d_RGB_X_Blue = cs_blue[0];
	m_RGB_Y_Blue = d_RGB_Y_Blue = cs_blue[1];
	m_Gamma = d_Gamma = p_Gamma;

	m_BloomUpdateLayer = false;
	m_BloomDeleteLayer = false;
	m_HaveBloomImage = false;
	m_BloomRadius = d_BloomRadius = 0.07f;
	m_BloomWeight = d_BloomWeight = 0.25f;

	m_VignettingEnabled = d_VignettingEnabled = false;
	m_VignettingScale = d_VignettingScale = 0.4f;

	m_AberrationEnabled = d_AberrationEnabled = false;
	m_AberrationAmount = d_AberrationAmount = 0.005f;

	m_GlareUpdateLayer = false;
	m_GlareDeleteLayer = false;
	m_HaveGlareImage = false;
	m_glareImage = NULL;
	m_bloomImage = NULL;
	m_GlareAmount = d_GlareAmount = 0.03f;
	m_GlareRadius = d_GlareRadius = 0.03f;
	m_GlareBlades = d_GlareBlades = 3;

	m_HistogramEnabled = d_HistogramEnabled = false;

	m_GREYCStorationParams.Reset();
	d_GREYCStorationParams.Reset();

	m_chiuParams.Reset();
	d_chiuParams.Reset();

	// init timer
	boost::xtime_get(&lastWriteImageTime, boost::TIME_UTC);

	// calculate reject warmup samples
	reject_warmup_samples = ((double)xRealWidth * (double)yRealHeight) * reject_warmup;

	// Precompute filter weight table
	#define FILTER_TABLE_SIZE 16
	filterTable = new float[FILTER_TABLE_SIZE * FILTER_TABLE_SIZE];
	float *ftp = filterTable;
	for (int y = 0; y < FILTER_TABLE_SIZE; ++y) {
		float fy = ((float)y + .5f) * filter->yWidth / FILTER_TABLE_SIZE;
		for (int x = 0; x < FILTER_TABLE_SIZE; ++x) {
			float fx = ((float)x + .5f) * filter->xWidth / FILTER_TABLE_SIZE;
			*ftp++ = filter->Evaluate(fx, fy);
		}
	}

	maxY = debug_mode ? INFINITY : 0.f;
	warmupSamples = 0;
	warmupComplete = debug_mode;
}

// Parameter Access functions
void FlexImageFilm::SetParameterValue(luxComponentParameters param, double value, int index)
{
	 switch (param) {
		case LUX_FILM_TM_TONEMAPKERNEL:
			m_TonemapKernel = Floor2Int(value);
			break;

		case LUX_FILM_TM_REINHARD_PRESCALE:
			m_ReinhardPreScale = value;
			break;
		case LUX_FILM_TM_REINHARD_POSTSCALE:
			m_ReinhardPostScale = value;
			break;
		case LUX_FILM_TM_REINHARD_BURN:
			m_ReinhardBurn = value;
			break;

		case LUX_FILM_TM_LINEAR_SENSITIVITY:
			m_LinearSensitivity = value;
			break;
		case LUX_FILM_TM_LINEAR_EXPOSURE:
			m_LinearExposure = value;
			break;
		case LUX_FILM_TM_LINEAR_FSTOP:
			m_LinearFStop = value;
			break;
		case LUX_FILM_TM_LINEAR_GAMMA:
			m_LinearGamma = value;
			break;

		case LUX_FILM_TM_CONTRAST_YWA:
			m_ContrastYwa = value;
			break;

		case LUX_FILM_TORGB_X_WHITE:
			m_RGB_X_White = value;
			break;
		case LUX_FILM_TORGB_Y_WHITE:
			m_RGB_Y_White = value;
			break;
		case LUX_FILM_TORGB_X_RED:
			m_RGB_X_Red = value;
			break;
		case LUX_FILM_TORGB_Y_RED:
			m_RGB_Y_Red = value;
			break;
		case LUX_FILM_TORGB_X_GREEN:
			m_RGB_X_Green = value;
			break;
		case LUX_FILM_TORGB_Y_GREEN:
			m_RGB_Y_Green = value;
			break;
		case LUX_FILM_TORGB_X_BLUE:
			m_RGB_X_Blue = value;
			break;
		case LUX_FILM_TORGB_Y_BLUE:
			m_RGB_Y_Blue = value;
			break;
		case LUX_FILM_TORGB_GAMMA:
			m_Gamma = value;
			break;
		case LUX_FILM_UPDATEBLOOMLAYER:
			m_BloomUpdateLayer = (value != 0.f);
			break;
		case LUX_FILM_DELETEBLOOMLAYER:
			m_BloomDeleteLayer = (value != 0.f);
			break;

		case LUX_FILM_BLOOMRADIUS:
			 m_BloomRadius = value;
			break;
		case LUX_FILM_BLOOMWEIGHT:
			 m_BloomWeight = value;
			break;

		case LUX_FILM_VIGNETTING_ENABLED:
			m_VignettingEnabled = (value != 0.f);
			break;
		case LUX_FILM_VIGNETTING_SCALE:
			 m_VignettingScale = value;
			break;

		case LUX_FILM_ABERRATION_ENABLED:
			m_AberrationEnabled = (value != 0.f);
			break;
		case LUX_FILM_ABERRATION_AMOUNT:
			 m_AberrationAmount = value;
			break;

		case LUX_FILM_UPDATEGLARELAYER:
			m_GlareUpdateLayer = (value != 0.f);
			break;
		case LUX_FILM_DELETEGLARELAYER:
			m_GlareDeleteLayer = (value != 0.f);
			break;
		case LUX_FILM_GLARE_AMOUNT:
			 m_GlareAmount = value;
			break;
		case LUX_FILM_GLARE_RADIUS:
			 m_GlareRadius = value;
			break;
		case LUX_FILM_GLARE_BLADES:
			 m_GlareBlades = (int)value;
			break;

		case LUX_FILM_HISTOGRAM_ENABLED:
			m_HistogramEnabled = (value != 0.f);
			break;

		case LUX_FILM_NOISE_CHIU_ENABLED:
			m_chiuParams.enabled = (value != 0.f);
			break;
		case LUX_FILM_NOISE_CHIU_RADIUS:
			m_chiuParams.radius = value;
			break;
		case LUX_FILM_NOISE_CHIU_INCLUDECENTER:
			m_chiuParams.includecenter = (value != 0.f);
			break;

		case LUX_FILM_NOISE_GREYC_ENABLED:
			m_GREYCStorationParams.enabled = (value != 0.f);
			break;
		case LUX_FILM_NOISE_GREYC_AMPLITUDE:
			m_GREYCStorationParams.amplitude = value;
			break;
		case LUX_FILM_NOISE_GREYC_NBITER:
			m_GREYCStorationParams.nb_iter = int(value);
			break;
		case LUX_FILM_NOISE_GREYC_SHARPNESS:
			m_GREYCStorationParams.sharpness = value;
			break;
		case LUX_FILM_NOISE_GREYC_ANISOTROPY:
			m_GREYCStorationParams.anisotropy = value;
			break;
		case LUX_FILM_NOISE_GREYC_ALPHA:
			m_GREYCStorationParams.alpha = value;
			break;
		case LUX_FILM_NOISE_GREYC_SIGMA:
			m_GREYCStorationParams.sigma = value;
			break;
		case LUX_FILM_NOISE_GREYC_FASTAPPROX:
			m_GREYCStorationParams.fast_approx = (value != 0.f);
			break;
		case LUX_FILM_NOISE_GREYC_GAUSSPREC:
			m_GREYCStorationParams.gauss_prec = value;
			break;
		case LUX_FILM_NOISE_GREYC_DL:
			m_GREYCStorationParams.dl = value;
			break;
		case LUX_FILM_NOISE_GREYC_DA:
			m_GREYCStorationParams.da = value;
			break;
		case LUX_FILM_NOISE_GREYC_INTERP:
			m_GREYCStorationParams.interp = (int) value;
			break;
		case LUX_FILM_NOISE_GREYC_TILE:
			m_GREYCStorationParams.tile = (int) value;
			break;
		case LUX_FILM_NOISE_GREYC_BTILE:
			m_GREYCStorationParams.btile = (int) value;
			break;
		case LUX_FILM_NOISE_GREYC_THREADS:
			m_GREYCStorationParams.threads = (int) value;
			break;

		case LUX_FILM_LG_SCALE:
			SetGroupScale(index, value);
			break;
		case LUX_FILM_LG_ENABLE:
			SetGroupEnable(index, value != 0.f);
			break;
		case LUX_FILM_LG_SCALE_RED: {
			RGBColor color(GetGroupRGBScale(index));
			color.c[0] = value;
			SetGroupRGBScale(index, color);
			break;
		}
		case LUX_FILM_LG_SCALE_GREEN: {
			RGBColor color(GetGroupRGBScale(index));
			color.c[1] = value;
			SetGroupRGBScale(index, color);
			break;
		}
		case LUX_FILM_LG_SCALE_BLUE: {
			RGBColor color(GetGroupRGBScale(index));
			color.c[2] = value;
			SetGroupRGBScale(index, color);
			break;
		}
		case LUX_FILM_LG_TEMPERATURE: {
			SetGroupTemperature(index, value);
			break;
		}

		 default:
			break;
	 }
}
double FlexImageFilm::GetParameterValue(luxComponentParameters param, int index)
{
	 switch (param) {
		case LUX_FILM_TM_TONEMAPKERNEL:
			return m_TonemapKernel;
			break;

		case LUX_FILM_TM_REINHARD_PRESCALE:
			return m_ReinhardPreScale;
			break;
		case LUX_FILM_TM_REINHARD_POSTSCALE:
			return m_ReinhardPostScale;
			break;
		case LUX_FILM_TM_REINHARD_BURN:
			return m_ReinhardBurn;
			break;

		case LUX_FILM_TM_LINEAR_SENSITIVITY:
			return m_LinearSensitivity;
			break;
		case LUX_FILM_TM_LINEAR_EXPOSURE:
			return m_LinearExposure;
			break;
		case LUX_FILM_TM_LINEAR_FSTOP:
			return m_LinearFStop;
			break;
		case LUX_FILM_TM_LINEAR_GAMMA:
			return m_LinearGamma;
			break;

		case LUX_FILM_TM_CONTRAST_YWA:
			return m_ContrastYwa;
			break;

		case LUX_FILM_TORGB_X_WHITE:
			return m_RGB_X_White;
			break;
		case LUX_FILM_TORGB_Y_WHITE:
			return m_RGB_Y_White;
			break;
		case LUX_FILM_TORGB_X_RED:
			return m_RGB_X_Red;
			break;
		case LUX_FILM_TORGB_Y_RED:
			return m_RGB_Y_Red;
			break;
		case LUX_FILM_TORGB_X_GREEN:
			return m_RGB_X_Green;
			break;
		case LUX_FILM_TORGB_Y_GREEN:
			return m_RGB_Y_Green;
			break;
		case LUX_FILM_TORGB_X_BLUE:
			return m_RGB_X_Blue;
			break;
		case LUX_FILM_TORGB_Y_BLUE:
			return m_RGB_Y_Blue;
			break;
		case LUX_FILM_TORGB_GAMMA:
			return m_Gamma;
			break;

		case LUX_FILM_BLOOMRADIUS:
			return m_BloomRadius;
			break;
		case LUX_FILM_BLOOMWEIGHT:
			return m_BloomWeight;
			break;

		case LUX_FILM_VIGNETTING_ENABLED:
			return m_VignettingEnabled;
			break;
		case LUX_FILM_VIGNETTING_SCALE:
			return m_VignettingScale;
			break;

		case LUX_FILM_ABERRATION_ENABLED:
			return m_AberrationEnabled;
			break;
		case LUX_FILM_ABERRATION_AMOUNT:
			return m_AberrationAmount;
			break;

		case LUX_FILM_GLARE_AMOUNT:
			return m_GlareAmount;
			break;
		case LUX_FILM_GLARE_RADIUS:
			return m_GlareRadius;
			break;
		case LUX_FILM_GLARE_BLADES:
			return m_GlareBlades;
			break;

		case LUX_FILM_HISTOGRAM_ENABLED:
			return m_HistogramEnabled;
			break;

		case LUX_FILM_NOISE_CHIU_ENABLED:
			return m_chiuParams.enabled;
			break;
		case LUX_FILM_NOISE_CHIU_RADIUS:
			return m_chiuParams.radius;
			break;
		case LUX_FILM_NOISE_CHIU_INCLUDECENTER:
			return m_chiuParams.includecenter;
			break;

		case LUX_FILM_NOISE_GREYC_ENABLED:
			return m_GREYCStorationParams.enabled;
			break;
		case LUX_FILM_NOISE_GREYC_AMPLITUDE:
			return m_GREYCStorationParams.amplitude;
			break;
		case LUX_FILM_NOISE_GREYC_NBITER:
			return m_GREYCStorationParams.nb_iter;
			break;
		case LUX_FILM_NOISE_GREYC_SHARPNESS:
			return m_GREYCStorationParams.sharpness;
			break;
		case LUX_FILM_NOISE_GREYC_ANISOTROPY:
			return m_GREYCStorationParams.anisotropy;
			break;
		case LUX_FILM_NOISE_GREYC_ALPHA:
			return m_GREYCStorationParams.alpha;
			break;
		case LUX_FILM_NOISE_GREYC_SIGMA:
			return m_GREYCStorationParams.sigma;
			break;
		case LUX_FILM_NOISE_GREYC_FASTAPPROX:
			return m_GREYCStorationParams.fast_approx;
			break;
		case LUX_FILM_NOISE_GREYC_GAUSSPREC:
			return m_GREYCStorationParams.gauss_prec;
			break;
		case LUX_FILM_NOISE_GREYC_DL:
			return m_GREYCStorationParams.dl;
			break;
		case LUX_FILM_NOISE_GREYC_DA:
			return m_GREYCStorationParams.da;
			break;
		case LUX_FILM_NOISE_GREYC_INTERP:
			return m_GREYCStorationParams.interp;
			break;
		case LUX_FILM_NOISE_GREYC_TILE:
			return m_GREYCStorationParams.tile;
			break;
		case LUX_FILM_NOISE_GREYC_BTILE:
			return m_GREYCStorationParams.btile;
			break;
		case LUX_FILM_NOISE_GREYC_THREADS:
			return m_GREYCStorationParams.threads;
			break;

		case LUX_FILM_LG_COUNT:
			return GetNumBufferGroups();
			break;
		case LUX_FILM_LG_ENABLE:
			return GetGroupEnable(index);
			break;
		case LUX_FILM_LG_SCALE:
			return GetGroupScale(index);
			break;
		case LUX_FILM_LG_SCALE_RED:
			return GetGroupRGBScale(index).c[0];
			break;
		case LUX_FILM_LG_SCALE_GREEN:
			return GetGroupRGBScale(index).c[1];
			break;
		case LUX_FILM_LG_SCALE_BLUE:
			return GetGroupRGBScale(index).c[2];
			break;
		case LUX_FILM_LG_TEMPERATURE:
			return GetGroupTemperature(index);
			break;

		default:
			break;
	 }
	 return 0.;
}
double FlexImageFilm::GetDefaultParameterValue(luxComponentParameters param, int index)
{
	 switch (param) {
		case LUX_FILM_TM_TONEMAPKERNEL:
			return d_TonemapKernel;
			break;

		case LUX_FILM_TM_REINHARD_PRESCALE:
			return d_ReinhardPreScale;
			break;
		case LUX_FILM_TM_REINHARD_POSTSCALE:
			return d_ReinhardPostScale;
			break;
		case LUX_FILM_TM_REINHARD_BURN:
			return d_ReinhardBurn;
			break;

		case LUX_FILM_TM_LINEAR_SENSITIVITY:
			return d_LinearSensitivity;
			break;
		case LUX_FILM_TM_LINEAR_EXPOSURE:
			return d_LinearExposure;
			break;
		case LUX_FILM_TM_LINEAR_FSTOP:
			return d_LinearFStop;
			break;
		case LUX_FILM_TM_LINEAR_GAMMA:
			return d_LinearGamma;
			break;

		case LUX_FILM_TM_CONTRAST_YWA:
			return d_ContrastYwa;
			break;

		case LUX_FILM_TORGB_X_WHITE:
			return d_RGB_X_White;
			break;
		case LUX_FILM_TORGB_Y_WHITE:
			return d_RGB_Y_White;
			break;
		case LUX_FILM_TORGB_X_RED:
			return d_RGB_X_Red;
			break;
		case LUX_FILM_TORGB_Y_RED:
			return d_RGB_Y_Red;
			break;
		case LUX_FILM_TORGB_X_GREEN:
			return d_RGB_X_Green;
			break;
		case LUX_FILM_TORGB_Y_GREEN:
			return d_RGB_Y_Green;
			break;
		case LUX_FILM_TORGB_X_BLUE:
			return d_RGB_X_Blue;
			break;
		case LUX_FILM_TORGB_Y_BLUE:
			return d_RGB_Y_Blue;
			break;
		case LUX_FILM_TORGB_GAMMA:
			return d_Gamma;
			break;

		case LUX_FILM_BLOOMRADIUS:
			return d_BloomRadius;
			break;
		case LUX_FILM_BLOOMWEIGHT:
			return d_BloomWeight;
			break;

		case LUX_FILM_VIGNETTING_ENABLED:
			return d_VignettingEnabled;
			break;
		case LUX_FILM_VIGNETTING_SCALE:
			return d_VignettingScale;
			break;

		case LUX_FILM_ABERRATION_ENABLED:
			return d_AberrationEnabled;
			break;
		case LUX_FILM_ABERRATION_AMOUNT:
			return d_AberrationAmount;
			break;

		case LUX_FILM_GLARE_AMOUNT:
			return d_GlareAmount;
			break;
		case LUX_FILM_GLARE_RADIUS:
			return d_GlareRadius;
			break;
		case LUX_FILM_GLARE_BLADES:
			return d_GlareBlades;
			break;

		case LUX_FILM_HISTOGRAM_ENABLED:
			return d_HistogramEnabled;
			break;

		case LUX_FILM_NOISE_CHIU_ENABLED:
			return d_chiuParams.enabled;
			break;
		case LUX_FILM_NOISE_CHIU_RADIUS:
			return d_chiuParams.radius;
			break;
		case LUX_FILM_NOISE_CHIU_INCLUDECENTER:
			return d_chiuParams.includecenter;
			break;

		case LUX_FILM_NOISE_GREYC_ENABLED:
			return d_GREYCStorationParams.enabled;
			break;
		case LUX_FILM_NOISE_GREYC_AMPLITUDE:
			return d_GREYCStorationParams.amplitude;
			break;
		case LUX_FILM_NOISE_GREYC_NBITER:
			return d_GREYCStorationParams.nb_iter;
			break;
		case LUX_FILM_NOISE_GREYC_SHARPNESS:
			return d_GREYCStorationParams.sharpness;
			break;
		case LUX_FILM_NOISE_GREYC_ANISOTROPY:
			return d_GREYCStorationParams.anisotropy;
			break;
		case LUX_FILM_NOISE_GREYC_ALPHA:
			return d_GREYCStorationParams.alpha;
			break;
		case LUX_FILM_NOISE_GREYC_SIGMA:
			return d_GREYCStorationParams.sigma;
			break;
		case LUX_FILM_NOISE_GREYC_FASTAPPROX:
			return d_GREYCStorationParams.fast_approx;
			break;
		case LUX_FILM_NOISE_GREYC_GAUSSPREC:
			return d_GREYCStorationParams.gauss_prec;
			break;
		case LUX_FILM_NOISE_GREYC_DL:
			return d_GREYCStorationParams.dl;
			break;
		case LUX_FILM_NOISE_GREYC_DA:
			return d_GREYCStorationParams.da;
			break;
		case LUX_FILM_NOISE_GREYC_INTERP:
			return d_GREYCStorationParams.interp;
			break;
		case LUX_FILM_NOISE_GREYC_TILE:
			return d_GREYCStorationParams.tile;
			break;
		case LUX_FILM_NOISE_GREYC_BTILE:
			return d_GREYCStorationParams.btile;
			break;
		case LUX_FILM_NOISE_GREYC_THREADS:
			return d_GREYCStorationParams.threads;
			break;

		case LUX_FILM_LG_ENABLE:
			return true;
			break;
		case LUX_FILM_LG_SCALE:
			return 1.f;
			break;
		case LUX_FILM_LG_SCALE_RED:
			return 1.f;
			break;
		case LUX_FILM_LG_SCALE_GREEN:
			return 1.f;
			break;
		case LUX_FILM_LG_SCALE_BLUE:
			return 1.f;
			break;
		case LUX_FILM_LG_TEMPERATURE:
			return 0.f;
			break;

		default:
			break;
	 }
	 return 0.;
}

void FlexImageFilm::SetStringParameterValue(luxComponentParameters param, const string& value, int index) {
	switch(param) {
		case LUX_FILM_LG_NAME:
			return SetGroupName(index, value);
		default:
			break;
	}
}
string FlexImageFilm::GetStringParameterValue(luxComponentParameters param, int index) {
	switch(param) {
		case LUX_FILM_LG_NAME:
			return GetGroupName(index);
		default:
			break;
	}
	return "";
}

void FlexImageFilm::GetSampleExtent(int *xstart, int *xend,
	int *ystart, int *yend) const
{
	*xstart = Floor2Int(xPixelStart + .5f - filter->xWidth);
	*xend   = Floor2Int(xPixelStart + .5f + xPixelCount + filter->xWidth);
	*ystart = Floor2Int(yPixelStart + .5f - filter->yWidth);
	*yend   = Floor2Int(yPixelStart + .5f + yPixelCount + filter->yWidth);
}

void FlexImageFilm::RequestBufferGroups(const vector<string> &bg)
{
	for (u_int i = 0; i < bg.size(); ++i)
		bufferGroups.push_back(BufferGroup(bg[i]));
}

int FlexImageFilm::RequestBuffer(BufferType type, BufferOutputConfig output,
	const string& filePostfix)
{
	bufferConfigs.push_back(BufferConfig(type, output, filePostfix));
	return bufferConfigs.size() - 1;
}

void FlexImageFilm::CreateBuffers()
{
	if (bufferGroups.size() == 0)
		bufferGroups.push_back(BufferGroup("default"));
	for (u_int i = 0; i < bufferGroups.size(); ++i)
		bufferGroups[i].CreateBuffers(bufferConfigs,xPixelCount,yPixelCount);

	// Allocate ZBuf buffer if needed
	if(use_Zbuf)
		ZBuffer = new PerPixelNormalizedFloatBuffer(xPixelCount,yPixelCount);

    // Dade - check if we have to resume a rendering and restore the buffers
    if(writeResumeFlm && !restartResumeFlm) {
        // Dade - check if the film file exists
        string fname = filename+".flm";
		std::ifstream ifs(fname.c_str(), std::ios_base::in | std::ios_base::binary);

        if(ifs.good()) {
            // Dade - read the data
            luxError(LUX_NOERROR, LUX_INFO, (std::string("Reading film status from file ")+fname).c_str());
            UpdateFilm(ifs);
        }

        ifs.close();
    }
}

void FlexImageFilm::SetGroupName(u_int index, const string& name) 
{
	if( index >= bufferGroups.size())
		return;
	bufferGroups[index].name = name;
}
string FlexImageFilm::GetGroupName(u_int index) const
{
	if (index >= bufferGroups.size())
		return "";
	return bufferGroups[index].name;
}
void FlexImageFilm::SetGroupEnable(u_int index, bool status)
{
	if (index >= bufferGroups.size())
		return;
	bufferGroups[index].enable = status;
}
bool FlexImageFilm::GetGroupEnable(u_int index) const
{
	if (index >= bufferGroups.size())
		return false;
	return bufferGroups[index].enable;
}
void FlexImageFilm::SetGroupScale(u_int index, float value)
{
	if (index >= bufferGroups.size())
		return;
	bufferGroups[index].globalScale = value;
	ComputeGroupScale(index);
}
float FlexImageFilm::GetGroupScale(u_int index) const
{
	if (index >= bufferGroups.size())
		return 0.f;
	return bufferGroups[index].globalScale;
}
void FlexImageFilm::SetGroupRGBScale(u_int index, const RGBColor &value)
{
	if (index >= bufferGroups.size())
		return;
	bufferGroups[index].rgbScale = value;
	ComputeGroupScale(index);
}
RGBColor FlexImageFilm::GetGroupRGBScale(u_int index) const
{
	if (index >= bufferGroups.size())
		return 0.f;
	return bufferGroups[index].rgbScale;
}
void FlexImageFilm::SetGroupTemperature(u_int index, float value)
{
	if (index >= bufferGroups.size())
		return;
	bufferGroups[index].temperature = value;
	ComputeGroupScale(index);
}
float FlexImageFilm::GetGroupTemperature(u_int index) const
{
	if (index >= bufferGroups.size())
		return 0.f;
	return bufferGroups[index].temperature;
}
void FlexImageFilm::ComputeGroupScale(u_int index)
{
	const XYZColor white(colorSpace.ToXYZ(RGBColor(1.f)));
	bufferGroups[index].scale =
		colorSpace.ToXYZ(bufferGroups[index].rgbScale) / white;
	if (bufferGroups[index].temperature > 0.f) {
		XYZColor factor(BlackbodySPD(bufferGroups[index].temperature).ToXYZ());
		bufferGroups[index].scale *= factor / (factor.Y() * white);
	}
	bufferGroups[index].scale *= bufferGroups[index].globalScale;
}

void FlexImageFilm::AddSampleCount(float count) {
	for (u_int i = 0; i < bufferGroups.size(); ++i) {
		bufferGroups[i].numberOfSamples += count;

		// Dade - check if we have enough samples per pixel
		if ((haltSamplePerPixel > 0) &&
			(bufferGroups[i].numberOfSamples  >= haltSamplePerPixel * samplePerPass))
			enoughSamplePerPixel = true;
	}
}

void FlexImageFilm::AddSample(Contribution *contrib) {
	XYZColor xyz = contrib->color;
	const float alpha = contrib->alpha;
	const float weight = contrib->variance;

	// Issue warning if unexpected radiance value returned
	if (xyz.IsNaN() || xyz.Y() < -1e-5f || isinf(xyz.Y())) {
		if(debug_mode) {
			std::stringstream ss;
			ss << "Out of bound intensity in FlexImageFilm::AddSample: "
			   << xyz.Y() << ", sample discarded";
			luxError(LUX_LIMIT, LUX_WARNING, ss.str().c_str());
		}
		return;
	}

	if (alpha < 0 || isnan(alpha) || isinf(alpha))
		return;

	if (weight < 0 || isnan(weight) || isinf(weight))
		return;

	// Reject samples higher than max Y() after warmup period
	if (warmupComplete && xyz.Y() > maxY)
		return;
	else {
	 	maxY = max(maxY, xyz.Y());
		++warmupSamples;
	 	if (warmupSamples >= reject_warmup_samples)
			warmupComplete = true;
	}

	if (premultiplyAlpha)
		xyz *= alpha;

	BufferGroup &currentGroup = bufferGroups[contrib->bufferGroup];
	Buffer *buffer = currentGroup.getBuffer(contrib->buffer);

	// Compute sample's raster extent
	float dImageX = contrib->imageX - 0.5f;
	float dImageY = contrib->imageY - 0.5f;
	int x0 = Ceil2Int (dImageX - filter->xWidth);
	int x1 = Floor2Int(dImageX + filter->xWidth);
	int y0 = Ceil2Int (dImageY - filter->yWidth);
	int y1 = Floor2Int(dImageY + filter->yWidth);
/*	x0 = max(x0, xPixelStart);
	x1 = min(x1, xPixelStart + xPixelCount - 1);
	y0 = max(y0, yPixelStart);
	y1 = min(y1, yPixelStart + yPixelCount - 1);*/
	if (x1 < x0 || y1 < y0) return;
	// Loop over filter support and add sample to pixel arrays
	// Precompute $x$ and $y$ filter table offsets
//	int *ifx = (int *)alloca((x1-x0+1) * sizeof(int));				// TODO - radiance - pre alloc memory in constructor for speedup ?
	int ifx[32];
	for (int x = x0; x <= x1; ++x) {
		float fx = fabsf((x - dImageX) *
			filter->invXWidth * FILTER_TABLE_SIZE);
		ifx[x-x0] = min(Floor2Int(fx), FILTER_TABLE_SIZE-1);
	}
//	int *ify = (int *)alloca((y1-y0+1) * sizeof(int));				// TODO - radiance - pre alloc memory in constructor for speedup ?
	int ify[32];
	for (int y = y0; y <= y1; ++y) {
		float fy = fabsf((y - dImageY) *
			filter->invYWidth * FILTER_TABLE_SIZE);
		ify[y-y0] = min(Floor2Int(fy), FILTER_TABLE_SIZE-1);
	}
	float filterNorm = 0.f;
	for (int y = y0; y <= y1; ++y) {
		for (int x = x0; x <= x1; ++x) {
			const int offset = ify[y-y0]*FILTER_TABLE_SIZE + ifx[x-x0];
			filterNorm += filterTable[offset];
		}
	}
	filterNorm = weight / filterNorm;

	for (int y = max(y0, yPixelStart); y <= min(y1, yPixelStart + yPixelCount - 1); ++y) {
		for (int x = max(x0, xPixelStart); x <= min(x1, xPixelStart + xPixelCount - 1); ++x) {
			// Evaluate filter value at $(x,y)$ pixel
			const int offset = ify[y-y0]*FILTER_TABLE_SIZE + ifx[x-x0];
			const float filterWt = filterTable[offset] * filterNorm;
			// Update pixel values with filtered sample contribution
			buffer->Add(x - xPixelStart,y - yPixelStart,
				xyz, alpha, filterWt);
			// Update ZBuffer values with filtered zdepth contribution
			if(use_Zbuf && contrib->zdepth != 0.f)
				ZBuffer->Add(x - xPixelStart, y - yPixelStart, contrib->zdepth, 1.0f);
		}
	}

	// Check write output interval
	boost::xtime currentTime;
	boost::xtime_get(&currentTime, boost::TIME_UTC);
	bool timeToWriteImage = (currentTime.sec - lastWriteImageTime.sec > writeInterval);

	// Possibly write out in-progress image
	if (timeToWriteImage) {
		boost::xtime_get(&lastWriteImageTime, boost::TIME_UTC);
		WriteImage(IMAGE_FILEOUTPUT);
	}
}

void FlexImageFilm::WriteImage2(ImageType type, vector<XYZColor> &xyzcolor, vector<float> &alpha, string postfix)
{
	// Construct ColorSystem from values
	colorSpace = ColorSystem(m_RGB_X_Red, m_RGB_Y_Red,
		m_RGB_X_Green, m_RGB_Y_Green,
		m_RGB_X_Blue, m_RGB_Y_Blue,
		m_RGB_X_White, m_RGB_Y_White, 1.f);

	// Construct normalized Z buffer if used
	vector<float> zBuf;
	if(use_Zbuf && (write_EXR_ZBuf || write_PNG_ZBuf || write_TGA_ZBuf)) {
		const u_int nPix = xPixelCount * yPixelCount;
		zBuf.resize(nPix, 0.f);
		for (int offset = 0, y = 0; y < yPixelCount; ++y) {
			for (int x = 0; x < xPixelCount; ++x,++offset) {
				zBuf[offset] = ZBuffer->GetData(x, y);
			}
		}
	}

	if (type & IMAGE_FILEOUTPUT) {
		// write out untonemapped EXR
		if (write_EXR && !write_EXR_applyimaging) {
			// convert to rgb
			const u_int nPix = xPixelCount * yPixelCount;
			vector<RGBColor> rgbColor(nPix);
			for ( u_int i = 0; i < nPix; i++ )
				rgbColor[i] = colorSpace.ToRGBConstrained(xyzcolor[i]);

			WriteEXRImage(rgbColor, alpha, filename + postfix + ".exr", zBuf);
		}

		// Dade - save the current status of the film if required
		if (writeResumeFlm)
			WriteResumeFilm(filename + ".flm");
	}

	// Dade - check if I have to run ApplyImagingPipeline
	if (((type & IMAGE_FRAMEBUFFER) && framebuffer) ||
		((type & IMAGE_FILEOUTPUT) && ((write_EXR && write_EXR_applyimaging) || write_TGA || write_PNG))) {
		// Apply the imaging/tonemapping pipeline
		ParamSet toneParams;
		std::string tmkernel = "reinhard";
		if(m_TonemapKernel == 0) {
			// Reinhard Tonemapper
			toneParams.AddFloat("prescale", &m_ReinhardPreScale, 1);
			toneParams.AddFloat("postscale", &m_ReinhardPostScale, 1);
			toneParams.AddFloat("burn", &m_ReinhardBurn, 1);
			tmkernel = "reinhard";
		} else if(m_TonemapKernel == 1) {
			// Linear Tonemapper
			toneParams.AddFloat("sensitivity", &m_LinearSensitivity, 1);
			toneParams.AddFloat("exposure", &m_LinearExposure, 1);
			toneParams.AddFloat("fstop", &m_LinearFStop, 1);
			toneParams.AddFloat("gamma", &m_LinearGamma, 1);
			tmkernel = "linear";
		} else if(m_TonemapKernel == 2) {
			// Contrast Tonemapper
			toneParams.AddFloat("ywa", &m_ContrastYwa, 1);
			tmkernel = "contrast";
		} else {		
			// MaxWhite Tonemapper
			tmkernel = "maxwhite";
		}

		// Delete bloom/glare layers if requested
		if (!m_BloomUpdateLayer && m_BloomDeleteLayer && m_HaveBloomImage) {
			// TODO - make thread safe
			m_HaveBloomImage = false;
			delete[] m_bloomImage;
			m_bloomImage = NULL;
			m_BloomDeleteLayer = false;
		}

		if (!m_GlareUpdateLayer && m_GlareDeleteLayer && m_HaveGlareImage) {
			// TODO - make thread safe
			m_HaveGlareImage = false;
			delete[] m_glareImage;
			m_glareImage = NULL;
			m_GlareDeleteLayer = false;
		}

		// Apply chosen tonemapper
		ApplyImagingPipeline(xyzcolor, xPixelCount, yPixelCount, m_GREYCStorationParams, m_chiuParams,
			colorSpace, histogram, m_HistogramEnabled, m_HaveBloomImage, m_bloomImage, m_BloomUpdateLayer,
			m_BloomRadius, m_BloomWeight, m_VignettingEnabled, m_VignettingScale, m_AberrationEnabled, m_AberrationAmount,
			m_HaveGlareImage, m_glareImage, m_GlareUpdateLayer, m_GlareAmount, m_GlareRadius, m_GlareBlades,
			tmkernel.c_str(), &toneParams, m_Gamma, 0.f);

		// DO NOT USE xyzcolor ANYMORE AFTER THIS POINT
		vector<RGBColor> &rgbcolor = reinterpret_cast<vector<RGBColor> &>(xyzcolor);

		// Disable further bloom layer updates if used.
		m_BloomUpdateLayer = false;
		m_GlareUpdateLayer = false;

		if (type & IMAGE_FILEOUTPUT) {
			// write out tonemapped EXR
			if ((write_EXR && write_EXR_applyimaging))
				WriteEXRImage(rgbcolor, alpha, filename + postfix + ".exr", zBuf);
		}

		// Output to low dynamic range formats
		if ((type & IMAGE_FILEOUTPUT) || (type & IMAGE_FRAMEBUFFER)) {
			// Clamp too high values
			const u_int nPix = xPixelCount * yPixelCount;
			for (u_int i = 0; i < nPix; ++i)
				rgbcolor[i] = colorSpace.Limit(rgbcolor[i], clampMethod);

			// write out tonemapped TGA
			if ((type & IMAGE_FILEOUTPUT) && write_TGA)
				WriteTGAImage(rgbcolor, alpha, filename + postfix + ".tga");
			// write out tonemapped PNG
			if ((type & IMAGE_FILEOUTPUT) && write_PNG)
				WritePNGImage(rgbcolor, alpha, filename + postfix + ".png");
			// Copy to framebuffer pixels
			if ((type & IMAGE_FRAMEBUFFER) && framebuffer) {
				for (u_int i = 0; i < nPix; i++) {
					framebuffer[3 * i] = static_cast<unsigned char>(Clamp(256 * rgbcolor[i].c[0], 0.f, 255.f));
					framebuffer[3 * i + 1] = static_cast<unsigned char>(Clamp(256 * rgbcolor[i].c[1], 0.f, 255.f));
					framebuffer[3 * i + 2] = static_cast<unsigned char>(Clamp(256 * rgbcolor[i].c[2], 0.f, 255.f));
				}
			}
		}
	}
}

void FlexImageFilm::WriteImage(ImageType type)
{
	const int nPix = xPixelCount * yPixelCount;
	vector<XYZColor> pixels(nPix);
	vector<float> alpha(nPix), alphaWeight(nPix, 0.f);

	// NOTE - lordcrc - separated buffer loop into two separate loops
	// in order to eliminate one of the framebuffer copies

	// write stand-alone buffers
	for(u_int j = 0; j < bufferGroups.size(); ++j) {
		if (!bufferGroups[j].enable)
			continue;

		for(u_int i = 0; i < bufferConfigs.size(); ++i) {
			const Buffer &buffer = *(bufferGroups[j].buffers[i]);

			if (!(bufferConfigs[i].output & BUF_STANDALONE))
				continue;

			buffer.GetData(&(pixels[0]), &(alpha[0]));
			WriteImage2(type, pixels, alpha, bufferConfigs[i].postfix);
		}
	}

	float Y = 0.f;
	// in order to fix bug #360
	// ouside loop not to trash the complete picture
	// if there are several buffer groups
	fill(pixels.begin(), pixels.end(), XYZColor(0.f));
	fill(alpha.begin(), alpha.end(), 0.f);

	XYZColor p;
	float a;

	// write framebuffer
	for(u_int j = 0; j < bufferGroups.size(); ++j) {
		if (!bufferGroups[j].enable)
			continue;

		for(u_int i = 0; i < bufferConfigs.size(); ++i) {
			const Buffer &buffer = *(bufferGroups[j].buffers[i]);
			if (!(bufferConfigs[i].output & BUF_FRAMEBUFFER))
				continue;

			for (int offset = 0, y = 0; y < yPixelCount; ++y) {
				for (int x = 0; x < xPixelCount; ++x,++offset) {

					alphaWeight[offset] += buffer.GetData(x, y, &p, &a);

					pixels[offset] += p * bufferGroups[j].scale;
					alpha[offset] += a;
				}
			}
		}
	}
	// outside loop in order to write complete image
	for (int pix = 0; pix < nPix; ++pix) {
		if (alphaWeight[pix] > 0.f)
			alpha[pix] /= alphaWeight[pix];
		Y += pixels[pix].c[1];
	}
	Y /= nPix;
	WriteImage2(type, pixels, alpha, "");
	EV = logf(Y * 10.f) / logf(2.f);
}

// GUI LDR framebuffer access methods
void FlexImageFilm::createFrameBuffer()
{
	// allocate pixels
	unsigned int nPix = xPixelCount * yPixelCount;
	framebuffer = new unsigned char[3*nPix];			// TODO delete data

	// zero it out
	memset(framebuffer,0,sizeof(*framebuffer)*3*nPix);
}
void FlexImageFilm::updateFrameBuffer()
{
	if(!framebuffer) {
		createFrameBuffer();
	}

	WriteImage(IMAGE_FRAMEBUFFER);
}
unsigned char* FlexImageFilm::getFrameBuffer()
{
	if(!framebuffer)
		createFrameBuffer();

	return framebuffer;
}

void FlexImageFilm::WriteResumeFilm(const string &filename)
{
	// Dade - save the status of the film to the file
	luxError(LUX_NOERROR, LUX_INFO, (std::string("Writing film status to file ") +
			filename).c_str());

    std::ofstream filestr(filename.c_str(), std::ios_base::out | std::ios_base::binary);
	if(!filestr) {
		std::stringstream ss;
	 	ss << "Cannot open file '" << filename << "' for writing resume film";
		luxError(LUX_SYSTEM, LUX_SEVERE, ss.str().c_str());

		return;
	}

    TransmitFilm(filestr,false,true);

    filestr.close();
}

void FlexImageFilm::WriteTGAImage(vector<RGBColor> &rgb, vector<float> &alpha, const string &filename)
{
	// Write Truevision Targa TGA image
	luxError(LUX_NOERROR, LUX_INFO, (std::string("Writing Tonemapped TGA image to file ")+filename).c_str());
	WriteTargaImage(write_TGA_channels, write_TGA_ZBuf, filename, rgb, alpha,
		xPixelCount, yPixelCount,
		xResolution, yResolution,
		xPixelStart, yPixelStart);
}

void FlexImageFilm::WritePNGImage(vector<RGBColor> &rgb, vector<float> &alpha, const string &filename)
{
	// Write Portable Network Graphics PNG image
	luxError(LUX_NOERROR, LUX_INFO, (std::string("Writing Tonemapped PNG image to file ")+filename).c_str());
	WritePngImage(write_PNG_channels, write_PNG_16bit, write_PNG_ZBuf, filename, rgb, alpha,
		xPixelCount, yPixelCount,
		xResolution, yResolution,
		xPixelStart, yPixelStart, colorSpace, m_Gamma);
}

void FlexImageFilm::WriteEXRImage(vector<RGBColor> &rgb, vector<float> &alpha, const string &filename, vector<float> &zbuf)
{
	
	if(write_EXR_ZBuf) {
		if(write_EXR_ZBuf_normalizationtype == CameraStartEnd) {
			// Camera normalization
		} else if(write_EXR_ZBuf_normalizationtype == MinMax) {
			// Min/Max normalization
			const u_int nPix = xPixelCount * yPixelCount;
			float min = 0.f;
			float max = INFINITY;
			for(u_int i=0; i<nPix; i++) {
				if(zbuf[i] > 0.f) {
					if(zbuf[i] > min) min = zbuf[i];
					if(zbuf[i] < max) max = zbuf[i];
				}
			}

			vector<float> zBuf(nPix);
			for (u_int i=0; i<nPix; i++)
				zBuf[i] = (zbuf[i]-min) / (max-min);

			luxError(LUX_NOERROR, LUX_INFO, (std::string("Writing OpenEXR image to file ")+filename).c_str());
			WriteOpenEXRImage(write_EXR_channels, write_EXR_halftype, write_EXR_ZBuf, write_EXR_compressiontype, filename, rgb, alpha,
				xPixelCount, yPixelCount,
				xResolution, yResolution,
				xPixelStart, yPixelStart, zBuf);
			return;
		}
	}

	// Write OpenEXR RGBA image
	luxError(LUX_NOERROR, LUX_INFO, (std::string("Writing OpenEXR image to file ")+filename).c_str());
	WriteOpenEXRImage(write_EXR_channels, write_EXR_halftype, write_EXR_ZBuf, write_EXR_compressiontype, filename, rgb, alpha,
		xPixelCount, yPixelCount,
		xResolution, yResolution,
		xPixelStart, yPixelStart, zbuf);
}

/**
 * FLM format
 * ----------
 *
 * Layout:
 *
 *   HEADER
 *   magic_number                  - int   - the magic number number
 *   version_number                - int   - the version number
 *   x_resolution                  - int   - the x resolution of the buffers
 *   y_resolution                  - int   - the y resolution of the buffers
 *   #buffer_groups                - u_int - the number of lightgroups
 *   #buffer_configs               - u_int - the number of buffers per light group
 *   for i in 1:#buffer_configs
 *     buffer_type                 - int   - the type of the i'th buffer
 *   #parameters                   - u_int - the number of stored parameters
 *   for i in 1:#parameters
 *     param_type                  - int   - the type of the i'th parameter
 *     param_size                  - int   - the size of the value of the i'th parameter in bytes
 *     param_id                    - int   - the id of the i'th parameter
 *     param_index                 - int   - the index of the i'th parameter
 *     param_value                 - *     - the value of the i'th parameter
 *
 *   DATA
 *   for i in 1:#buffer_groups
 *     #samples                    - float - the number of samples in the i'th buffer group
 *     for j in 1:#buffer_configs
 *       for y in 1:y_resolution
 *         for x in 1:x_resolution
 *           X                     - float - the weighted sum of all X values added to the pixel
 *           Y                     - float - the weighted sum of all Y values added to the pixel
 *           Z                     - float - the weighted sum of all Z values added to the pixel
 *           alpha                 - float - the weighted sum of all alpha values added to the pixel
 *           weight_sum            - float - the sum of al weights of all values added to the pixel
 *     
 *
 * Remarks:
 *  - data is written as binary little-endian
 *  - data is gzipped
 *  - the version is not intended for backward/forward compatibility but just as a check
 */
static const int FLM_MAGIC_NUMBER = 0xCEBCD816;
static const int FLM_VERSION = 0; // should be incremented on each change to the format to allow detecting unsupported FLM data!
enum FlmParameterType {
	FLM_PARAMETER_TYPE_FLOAT = 0,
	FLM_PARAMETER_TYPE_STRING = 1
};

class FlmParameter {
public:
	FlmParameter() {}
	FlmParameter(FlexImageFilm *aFilm, FlmParameterType aType, luxComponentParameters aParam, int aIndex) {
		type = aType;
		id = (int)aParam;
		index = aIndex;
		switch(type) {
			case FLM_PARAMETER_TYPE_FLOAT:
				size = 4;
				floatValue = (float)aFilm->GetParameterValue(aParam, aIndex);
				break;
			case FLM_PARAMETER_TYPE_STRING:
				stringValue = aFilm->GetStringParameterValue(aParam, aIndex);
				size = (int)stringValue.size();
				break;
			default:
				{
					std::stringstream ss;
					ss << "Invalid parameter type (expected value in [0,1], got=" << type << ")";
					luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str() );
				}
				break;
		}
	}

	void Set(FlexImageFilm *aFilm) {
		switch(type) {
			case FLM_PARAMETER_TYPE_FLOAT:
				aFilm->SetParameterValue(luxComponentParameters(id), (double) floatValue, index);
				break;
			case FLM_PARAMETER_TYPE_STRING:
				aFilm->SetStringParameterValue(luxComponentParameters(id), stringValue, index);
				break;
			default:
				// ignore invalid type (already reported in constructor)
				break;
		}
	}

	bool Read(std::basic_istream<char> &is, bool isLittleEndian, FlexImageFilm *film ) {
		int tmpType;
		tmpType = osReadLittleEndianInt(isLittleEndian, is);
		type = FlmParameterType(tmpType);
		if (!is.good()) {
			luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
			return false;
		}
		if( type < 0 || type > 1 ) {
			std::stringstream ss;
			ss << "Invalid parameter type (expected value in [0,1], received=" << tmpType << ")";
			luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str() );
			return false;
		}
		size = osReadLittleEndianInt(isLittleEndian, is);
		if (!is.good()) {
			luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
			return false;
		}
		if( size < 0 || size > 1024 ) { // currently parameter values are limited to 1KB (seems reasonable)
			std::stringstream ss;
			ss << "Invalid parameter size (expected value in [0,1024], received=" << size << ")";
			luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str() );
			return false;
		}
		id = osReadLittleEndianInt(isLittleEndian, is);
		if (!is.good()) {
			luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
			return false;
		}
		index = osReadLittleEndianInt(isLittleEndian, is);
		if (!is.good()) {
			luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
			return false;
		}
		if( index < 0 ) {
			std::stringstream ss;
			ss << "Invalid parameter index (expected positive value, received=" << index << ")";
			luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str() );
			return false;
		}
		switch(type) {
			case FLM_PARAMETER_TYPE_FLOAT:
				floatValue = osReadLittleEndianFloat(isLittleEndian, is);
				break;
			case FLM_PARAMETER_TYPE_STRING:
				{
					char* chars = new char[size+1];
					is.read(chars, size);
					chars[size] = '\0';
					stringValue = string(chars);
					delete[] chars;
				}
				break;
			default:
				return false;
		}
		return true;
	}
	void Write(std::basic_ostream<char> &os, bool isLittleEndian) const {
		osWriteLittleEndianInt(isLittleEndian, os, type);
		osWriteLittleEndianInt(isLittleEndian, os, size);
		osWriteLittleEndianInt(isLittleEndian, os, id);
		osWriteLittleEndianInt(isLittleEndian, os, index);
		switch(type) {
			case FLM_PARAMETER_TYPE_FLOAT:
				osWriteLittleEndianFloat(isLittleEndian, os, floatValue);
				break;
			case FLM_PARAMETER_TYPE_STRING:
				os.write(stringValue.c_str(), size);
				break;
			default:
				// ignore invalid type (already reported in constructor)
				break;
		}
	}

private:
	FlmParameterType type;
	int size;
	int id;
	int index;
		
	float floatValue;
	string stringValue;
};

class FlmHeader {
public:
	FlmHeader() {}
	bool Read(filtering_stream<input> &in, bool isLittleEndian, FlexImageFilm *film );
	void Write(std::basic_ostream<char> &os, bool isLittleEndian) const;

	int magicNumber;
	int versionNumber;
	int xResolution;
	int yResolution;
	u_int numBufferGroups;
	u_int numBufferConfigs;
	vector<int> bufferTypes;
	u_int numParams;
	vector<FlmParameter> params;
};

bool FlmHeader::Read(filtering_stream<input> &in, bool isLittleEndian, FlexImageFilm *film ) {
	// Read and verify magic number and version
	magicNumber = osReadLittleEndianInt(isLittleEndian, in);
	if (!in.good()) {
		luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
		return false;
	}
	if (magicNumber != FLM_MAGIC_NUMBER) {
		std::stringstream ss;
		ss << "Invalid FLM magic number (expected=" << FLM_MAGIC_NUMBER 
			<< ", received=" << magicNumber << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	versionNumber = osReadLittleEndianInt(isLittleEndian, in);
	if (!in.good()) {
		luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
		return false;
	}
	if (versionNumber != FLM_VERSION) {
		std::stringstream ss;
		ss << "Invalid FLM version (expected=" << FLM_VERSION 
			<< ", received=" << versionNumber << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	// Read and verify the buffer resolution
	xResolution = osReadLittleEndianInt(isLittleEndian, in);
	yResolution = osReadLittleEndianInt(isLittleEndian, in);
	if (xResolution <= 0 || yResolution <= 0 ) {
		std::stringstream ss;
		ss << "Invalid resolution (expected positive resolution, received=" << xResolution << "x" << yResolution << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	if (film != NULL &&
		(xResolution != film->GetXPixelCount() ||
		yResolution != film->GetYPixelCount())) {
		std::stringstream ss;
		ss << "Invalid resolution (expected=" << film->GetXPixelCount() << "x" << film->GetYPixelCount();
		ss << ", received=" << xResolution << "x" << yResolution << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	// Read and verify #buffer groups and buffer configs
	numBufferGroups = osReadLittleEndianUInt(isLittleEndian, in);
	if (!in.good()) {
		luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
		return false;
	}
	if (film != NULL && numBufferGroups != film->GetNumBufferGroups()) {
		std::stringstream ss;
		ss << "Invalid number of buffer groups (expected=" << film->GetNumBufferGroups() 
			<< ", received=" << numBufferGroups << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	numBufferConfigs = osReadLittleEndianUInt(isLittleEndian, in);
	if (!in.good()) {
		luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
		return false;
	}
	if (film != NULL && numBufferConfigs != film->GetNumBufferConfigs()) {
		std::stringstream ss;
		ss << "Invalid number of buffers (expected=" << film->GetNumBufferConfigs()
			<< ", received=" << numBufferConfigs << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	for (u_int i = 0; i < numBufferConfigs; ++i) {
		int type;
		type = osReadLittleEndianInt(isLittleEndian, in);
		if (!in.good()) {
			luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
			return false;
		}
		if (type < 0 || type >= NUM_OF_BUFFER_TYPES) {
			std::stringstream ss;
			ss << "Invalid buffer type for buffer " << i << "(expected number in [0," << NUM_OF_BUFFER_TYPES << "[, received=" << type << ")";
			luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
			return false;
		}
		if (film != NULL && type != film->GetBufferConfig(i).type) {
			std::stringstream ss;
			ss << "Invalid buffer type for buffer " << i << " (expected=" << film->GetBufferConfig(i).type
				<< ", received=" << type << ")";
			luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
			return false;
		}
		bufferTypes.push_back(type);
	}
	// Read parameters
	numParams = osReadLittleEndianUInt(isLittleEndian, in);
	if (!in.good()) {
		luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
		return false;
	}
	params.reserve(numParams);
	for(u_int i = 0; i < numParams; ++i) {
		FlmParameter param;
		bool ok = param.Read(in, isLittleEndian, film);
		if (!in.good()) {
			luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
			return false;
		}
		if(!ok) {
			//std::stringstream ss;
			//ss << "Invalid parameter (id=" << param.id << ", index=" << param.index << ", value=" << param.value << ")";
			//luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
			return false;
		}
		params.push_back(param);
	}
	return true;
}

void FlmHeader::Write(std::basic_ostream<char> &os, bool isLittleEndian) const
{
	// Write magic number and version
	osWriteLittleEndianInt(isLittleEndian, os, magicNumber);
	osWriteLittleEndianInt(isLittleEndian, os, versionNumber);
	// Write buffer resolution
	osWriteLittleEndianInt(isLittleEndian, os, xResolution);
	osWriteLittleEndianInt(isLittleEndian, os, yResolution);
	// Write #buffer groups and buffer configs for verification
	osWriteLittleEndianUInt(isLittleEndian, os, numBufferGroups);
	osWriteLittleEndianUInt(isLittleEndian, os, numBufferConfigs);
	for (u_int i = 0; i < numBufferConfigs; ++i)
		osWriteLittleEndianInt(isLittleEndian, os, bufferTypes[i]);
	// Write parameters
	osWriteLittleEndianUInt(isLittleEndian, os, numParams);
	for(u_int i = 0; i < numParams; ++i) {
		params[i].Write(os, isLittleEndian);
	}
}

void FlexImageFilm::TransmitFilm(
        std::basic_ostream<char> &stream,
        bool clearBuffers,
		bool transmitParams) 
{
    const bool isLittleEndian = osIsLittleEndian();

    std::stringstream ss;
    ss << "Transmitting film (little endian=" <<(isLittleEndian ? "true" : "false") << ")";
    luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

    std::stringstream os;
	// Write the header
	FlmHeader header;
	header.magicNumber = FLM_MAGIC_NUMBER;
	header.versionNumber = FLM_VERSION;
	header.xResolution = GetXPixelCount();
	header.yResolution = GetYPixelCount();
	header.numBufferGroups = bufferGroups.size();
	header.numBufferConfigs = bufferConfigs.size();
	for (u_int i = 0; i < bufferConfigs.size(); ++i)
		header.bufferTypes.push_back(bufferConfigs[i].type);
	// Write parameters
	if (transmitParams) {
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_TONEMAPKERNEL, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_REINHARD_PRESCALE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_REINHARD_POSTSCALE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_REINHARD_BURN, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_LINEAR_SENSITIVITY, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_LINEAR_EXPOSURE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_LINEAR_FSTOP, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_LINEAR_GAMMA, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TM_CONTRAST_YWA, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_X_WHITE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_Y_WHITE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_X_RED, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_Y_RED, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_X_GREEN, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_Y_GREEN, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_X_BLUE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_Y_BLUE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_TORGB_GAMMA, 0));


		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_UPDATEBLOOMLAYER, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_DELETEBLOOMLAYER, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_BLOOMRADIUS, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_BLOOMWEIGHT, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_VIGNETTING_ENABLED, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_VIGNETTING_SCALE, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_ABERRATION_ENABLED, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_ABERRATION_AMOUNT, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_UPDATEGLARELAYER, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_DELETEGLARELAYER, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_GLARE_AMOUNT, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_GLARE_RADIUS, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_GLARE_BLADES, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_CHIU_ENABLED, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_CHIU_RADIUS, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_CHIU_INCLUDECENTER, 0));

		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_ENABLED, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_AMPLITUDE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_NBITER, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_SHARPNESS, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_ANISOTROPY, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_ALPHA, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_SIGMA, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_FASTAPPROX, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_GAUSSPREC, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_DL, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_DA, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_INTERP, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_TILE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_BTILE, 0));
		header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_NOISE_GREYC_THREADS, 0));

		for(u_int i = 0; i < GetNumBufferGroups(); ++i) {
			header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_LG_SCALE, int(i)));
			header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_LG_ENABLE, int(i)));
			header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_LG_SCALE_RED, int(i)));
			header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_LG_SCALE_GREEN, int(i)));
			header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_LG_SCALE_BLUE, int(i)));
			header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_FLOAT, LUX_FILM_LG_TEMPERATURE, int(i)));

			header.params.push_back(FlmParameter(this, FLM_PARAMETER_TYPE_STRING, LUX_FILM_LG_NAME, int(i)));
		}

		header.numParams = header.params.size();
	} else {
		header.numParams = 0;
	}
	header.Write(os, isLittleEndian);
	// Write each buffer group
	for (u_int i = 0; i < bufferGroups.size(); ++i) {
		BufferGroup& bufferGroup = bufferGroups[i];
		// Write number of samples
		osWriteLittleEndianDouble(isLittleEndian, os, bufferGroup.numberOfSamples);

		// Write each buffer
		for (u_int j = 0; j < bufferConfigs.size(); ++j) {
			Buffer* buffer = bufferGroup.getBuffer(j);

			// Write pixels
			const BlockedArray<Pixel>* pixelBuf = buffer->pixels;
			for (int y = 0; y < pixelBuf->vSize(); ++y) {
				for (int x = 0; x < pixelBuf->uSize(); ++x) {
					const Pixel &pixel = (*pixelBuf)(x, y);
					osWriteLittleEndianFloat(isLittleEndian, os, pixel.L.c[0]);
					osWriteLittleEndianFloat(isLittleEndian, os, pixel.L.c[1]);
					osWriteLittleEndianFloat(isLittleEndian, os, pixel.L.c[2]);
					osWriteLittleEndianFloat(isLittleEndian, os, pixel.alpha);
					osWriteLittleEndianFloat(isLittleEndian, os, pixel.weightSum);
				}
			}

			if (clearBuffers) {
				// Dade - reset the rendering buffer
				buffer->Clear();
			}
		}

		if (clearBuffers) {
			// Dade - reset the rendering buffer
			bufferGroup.numberOfSamples = 0;
		}
	}

	if (!os.good()) {
		luxError(LUX_SYSTEM, LUX_SEVERE, "Error while preparing film data for transmission");
		return;
	}

	filtering_streambuf<input> in;
	in.push(gzip_compressor(9));
	in.push(os);
	std::streamsize size = boost::iostreams::copy(in, stream);
	if (!stream.good()) {
		luxError(LUX_SYSTEM, LUX_SEVERE, "Error while transmitting film");
		return;
	}

	ss.str("");
	ss << "Film transmission done (" << (size / 1024.0f) << " Kbytes sent)";
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
}

float FlexImageFilm::UpdateFilm(std::basic_istream<char> &stream) {
	const bool isLittleEndian = osIsLittleEndian();

	filtering_stream<input> in;
	in.push(gzip_decompressor());
	in.push(stream);

	std::stringstream ss;
	ss << "Receiving film (little endian=" << (isLittleEndian ? "true" : "false") << ")";
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	// Read header
	FlmHeader header;
	if (!header.Read(in, isLittleEndian, this))
		return 0.f;

	// Read buffer groups
	vector<float> bufferGroupNumSamples(bufferGroups.size());
	vector<BlockedArray<Pixel>*> tmpPixelArrays(bufferGroups.size() * bufferConfigs.size());
	for (u_int i = 0; i < bufferGroups.size(); i++) {
		double numberOfSamples;
		numberOfSamples = osReadLittleEndianDouble(isLittleEndian, in);
		if (!in.good())
			break;
		bufferGroupNumSamples[i] = numberOfSamples;

		// Read buffers
		for(u_int j = 0; j < bufferConfigs.size(); ++j) {
			const Buffer* localBuffer = bufferGroups[i].getBuffer(j);
			// Read pixels
			BlockedArray<Pixel> *tmpPixelArr = new BlockedArray<Pixel>(
				localBuffer->xPixelCount, localBuffer->yPixelCount);
			tmpPixelArrays[i*bufferConfigs.size() + j] = tmpPixelArr;
			for (int y = 0; y < tmpPixelArr->vSize(); ++y) {
				for (int x = 0; x < tmpPixelArr->uSize(); ++x) {
					Pixel &pixel = (*tmpPixelArr)(x, y);
					pixel.L.c[0] = osReadLittleEndianFloat(isLittleEndian, in);
					pixel.L.c[1] = osReadLittleEndianFloat(isLittleEndian, in);
					pixel.L.c[2] = osReadLittleEndianFloat(isLittleEndian, in);
					pixel.alpha = osReadLittleEndianFloat(isLittleEndian, in);
					pixel.weightSum = osReadLittleEndianFloat(isLittleEndian, in);
				}
			}
			if (!in.good())
				break;
		}
		if (!in.good())
			break;
	}

	// Dade - check for errors
	float totNumberOfSamples = 0.f;
	if (in.good()) {
		// Update parameters
		for (vector<FlmParameter>::iterator it = header.params.begin(); it != header.params.end(); ++it)
			it->Set(this);

		// Dade - add all received data
		for (u_int i = 0; i < bufferGroups.size(); ++i) {
			BufferGroup &currentGroup = bufferGroups[i];
			for (u_int j = 0; j < bufferConfigs.size(); ++j) {
				const BlockedArray<Pixel> *receivedPixels = tmpPixelArrays[ i * bufferConfigs.size() + j ];
				Buffer *buffer = currentGroup.getBuffer(j);

				for (int y = 0; y < buffer->yPixelCount; ++y) {
					for (int x = 0; x < buffer->xPixelCount; ++x) {
						const Pixel &pixel = (*receivedPixels)(x, y);
						Pixel &pixelResult = (*buffer->pixels)(x, y);

						pixelResult.L.c[0] += pixel.L.c[0];
						pixelResult.L.c[1] += pixel.L.c[1];
						pixelResult.L.c[2] += pixel.L.c[2];
						pixelResult.alpha += pixel.alpha;
						pixelResult.weightSum += pixel.weightSum;
					}
				}
			}

			currentGroup.numberOfSamples += bufferGroupNumSamples[i];
			// Check if we have enough samples per pixel
			if ((haltSamplePerPixel > 0) &&
				(currentGroup.numberOfSamples >= haltSamplePerPixel * samplePerPass))
				enoughSamplePerPixel = true;
			totNumberOfSamples += bufferGroupNumSamples[i];
		}

		if (scene != NULL)
			scene->numberOfSamplesFromNetwork += totNumberOfSamples;

		ss.str("");
		ss << "Received film with " << totNumberOfSamples << " samples";
		luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
	} else
		luxError(LUX_SYSTEM, LUX_ERROR, "IO error while receiving film buffers");

	// Clean up
	for (u_int i = 0; i < tmpPixelArrays.size(); ++i)
		delete tmpPixelArrays[i];

	return totNumberOfSamples;
}

void FlexImageFilm::GetColorspaceParam(const ParamSet &params, const string name, float values[2]) {
	int i;
	const float *v = params.FindFloat(name, &i);
	if (v && i == 2) {
		values[0] = v[0];
		values[1] = v[1];
	}
}

// params / creation
Film* FlexImageFilm::CreateFilm(const ParamSet &params, Filter *filter)
{
	// General
	bool premultiplyAlpha = params.FindOneBool("premultiplyalpha", false);

	int xres = params.FindOneInt("xresolution", 800);
	int yres = params.FindOneInt("yresolution", 600);

	float crop[4] = { 0, 1, 0, 1 };
	int cwi;
	const float *cr = params.FindFloat("cropwindow", &cwi);
	if (cr && cwi == 4) {
		crop[0] = Clamp(min(cr[0], cr[1]), 0.f, 1.f);
		crop[1] = Clamp(max(cr[0], cr[1]), 0.f, 1.f);
		crop[2] = Clamp(min(cr[2], cr[3]), 0.f, 1.f);
		crop[3] = Clamp(max(cr[2], cr[3]), 0.f, 1.f);
	}

	// Output Image File Formats
	string clampMethodString = params.FindOneString("ldr_clamp_method", "lum");
	int clampMethod = 0;
	if (clampMethodString == "lum")
		clampMethod = 0;
	else if (clampMethodString == "hue")
		clampMethod = 1;
	else if (clampMethodString == "cut")
		clampMethod = 2;
	else {
		std::stringstream ss;
		ss << "LDR clamping method  '" << clampMethodString << "' unknown. Using \"lum\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
	}

	// OpenEXR
	bool w_EXR = params.FindOneBool("write_exr", false);

	OutputChannels w_EXR_channels = RGB;
	string w_EXR_channelsStr = params.FindOneString("write_exr_channels", "RGB");
	if (w_EXR_channelsStr == "Y") w_EXR_channels = Y;
	else if (w_EXR_channelsStr == "YA") w_EXR_channels = YA;
	else if (w_EXR_channelsStr == "RGB") w_EXR_channels = RGB;
	else if (w_EXR_channelsStr == "RGBA") w_EXR_channels = RGBA;
	else {
		std::stringstream ss;
		ss << "OpenEXR Output Channels  '" << w_EXR_channelsStr << "' unknown. Using \"RGB\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		w_EXR_channels = RGB;
	}

	bool w_EXR_halftype = params.FindOneBool("write_exr_halftype", true);

	int w_EXR_compressiontype = 1;
	string w_EXR_compressiontypeStr = params.FindOneString("write_exr_compressiontype", "PIZ (lossless)");
	if (w_EXR_compressiontypeStr == "RLE (lossless)") w_EXR_compressiontype = 0;
	else if (w_EXR_compressiontypeStr == "PIZ (lossless)") w_EXR_compressiontype = 1;
	else if (w_EXR_compressiontypeStr == "ZIP (lossless)") w_EXR_compressiontype = 2;
	else if (w_EXR_compressiontypeStr == "Pxr24 (lossy)") w_EXR_compressiontype = 3;
	else if (w_EXR_compressiontypeStr == "None") w_EXR_compressiontype = 4;
	else {
		std::stringstream ss;
		ss << "OpenEXR Compression Type '" << w_EXR_compressiontypeStr << "' unknown. Using \"PIZ (lossless)\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		w_EXR_compressiontype = 1;
	}

	bool w_EXR_applyimaging = params.FindOneBool("write_exr_applyimaging", true);
	bool w_EXR_gamutclamp = params.FindOneBool("write_exr_gamutclamp", true);

	bool w_EXR_ZBuf = params.FindOneBool("write_exr_ZBuf", false);

	ZBufNormalization w_EXR_ZBuf_normalizationtype = None;
	string w_EXR_ZBuf_normalizationtypeStr = params.FindOneString("write_exr_zbuf_normalizationtype", "None");
	if (w_EXR_ZBuf_normalizationtypeStr == "None") w_EXR_ZBuf_normalizationtype = None;
	else if (w_EXR_ZBuf_normalizationtypeStr == "Camera Start/End clip") w_EXR_ZBuf_normalizationtype = CameraStartEnd;
	else if (w_EXR_ZBuf_normalizationtypeStr == "Min/Max") w_EXR_ZBuf_normalizationtype = MinMax;
	else {
		std::stringstream ss;
		ss << "OpenEXR ZBuf Normalization Type '" << w_EXR_ZBuf_normalizationtypeStr << "' unknown. Using \"None\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		w_EXR_ZBuf_normalizationtype = None;
	}

	// Portable Network Graphics (PNG)
	bool w_PNG = params.FindOneBool("write_png", true);

	OutputChannels w_PNG_channels = RGB;
	string w_PNG_channelsStr = params.FindOneString("write_png_channels", "RGB");
	if (w_PNG_channelsStr == "Y") w_PNG_channels = Y;
	else if (w_PNG_channelsStr == "YA") w_PNG_channels = YA;
	else if (w_PNG_channelsStr == "RGB") w_PNG_channels = RGB;
	else if (w_PNG_channelsStr == "RGBA") w_PNG_channels = RGBA;
	else {
		std::stringstream ss;
		ss << "PNG Output Channels  '" << w_PNG_channelsStr << "' unknown. Using \"RGB\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		w_PNG_channels = RGB;
	}

	bool w_PNG_16bit = params.FindOneBool("write_png_16bit", false);
	bool w_PNG_gamutclamp = params.FindOneBool("write_png_gamutclamp", true);

	bool w_PNG_ZBuf = params.FindOneBool("write_png_ZBuf", false);

	ZBufNormalization w_PNG_ZBuf_normalizationtype = MinMax;
	string w_PNG_ZBuf_normalizationtypeStr = params.FindOneString("write_png_zbuf_normalizationtype", "Min/Max");
	if (w_PNG_ZBuf_normalizationtypeStr == "None") w_PNG_ZBuf_normalizationtype = None;
	else if (w_PNG_ZBuf_normalizationtypeStr == "Camera Start/End clip") w_PNG_ZBuf_normalizationtype = CameraStartEnd;
	else if (w_PNG_ZBuf_normalizationtypeStr == "Min/Max") w_PNG_ZBuf_normalizationtype = MinMax;
	else {
		std::stringstream ss;
		ss << "PNG ZBuf Normalization Type '" << w_PNG_ZBuf_normalizationtypeStr << "' unknown. Using \"Min/Max\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		w_PNG_ZBuf_normalizationtype = MinMax;
	}

	// TGA
	bool w_TGA = params.FindOneBool("write_tga", false);

	OutputChannels w_TGA_channels = RGB;
	string w_TGA_channelsStr = params.FindOneString("write_tga_channels", "RGB");
	if (w_TGA_channelsStr == "Y") w_TGA_channels = Y;
	else if (w_TGA_channelsStr == "RGB") w_TGA_channels = RGB;
	else if (w_TGA_channelsStr == "RGBA") w_TGA_channels = RGBA;
	else {
		std::stringstream ss;
		ss << "TGA Output Channels  '" << w_TGA_channelsStr << "' unknown. Using \"RGB\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		w_TGA_channels = RGB;
	}

	bool w_TGA_gamutclamp = params.FindOneBool("write_tga_gamutclamp", true);

	bool w_TGA_ZBuf = params.FindOneBool("write_tga_ZBuf", false);

	ZBufNormalization w_TGA_ZBuf_normalizationtype = MinMax;
	string w_TGA_ZBuf_normalizationtypeStr = params.FindOneString("write_tga_zbuf_normalizationtype", "Min/Max");
	if (w_TGA_ZBuf_normalizationtypeStr == "None") w_TGA_ZBuf_normalizationtype = None;
	else if (w_TGA_ZBuf_normalizationtypeStr == "Camera Start/End clip") w_TGA_ZBuf_normalizationtype = CameraStartEnd;
	else if (w_TGA_ZBuf_normalizationtypeStr == "Min/Max") w_TGA_ZBuf_normalizationtype = MinMax;
	else {
		std::stringstream ss;
		ss << "TGA ZBuf Normalization Type '" << w_TGA_ZBuf_normalizationtypeStr << "' unknown. Using \"Min/Max\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		w_TGA_ZBuf_normalizationtype = MinMax;
	}


	// Output FILM / FLM 
    bool w_resume_FLM = params.FindOneBool("write_resume_flm", false);
	bool restart_resume_FLM = params.FindOneBool("restart_resume_flm", false);

	// output filenames
	string filename = params.FindOneString("filename", "luxout");

	// intervals
	int writeInterval = params.FindOneInt("writeinterval", 60);
	int displayInterval = params.FindOneInt("displayinterval", 12);

	// Rejection mechanism
	int reject_warmup = params.FindOneInt("reject_warmup", 64); // minimum samples/px before rejecting

	// Debugging mode (display erratic sample values and disable rejection mechanism)
	bool debug_mode = params.FindOneBool("debug", false);

	int haltspp = params.FindOneInt("haltspp", -1);

	// Color space primaries and white point
	// default is SMPTE
	float red[2] = {0.63f, 0.34f};
	GetColorspaceParam(params, "colorspace_red", red);

	float green[2] = {0.31f, 0.595f};
	GetColorspaceParam(params, "colorspace_green", green);

	float blue[2] = {0.155f, 0.07f};
	GetColorspaceParam(params, "colorspace_blue", blue);

	float white[2] = {0.314275f, 0.329411f};
	GetColorspaceParam(params, "colorspace_white", white);

	// Tonemapping
	int s_TonemapKernel = 0;
	string tmkernelStr = params.FindOneString("tonemapkernel", "reinhard");
	if (tmkernelStr == "reinhard") s_TonemapKernel = 0;
	else if (tmkernelStr == "linear") s_TonemapKernel = 1;
	else if (tmkernelStr == "contrast") s_TonemapKernel = 2;
	else if (tmkernelStr == "maxwhite") s_TonemapKernel = 3;
	else {
		std::stringstream ss;
		ss << "Tonemap kernel  '" << tmkernelStr << "' unknown. Using \"reinhard\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		s_TonemapKernel = 0;
	}

	float s_ReinhardPreScale = params.FindOneFloat("reinhard_prescale", 1.f);
	float s_ReinhardPostScale = params.FindOneFloat("reinhard_postscale", 1.f);
	float s_ReinhardBurn = params.FindOneFloat("reinhard_burn", 6.f);
	float s_LinearSensitivity = params.FindOneFloat("linear_sensitivity", 50.f);
	float s_LinearExposure = params.FindOneFloat("linear_exposure", 1.f);
	float s_LinearFStop = params.FindOneFloat("linear_fstop", 2.8f);
	float s_LinearGamma = params.FindOneFloat("linear_gamma", 1.0f);
	float s_ContrastYwa = params.FindOneFloat("contrast_ywa", 1.f);
	float s_Gamma = params.FindOneFloat("gamma", 2.2f);

	return new FlexImageFilm(xres, yres, filter, crop,
		filename, premultiplyAlpha, writeInterval, displayInterval,
		clampMethod, w_EXR, w_EXR_channels, w_EXR_halftype, w_EXR_compressiontype, w_EXR_applyimaging, w_EXR_gamutclamp, w_EXR_ZBuf, w_EXR_ZBuf_normalizationtype,
		w_PNG, w_PNG_channels, w_PNG_16bit, w_PNG_gamutclamp, w_PNG_ZBuf, w_PNG_ZBuf_normalizationtype,
		w_TGA, w_TGA_channels, w_TGA_gamutclamp, w_TGA_ZBuf, w_TGA_ZBuf_normalizationtype, 
		w_resume_FLM, restart_resume_FLM, haltspp,
		s_TonemapKernel, s_ReinhardPreScale, s_ReinhardPostScale, s_ReinhardBurn, s_LinearSensitivity,
		s_LinearExposure, s_LinearFStop, s_LinearGamma, s_ContrastYwa, s_Gamma,
		red, green, blue, white, reject_warmup, debug_mode);
}


Film *FlexImageFilm::CreateFilmFromFLM(const string& flmFileName) {

	// NOTE - lordcrc - FlexImageFilm takes ownership of filter
	ParamSet dummyParams;
	Filter *dummyFilter = MakeFilter("box", dummyParams);

	// Read the FLM header
	std::ifstream stream(flmFileName.c_str(), std::ios_base::in | std::ios_base::binary);
	filtering_stream<input> in;
	in.push(gzip_decompressor());
	in.push(stream);
	const bool isLittleEndian = osIsLittleEndian();
	FlmHeader header;
	bool headerOk = header.Read(in, isLittleEndian, NULL);
	stream.close();
	if (!headerOk)
		return NULL;

	// Create the default film
	const string filename = flmFileName.substr(0, flmFileName.length() - 4); // remove .flm extention
	static const bool boolTrue = true;
	static const bool boolFalse = false;
	ParamSet filmParams;
	filmParams.AddString("filename", &filename );
	filmParams.AddInt("xresolution", &header.xResolution);
	filmParams.AddInt("yresolution", &header.yResolution);
	filmParams.AddBool("write_resume_flm", &boolTrue);
	filmParams.AddBool("restart_resume_flm", &boolFalse);
	filmParams.AddBool("write_exr", &boolFalse);
	filmParams.AddBool("write_exr_ZBuf", &boolFalse);
	filmParams.AddBool("write_png", &boolFalse);
	filmParams.AddBool("write_png_ZBuf", &boolFalse);
	filmParams.AddBool("write_tga", &boolFalse);
	filmParams.AddBool("write_tga_ZBuf", &boolFalse);
	Film *film = FlexImageFilm::CreateFilm(filmParams, dummyFilter);
	
	// Create the buffers (also loads the FLM file)
	for (u_int i = 0; i < header.numBufferConfigs; ++i)
		film->RequestBuffer(BufferType(header.bufferTypes[i]), BUF_FRAMEBUFFER, "");

	vector<string> bufferGroups;
	for (u_int i = 0; i < header.numBufferGroups; ++i) {
		std::stringstream ss;
		ss << "lightgroup #" << (i + 1);
		bufferGroups.push_back(ss.str());
	}
	film->RequestBufferGroups(bufferGroups);
	film->CreateBuffers();

	return film;
}

static DynamicLoader::RegisterFilm<FlexImageFilm> r1("fleximage");
static DynamicLoader::RegisterFilm<FlexImageFilm> r2("multiimage");
