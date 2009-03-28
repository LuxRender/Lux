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

/*
 * FlexImage Film class
 *
 */

#include "fleximage.h"
#include "error.h"
#include "scene.h"		// for Scene
#include "filter.h"
#include "exrio.h"
#include "igiio.h"
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
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
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
	const string &filename1, bool premult, int wI, int dI,
	bool w_tonemapped_EXR, bool w_untonemapped_EXR, bool w_tonemapped_IGI,
	bool w_untonemapped_IGI, bool w_tonemapped_TGA, bool w_resume_FLM, bool restart_resume_FLM, int haltspp,
	int p_TonemapKernel, float p_ReinhardPreScale, float p_ReinhardPostScale,
	float p_ReinhardBurn, float p_LinearSensitivity, float p_LinearExposure, float p_LinearFStop, float p_LinearGamma,
	float p_ContrastYwa, float p_Gamma,
	const float cs_red[2], const float cs_green[2], const float cs_blue[2], const float whitepoint[2],
	int reject_warmup, bool debugmode) :
	Film(xres, yres, haltspp), filter(filt), writeInterval(wI), displayInterval(dI),
	filename(filename1), premultiplyAlpha(premult), buffersInited(false),
	writeTmExr(w_tonemapped_EXR), writeUtmExr(w_untonemapped_EXR), writeTmIgi(w_tonemapped_IGI),
	writeUtmIgi(w_untonemapped_IGI), writeTmTga(w_tonemapped_TGA), writeResumeFlm(w_resume_FLM), restartResumeFlm(restart_resume_FLM),
	framebuffer(NULL), debug_mode(debugmode),
	colorSpace(cs_red[0], cs_red[1], cs_green[0], cs_green[1], cs_blue[0], cs_blue[1], whitepoint[0], whitepoint[1], 1.f)
{
	// Compute film image extent
	memcpy(cropWindow, crop, 4 * sizeof(float));
	xPixelStart = Ceil2Int(xResolution * cropWindow[0]);
	xPixelCount = max(1, Ceil2Int(xResolution * cropWindow[1]) - xPixelStart);
	yPixelStart = Ceil2Int(yResolution * cropWindow[2]);
	yPixelCount = max(1, Ceil2Int(yResolution * cropWindow[3]) - yPixelStart);

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
	m_HaveBloomImage = false;
	m_BloomRadius = d_BloomRadius = 0.07f;
	m_BloomWeight = d_BloomWeight = 0.25f;

	m_VignettingEnabled = d_VignettingEnabled = false;
	m_VignettingScale = d_VignettingScale = 0.4f;

	m_AberrationEnabled = d_AberrationEnabled = false;
	m_AberrationAmount = d_AberrationAmount = 0.005f;

	m_GlareEnabled = d_GlareEnabled = false;
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
	reject_warmup_samples = ((double)xResolution * (double)yResolution) * reject_warmup;

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

	maxY = 0.f;
	warmupSamples = 0;
	warmupComplete = false;
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
			if(value != 0.f)
				m_BloomUpdateLayer = true;
			else
				m_BloomUpdateLayer = false;
			break;

		case LUX_FILM_BLOOMRADIUS:
			 m_BloomRadius = value;
			break;
		case LUX_FILM_BLOOMWEIGHT:
			 m_BloomWeight = value;
			break;

		case LUX_FILM_VIGNETTING_ENABLED:
			if(value != 0.f)
				m_VignettingEnabled = true;
			else
				m_VignettingEnabled = false;
			break;
		case LUX_FILM_VIGNETTING_SCALE:
			 m_VignettingScale = value;
			break;

		case LUX_FILM_ABERRATION_ENABLED:
			if(value != 0.f)
				m_AberrationEnabled = true;
			else
				m_AberrationEnabled = false;
			break;
		case LUX_FILM_ABERRATION_AMOUNT:
			 m_AberrationAmount = value;
			break;

		case LUX_FILM_GLARE_ENABLED:
			if(value != 0.f)
				m_GlareEnabled = true;
			else
				m_GlareEnabled = false;
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
			if(value != 0.f)
				m_HistogramEnabled = true;
			else
				m_HistogramEnabled = false;
			break;

		case LUX_FILM_NOISE_CHIU_ENABLED:
			m_chiuParams.enabled = value != 0.0;
			break;
		case LUX_FILM_NOISE_CHIU_RADIUS:
			m_chiuParams.radius = value;
			break;
		case LUX_FILM_NOISE_CHIU_INCLUDECENTER:
			m_chiuParams.includecenter = value != 0.0;
			break;

		case LUX_FILM_NOISE_GREYC_ENABLED:
			if(value != 0.f)
				m_GREYCStorationParams.enabled = true;
			else
				m_GREYCStorationParams.enabled = false;
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
			if(value != 0.f)
				m_GREYCStorationParams.fast_approx = true;
			else
				m_GREYCStorationParams.fast_approx = false;
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

		case LUX_FILM_GLARE_ENABLED:
			return m_GlareEnabled;
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

		case LUX_FILM_GLARE_ENABLED:
			return d_GlareEnabled;
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

    // Dade - check if we have to resume a rendering and restore the buffers
    if(writeResumeFlm && !restartResumeFlm) {
        // Dade - check if the film file exists
        string fname = filename+".flm";
		std::ifstream ifs(fname.c_str(), std::ios_base::in | std::ios_base::binary);

        if(ifs.good()) {
            // Dade - read the data
            luxError(LUX_NOERROR, LUX_INFO, (std::string("Reading film status from file ")+fname).c_str());
            UpdateFilm(NULL, ifs);
        }

        ifs.close();
    }
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
		bufferGroups[index].scale *= factor / (factor.y() * white);
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
	if (xyz.IsNaN() || xyz.y() < -1e-5f || isinf(xyz.y())) {
		if(debug_mode) {
			std::stringstream ss;
			ss << "Out of bound intensity in FlexImageFilm::AddSample: "
			   << xyz.y() << ", sample discarded";
			luxError(LUX_LIMIT, LUX_WARNING, ss.str().c_str());
		}
		return;
	}

	// Reject samples higher than max y() after warmup period
	if (warmupComplete) {
		if (xyz.y() > maxY)
			return;
	} else {
		if (debug_mode) {
			maxY = INFINITY;
			warmupComplete = true;
		} else {
		 	maxY = max(maxY, xyz.y());
			++warmupSamples;
		 	if (warmupSamples >= reject_warmup_samples)
				warmupComplete = true;
		}
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

	for (int y = max(y0, yPixelStart); y <= min(y1, yPixelStart + yPixelCount - 1); ++y) {
		for (int x = max(x0, xPixelStart); x <= min(x1, xPixelStart + xPixelCount - 1); ++x) {
			// Evaluate filter value at $(x,y)$ pixel
			const int offset = ify[y-y0]*FILTER_TABLE_SIZE + ifx[x-x0];
			const float filterWt = filterTable[offset] / filterNorm;
			// Update pixel values with filtered sample contribution
			buffer->Add(x - xPixelStart,y - yPixelStart,
				xyz, alpha, filterWt * weight);
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

void FlexImageFilm::WriteImage2(ImageType type, vector<Color> &color, vector<float> &alpha, string postfix)
{
	// Construct ColorSystem from values
	colorSpace = ColorSystem(m_RGB_X_Red, m_RGB_Y_Red,
		m_RGB_X_Green, m_RGB_Y_Green,
		m_RGB_X_Blue, m_RGB_Y_Blue,
		m_RGB_X_White, m_RGB_Y_White, 1.f);

	if (type & IMAGE_FILEOUTPUT) {
		// write out untonemapped EXR
		if (writeUtmExr) {
			// convert to rgb
			const u_int nPix = xPixelCount * yPixelCount;
			vector<Color> rgbColor(nPix);
			for ( u_int i = 0; i < nPix; i++ )
				rgbColor[i] = colorSpace.ToRGBConstrained( XYZColor(color[i].c) );

			WriteEXRImage(rgbColor, alpha, filename + postfix + "_untonemapped.exr");
		}
		// write out untonemapped IGI
		if (writeUtmIgi)
			WriteIGIImage(color, alpha, filename + postfix + "_untonemapped.igi"); // TODO add samples

		// Dade - save the current status of the film if required
		if (writeResumeFlm)
			WriteResumeFilm(filename + ".flm");
	}

	// Dade - check if I have to run ApplyImagingPipeline
	if (((type & IMAGE_FRAMEBUFFER) && framebuffer) ||
		((type & IMAGE_FILEOUTPUT) && (writeTmExr || writeTmIgi || writeTmTga))) {
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

		// Apply chosen tonemapper
		ApplyImagingPipeline(color, xPixelCount, yPixelCount, m_GREYCStorationParams, m_chiuParams,
			colorSpace, m_histogram, m_HistogramEnabled, m_HaveBloomImage, m_bloomImage, m_BloomUpdateLayer,
			m_BloomRadius, m_BloomWeight, m_VignettingEnabled, m_VignettingScale, m_AberrationEnabled, m_AberrationAmount,
			m_GlareEnabled, m_GlareAmount, m_GlareRadius, m_GlareBlades,
			tmkernel.c_str(), &toneParams, m_Gamma, 0.);

		// Disable further bloom layer updates if used.
		m_BloomUpdateLayer = false;

		if (type & IMAGE_FILEOUTPUT) {
			// write out tonemapped EXR
			if (writeTmExr)
				WriteEXRImage(color, alpha, filename + postfix + ".exr");

			// write out tonemapped IGI
			if (writeTmIgi) //TODO color should be XYZ values
				WriteIGIImage(color, alpha, filename + postfix + ".igi"); // TODO add samples
		}

		// Output to low dynamic range formats
		if ((type & IMAGE_FILEOUTPUT) || (type & IMAGE_FRAMEBUFFER)) {
			// Clamp too high values
			const u_int nPix = xPixelCount * yPixelCount;
			for (u_int i = 0; i < nPix; ++i)
				color[i] = colorSpace.Limit(color[i]);

			// write out tonemapped TGA
			if (writeTmTga)
				WriteTGAImage(color, alpha, filename + postfix + ".tga");
			// Copy to framebuffer pixels
			if ((type & IMAGE_FRAMEBUFFER) && framebuffer) {
				for (u_int i = 0; i < nPix; i++) {
					framebuffer[3 * i] = static_cast<unsigned char>(Clamp(256 * color[i].c[0], 0.f, 255.f));
					framebuffer[3 * i + 1] = static_cast<unsigned char>(Clamp(256 * color[i].c[1], 0.f, 255.f));
					framebuffer[3 * i + 2] = static_cast<unsigned char>(Clamp(256 * color[i].c[2], 0.f, 255.f));
				}
			}
		}
	}
}

void FlexImageFilm::ScaleOutput(vector<Color> &pixel, vector<float> &alpha, float *scale)
{
	int offset = 0;
	for (int y = 0; y < yPixelCount; ++y) {
		for (int x = 0; x < xPixelCount; ++x,++offset) {
			pixel[offset] *= scale[offset];
			alpha[offset] *= scale[offset];
		}
	}
}
void FlexImageFilm::WriteImage(ImageType type)
{
	const int nPix = xPixelCount * yPixelCount;
	vector<Color> pixels(nPix);
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

	Color p;
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

void FlexImageFilm::WriteTGAImage(vector<Color> &rgb, vector<float> &alpha, const string &filename)
{
	luxError(LUX_NOERROR, LUX_INFO, (std::string("Writing Tonemapped TGA image to file ")+filename).c_str());

	// Open file
	FILE* tgaFile = fopen(filename.c_str(),"wb");
	if (!tgaFile) {
		std::stringstream ss;
		ss<< "Cannot open file '"<<filename<<"' for output";
		luxError(LUX_SYSTEM, LUX_SEVERE, ss.str().c_str());
		return;
	}

	// write the header
	// make sure its platform independent of little endian and big endian
	char header[18];
	memset(header, 0, sizeof(char) * 18);

	header[2] = 2;							// set the data type of the targa (2 = uncompressed)
	short xResShort = xResolution;			// set the resolution and make sure the bytes are in the right order
	header[13] = (char) (xResShort >> 8);
	header[12] = xResShort & 0xFF;
	short yResShort = yResolution;
	header[15] = (char) (yResShort >> 8);
	header[14] = yResShort & 0xFF;
	header[16] = 32;						// set the bitdepth

	// put the header data into the file
	for (int i = 0; i < 18; ++i)
		fputc(header[i], tgaFile);

	// write the bytes of data out
	for (int i = yPixelCount - 1;  i >= 0 ; --i) {
		for (int j = 0; j < xPixelCount; ++j) {
			fputc(static_cast<unsigned char>(Clamp(256 * rgb[i * xPixelCount + j].c[2], 0.f, 255.f)), tgaFile);
			fputc(static_cast<unsigned char>(Clamp(256 * rgb[i * xPixelCount + j].c[1], 0.f, 255.f)), tgaFile);
			fputc(static_cast<unsigned char>(Clamp(256 * rgb[i * xPixelCount + j].c[0], 0.f, 255.f)), tgaFile);
			fputc(static_cast<unsigned char>(Clamp(256 * alpha[(i * xPixelCount + j)], 0.f, 255.f)), tgaFile);
		}
	}

	fclose(tgaFile);
}

void FlexImageFilm::WriteEXRImage(vector<Color> &rgb, vector<float> &alpha, const string &filename)
{
	// Write OpenEXR RGBA image
	luxError(LUX_NOERROR, LUX_INFO, (std::string("Writing OpenEXR image to file ")+filename).c_str());
	WriteRGBAImageFloat(filename, rgb, alpha,
		xPixelCount, yPixelCount,
		xResolution, yResolution,
		xPixelStart, yPixelStart);
}

void FlexImageFilm::WriteIGIImage(vector<Color> &rgb, vector<float> &alpha, const string &filename)
{
	// Write IGI image
	luxError(LUX_NOERROR, LUX_INFO, (std::string("Writing IGI image to file ")+filename).c_str());
	WriteIgiImage(filename, rgb, alpha,
		xPixelCount, yPixelCount,
		xResolution, yResolution,
		xPixelStart, yPixelStart);
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
 *   #buffer_groups                - int   - the number of lightgroups
 *   #buffer_configs               - int   - the number of buffers per light group
 *   for i in 1:#buffer_configs
 *     buffer_type                 - int   - the type of the i'th buffer
 *   #parameters                   - int   - the number of stored parameters
 *   for i in 1:#parameters
 *     param_id                    - int   - the id of the i'th parameter
 *     param_index                 - int   - the index of the i'th parameter
 *     param_value                 - float - the value of the i'th parameter
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

struct FlmParameter {
	FlmParameter() {}
	FlmParameter(int aId, int aIndex, float aValue) : id(aId), index(aIndex), value(aValue) {}
	FlmParameter(FlexImageFilm *aFilm, luxComponentParameters aParam, int aIndex) {
		id = (int)aParam;
		index = aIndex;
		value = (float)aFilm->GetParameterValue(aParam, aIndex);
	}

	bool IsValid(FlexImageFilm *aFilm) {
		return true; //TODO verify the parameter
	}

	void Set(FlexImageFilm *aFilm) {
		aFilm->SetParameterValue(luxComponentParameters(id), (double) value, index);
	}

	int id;
	int index;
	float value;
};

struct FlmHeader {
	FlmHeader() {}

	int magic_number;
	int version_number;
	int x_resolution;
	int y_resolution;
	int num_buffer_groups;
	int num_buffer_configs;
	vector<int> buffer_types;
	int num_params;
	vector<FlmParameter> params;
};

bool ReadFLMHeader( filtering_stream<input> &in, bool isLittleEndian, FlmHeader *header, FlexImageFilm *film ) {
	// Read and verify magic number and version
	int magicNumber;
	osReadLittleEndianInt(isLittleEndian, in, &magicNumber);
	if (!in.good()) {
		luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
		return false;
	}
	if(magicNumber != FLM_MAGIC_NUMBER) {
		std::stringstream ss;
		ss << "Invalid FLM magic number (expected=" << FLM_MAGIC_NUMBER 
			<< ", received=" << magicNumber << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	header->magic_number = magicNumber;
	int version;
	osReadLittleEndianInt(isLittleEndian, in, &version);
	if (!in.good()) {
		luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
		return false;
	}
	if(version != FLM_VERSION) {
		std::stringstream ss;
		ss << "Invalid FLM version (expected=" << FLM_VERSION 
			<< ", received=" << version << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	header->version_number = version;
	// Read and verify the buffer resolution
	int xRes, yRes;
	osReadLittleEndianInt(isLittleEndian, in, &xRes);
	osReadLittleEndianInt(isLittleEndian, in, &yRes);
	if(xRes <= 0 || yRes <= 0 ) {
		std::stringstream ss;
		ss << "Invalid resolution";
		ss << " (expected positive resolution";
		ss << ", received=" << xRes << "x" << yRes << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	if(film != NULL && (xRes != film->GetXPixelCount() || yRes != film->GetYPixelCount() ) ) {
		std::stringstream ss;
		ss << "Invalid resolution";
		ss << " (expected=" << film->GetXPixelCount() << "x" << film->GetYPixelCount();
		ss << ", received=" << xRes << "x" << yRes << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	header->x_resolution = xRes;
	header->y_resolution = yRes;
	// Read and verify #buffer groups and buffer configs
    int nBufferGroups;
    osReadLittleEndianInt(isLittleEndian, in, &nBufferGroups);
	if (!in.good()) {
		luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
		return false;
	}
	if(nBufferGroups <= 0) {
		std::stringstream ss;
		ss << "Invalid number of buffer groups (expected positive number"
			<< ", received=" << nBufferGroups << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	if(film != NULL && nBufferGroups != (int)film->GetNumBufferGroups()) {
		std::stringstream ss;
		ss << "Invalid number of buffer groups (expected=" << (int)film->GetNumBufferGroups() 
			<< ", received=" << nBufferGroups << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	header->num_buffer_groups = nBufferGroups;
	int nBuffers;
	osReadLittleEndianInt(isLittleEndian, in, &nBuffers);
	if (!in.good()) {
		luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
		return false;
	}
	if(nBuffers <= 0) {
		std::stringstream ss;
		ss << "Invalid number of buffers (expected positive number"
			<< ", received=" << nBuffers << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	if(film != NULL && nBuffers != (int)film->GetNumBufferConfigs()) {
		std::stringstream ss;
		ss << "Invalid number of buffers (expected=" << (int)film->GetNumBufferConfigs()
			<< ", received=" << nBuffers << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	header->num_buffer_configs = nBuffers;
	for( int i = 0; i < nBuffers; i++) {
		int type;
		osReadLittleEndianInt(isLittleEndian, in, &type);
		if (!in.good()) {
			luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
			return false;
		}
		if(type < 0 || type >= (int)NUM_OF_BUFFER_TYPES) {
			std::stringstream ss;
			ss << "Invalid buffer type for buffer " << i << "(expected number in [0," << (int)NUM_OF_BUFFER_TYPES << "["
			<< ", received=" << nBuffers << ")";
			luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
			return false;
		}
		if(film != NULL && type != (int)film->GetBufferConfig(i).type) {
			std::stringstream ss;
			ss << "Invalid buffer type for buffer " << i << " (expected=" << (int)film->GetBufferConfig(i).type
				<< ", received=" << type << ")";
			luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
			return false;
		}
		header->buffer_types.push_back( type );
	}
	// Read parameters
	int nParams;
	osReadLittleEndianInt(isLittleEndian, in, &nParams);
	if (!in.good()) {
		luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
		return false;
	}
	if(nParams < 0) {
		std::stringstream ss;
		ss << "Invalid number parameters (expected positive number"
			<< ", received=" << nParams << ")";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return false;
	}
	header->num_params = nParams;
	header->params.reserve(nParams);
	for(int i = 0; i < nParams; i++) {
		FlmParameter param;
		osReadLittleEndianInt(isLittleEndian, in, &param.id);
		osReadLittleEndianInt(isLittleEndian, in, &param.index);
		osReadLittleEndianFloat(isLittleEndian, in, &param.value);
		if (!in.good()) {
			luxError(LUX_SYSTEM, LUX_ERROR, "Error while receiving film");
			return false;
		}
		if(film != NULL && !param.IsValid(film)) {
			std::stringstream ss;
			ss << "Invalid parameter (id=" << param.id << ", index=" << param.index << ", value=" << param.value << ")";
			luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
			return false;
		}
		header->params.push_back(param);
	}
	return true;
}

void WriteFLMHeader( std::basic_ostream<char> &os, bool isLittleEndian, const FlmHeader &header ) {
	// Write magic number and version
	osWriteLittleEndianInt(isLittleEndian, os, header.magic_number);
	osWriteLittleEndianInt(isLittleEndian, os, header.version_number);
	// Write buffer resolution
	osWriteLittleEndianInt(isLittleEndian, os, header.x_resolution);
	osWriteLittleEndianInt(isLittleEndian, os, header.y_resolution);
	// Write #buffer groups and buffer configs for verification
	osWriteLittleEndianInt(isLittleEndian, os, header.num_buffer_groups);
	osWriteLittleEndianInt(isLittleEndian, os, header.num_buffer_configs);
	for (int i = 0; i < header.num_buffer_configs; i++)
		osWriteLittleEndianInt(isLittleEndian, os, header.buffer_types[i]);
	// Write parameters
	osWriteLittleEndianInt(isLittleEndian, os, header.num_params);
	for(int i = 0; i < header.num_params; i++) {
		osWriteLittleEndianInt(isLittleEndian, os, header.params[i].id);
		osWriteLittleEndianInt(isLittleEndian, os, header.params[i].index);
		osWriteLittleEndianFloat(isLittleEndian, os, header.params[i].value);
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
	header.magic_number = FLM_MAGIC_NUMBER;
	header.version_number = FLM_VERSION;
	header.x_resolution = GetXPixelCount();
	header.y_resolution = GetYPixelCount();
	header.num_buffer_groups = (int)bufferGroups.size();
	header.num_buffer_configs = (int)bufferConfigs.size();
	for (u_int i = 0; i < bufferConfigs.size(); i++)
		header.buffer_types.push_back((int)bufferConfigs[i].type);
	// Write parameters
	if( transmitParams ) {
		header.params.push_back(FlmParameter(this, LUX_FILM_TM_TONEMAPKERNEL, 0));

		header.params.push_back(FlmParameter(this, LUX_FILM_TM_REINHARD_PRESCALE, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_TM_REINHARD_POSTSCALE, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_TM_REINHARD_BURN, 0));

		header.params.push_back(FlmParameter(this, LUX_FILM_TM_LINEAR_SENSITIVITY, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_TM_LINEAR_EXPOSURE, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_TM_LINEAR_FSTOP, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_TM_LINEAR_GAMMA, 0));

		header.params.push_back(FlmParameter(this, LUX_FILM_TM_CONTRAST_YWA, 0));

		header.params.push_back(FlmParameter(this, LUX_FILM_TORGB_X_WHITE, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_TORGB_Y_WHITE, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_TORGB_X_RED, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_TORGB_Y_RED, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_TORGB_X_GREEN, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_TORGB_Y_GREEN, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_TORGB_X_BLUE, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_TORGB_Y_BLUE, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_TORGB_GAMMA, 0));


		header.params.push_back(FlmParameter(this, LUX_FILM_UPDATEBLOOMLAYER, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_BLOOMRADIUS, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_BLOOMWEIGHT, 0));

		header.params.push_back(FlmParameter(this, LUX_FILM_VIGNETTING_ENABLED, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_VIGNETTING_SCALE, 0));

		header.params.push_back(FlmParameter(this, LUX_FILM_ABERRATION_ENABLED, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_ABERRATION_AMOUNT, 0));

		header.params.push_back(FlmParameter(this, LUX_FILM_GLARE_ENABLED, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_GLARE_AMOUNT, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_GLARE_RADIUS, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_GLARE_BLADES, 0));

		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_CHIU_ENABLED, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_CHIU_RADIUS, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_CHIU_INCLUDECENTER, 0));

		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_ENABLED, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_AMPLITUDE, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_NBITER, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_SHARPNESS, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_ANISOTROPY, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_ALPHA, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_SIGMA, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_FASTAPPROX, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_GAUSSPREC, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_DL, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_DA, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_INTERP, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_TILE, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_BTILE, 0));
		header.params.push_back(FlmParameter(this, LUX_FILM_NOISE_GREYC_THREADS, 0));

		for(u_int i=0; i < GetNumBufferGroups(); i++) {
			header.params.push_back(FlmParameter(this, LUX_FILM_LG_SCALE, int(i)));
			header.params.push_back(FlmParameter(this, LUX_FILM_LG_ENABLE, int(i)));
			header.params.push_back(FlmParameter(this, LUX_FILM_LG_SCALE_RED, int(i)));
			header.params.push_back(FlmParameter(this, LUX_FILM_LG_SCALE_GREEN, int(i)));
			header.params.push_back(FlmParameter(this, LUX_FILM_LG_SCALE_BLUE, int(i)));
			header.params.push_back(FlmParameter(this, LUX_FILM_LG_TEMPERATURE, int(i)));
		}

		header.num_params = (int)header.params.size();
	}
	else {
		header.num_params = 0;
	}
	WriteFLMHeader(os, isLittleEndian, header);
	// Write each buffer group
	for (u_int i = 0; i < bufferGroups.size(); i++) {
		BufferGroup& bufferGroup = bufferGroups[i];
		// Write number of samples
		osWriteLittleEndianFloat(isLittleEndian, os, bufferGroup.numberOfSamples);

		// Write each buffer
		for (u_int j = 0; j < bufferConfigs.size(); j++) {
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

			if(clearBuffers) {
				// Dade - reset the rendering buffer
				buffer->Clear();
			}
		}

		if(clearBuffers) {
			// Dade - reset the rendering buffer
			bufferGroup.numberOfSamples = 0;
		}
	}

	if(!os.good()) {
		luxError(LUX_SYSTEM, LUX_SEVERE, "Error while preparing film data for transmission");
		return;
	}

    filtering_streambuf<input> in;
    in.push(gzip_compressor(9));
    in.push(os);
    std::streamsize size = boost::iostreams::copy(in, stream);
	if(!stream.good()) {
		luxError(LUX_SYSTEM, LUX_SEVERE, "Error while transmitting film");
		return;
	}

    ss.str("");
    ss << "Film transmission done (" << (size / 1024.0f) << " Kbytes sent)";
    luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
}

float FlexImageFilm::UpdateFilm(Scene *scene, std::basic_istream<char> &stream) {
	const bool isLittleEndian = osIsLittleEndian();

    filtering_stream<input> in;
    in.push(gzip_decompressor());
    in.push(stream);

	std::stringstream ss;
	ss << "Receiving film (little endian=" << (isLittleEndian ? "true" : "false") << ")";
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	// Read header
	FlmHeader header;
	if(!ReadFLMHeader(in, isLittleEndian, &header, this)) {
		return 0.0f;
	}

	// Read buffer groups
	bool err = false; // flag for data errors
	vector<float> bufferGroupNumSamples(bufferGroups.size());
	vector<BlockedArray<Pixel>*> tmpPixelArrays(bufferGroups.size() * bufferConfigs.size());
	for(u_int i = 0; i < bufferGroups.size(); i++) {
		float numberOfSamples;
		osReadLittleEndianFloat(isLittleEndian, in, &numberOfSamples);
		if(!in.good() || err) break;
		bufferGroupNumSamples[i] = numberOfSamples;

		// Read buffers
		for(u_int j = 0; j < bufferConfigs.size(); j++) {
			const Buffer* localBuffer = bufferGroups[i].getBuffer(j);
			// Read pixels
			BlockedArray<Pixel> *tmpPixelArr = new BlockedArray<Pixel>(
				localBuffer->xPixelCount, localBuffer->yPixelCount);
			tmpPixelArrays[i*bufferConfigs.size() + j] = tmpPixelArr;
			for (int y = 0; y < tmpPixelArr->vSize(); ++y) {
				for (int x = 0; x < tmpPixelArr->uSize(); ++x) {
					Pixel &pixel = (*tmpPixelArr)(x, y);
					osReadLittleEndianFloat(isLittleEndian, in, &pixel.L.c[0]);
					osReadLittleEndianFloat(isLittleEndian, in, &pixel.L.c[1]);
					osReadLittleEndianFloat(isLittleEndian, in, &pixel.L.c[2]);
					osReadLittleEndianFloat(isLittleEndian, in, &pixel.alpha);
					osReadLittleEndianFloat(isLittleEndian, in, &pixel.weightSum);
				}
			}
			if(!in.good() || err) break;
		}
		if(!in.good() || err) break;
	}

    // Dade - check for errors
	float totNumberOfSamples = 0.f;
    if (in.good() && !err) {
		// Update parameters
		for(vector<FlmParameter>::iterator it = header.params.begin(); it != header.params.end(); it++) {
			it->Set(this);
		}

        // Dade - add all received data
		for(u_int i = 0; i < bufferGroups.size(); i++) {
			BufferGroup &currentGroup = bufferGroups[i];
			for(u_int j = 0; j < bufferConfigs.size(); j++) {
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
			totNumberOfSamples += bufferGroupNumSamples[i];
		}

        if(scene != NULL)
            scene->numberOfSamplesFromNetwork += totNumberOfSamples;

		ss.str("");
		ss << "Received film with " << totNumberOfSamples << " samples";
		luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
    } else if( err ) {
        luxError(LUX_SYSTEM, LUX_ERROR, "Data error while receiving film buffers");
    } else if( !in.good() ) {
        luxError(LUX_SYSTEM, LUX_ERROR, "IO error while receiving film buffers");
    }

	// Clean up
	for(u_int i = 0; i < tmpPixelArrays.size(); i++) {
		if( tmpPixelArrays[i] )
			delete tmpPixelArrays[i];
	}

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
	bool premultiplyAlpha = params.FindOneBool("premultiplyalpha", true);

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

	// output fileformats
	bool w_tonemapped_EXR = params.FindOneBool("write_tonemapped_exr", true);
	bool w_untonemapped_EXR = params.FindOneBool("write_untonemapped_exr", false);
	bool w_tonemapped_IGI = params.FindOneBool("write_tonemapped_igi", false);
	bool w_untonemapped_IGI = params.FindOneBool("write_untonemapped_igi", false);
	bool w_tonemapped_TGA = params.FindOneBool("write_tonemapped_tga", false);

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
		w_tonemapped_EXR, w_untonemapped_EXR, w_tonemapped_IGI, w_untonemapped_IGI, w_tonemapped_TGA, w_resume_FLM, restart_resume_FLM, haltspp,

		s_TonemapKernel, s_ReinhardPreScale, s_ReinhardPostScale, s_ReinhardBurn, s_LinearSensitivity,
		s_LinearExposure, s_LinearFStop, s_LinearGamma, s_ContrastYwa, s_Gamma,

		red, green, blue, white, reject_warmup, debug_mode);
}


FlexImageFilm *FlexImageFilm::CreateFilmFromFLM(const string& flmFileName) {
	static Filter *dummyFilter = NULL;
	if( dummyFilter == NULL ) {
		ParamSet dummyParams;
		dummyFilter = MakeFilter("box", dummyParams );
	}

	// Read the FLM header
	std::ifstream stream(flmFileName.c_str(), std::ios_base::in | std::ios_base::binary);
	filtering_stream<input> in;
    in.push(gzip_decompressor());
    in.push(stream);
	const bool isLittleEndian = osIsLittleEndian();
	FlmHeader header;
	bool headerOk = ReadFLMHeader(in, isLittleEndian, &header, NULL);
	stream.close();
	if( !headerOk )
		return NULL;

	// Create the default film
	const string filename = flmFileName.substr(0, flmFileName.length() - 4); // remove .flm extention
	const bool boolTrue = true;
	const bool boolFalse = false;
	ParamSet filmParams;
	filmParams.AddString("filename", &filename );
	filmParams.AddInt("xresolution", &header.x_resolution);
	filmParams.AddInt("yresolution", &header.y_resolution);
	filmParams.AddBool("write_resume_flm", &boolTrue);
	filmParams.AddBool("restart_resume_flm", &boolFalse);
	filmParams.FindOneBool("write_tonemapped_exr", &boolFalse);
	filmParams.FindOneBool("write_untonemapped_exr", &boolFalse);
	filmParams.AddBool("write_tonemapped_igi", &boolFalse);
	filmParams.AddBool("write_untonemapped_igi", &boolFalse);
	filmParams.AddBool("write_tonemapped_tga", &boolFalse);
	FlexImageFilm *film = dynamic_cast<FlexImageFilm*>( CreateFilm(filmParams, dummyFilter) );
	
	// Create the buffers (also loads the FLM file)
	for(int i = 0; i < header.num_buffer_configs; i++) {
		film->RequestBuffer(BufferType(header.buffer_types[i]), BUF_FRAMEBUFFER, "");
	}
	vector<string> bufferGroups;
	for(int i = 0; i < header.num_buffer_groups; i++) {
		std::stringstream ss;
		ss << "lightgroup #" << ( i + 1 );
		bufferGroups.push_back(ss.str());
	}
	film->RequestBufferGroups(bufferGroups);
	film->CreateBuffers();

	return film;
}

static DynamicLoader::RegisterFilm<FlexImageFilm> r1("fleximage");
static DynamicLoader::RegisterFilm<FlexImageFilm> r2("multiimage");
