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
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

// Spectrumwavelengths.cpp*
#include "spectrumwavelengths.h"
#include "regular.h"
#include "memory.h"

using namespace lux;

#include "data/rgbE_32.h"
#include "data/xyzbasis.h"

const RegularSPD SpectrumWavelengths::spd_w(refrgb2spect_white,
	refrgb2spect_start, refrgb2spect_end, refrgb2spect_bins,
	refrgb2spect_scale);

const RegularSPD SpectrumWavelengths::spd_c(refrgb2spect_cyan,
	refrgb2spect_start, refrgb2spect_end, refrgb2spect_bins,
	refrgb2spect_scale);

const RegularSPD SpectrumWavelengths::spd_m(refrgb2spect_magenta,
	refrgb2spect_start, refrgb2spect_end, refrgb2spect_bins,
	refrgb2spect_scale);

const RegularSPD SpectrumWavelengths::spd_y(refrgb2spect_yellow,
	refrgb2spect_start, refrgb2spect_end, refrgb2spect_bins,
	refrgb2spect_scale);

const RegularSPD SpectrumWavelengths::spd_r(refrgb2spect_red,
	refrgb2spect_start, refrgb2spect_end, refrgb2spect_bins,
	refrgb2spect_scale);

const RegularSPD SpectrumWavelengths::spd_g(refrgb2spect_green,
	refrgb2spect_start, refrgb2spect_end, refrgb2spect_bins,
	refrgb2spect_scale);

const RegularSPD SpectrumWavelengths::spd_b(refrgb2spect_blue,
	refrgb2spect_start, refrgb2spect_end, refrgb2spect_bins,
	refrgb2spect_scale);

const RegularSPD SpectrumWavelengths::spd_ciex(CIE_X, CIEstart, CIEend, nCIE,
	683.f * float(WAVELENGTH_END - WAVELENGTH_START) / WAVELENGTH_SAMPLES);

const RegularSPD SpectrumWavelengths::spd_ciey(CIE_Y, CIEstart, CIEend, nCIE,
	683.f * float(WAVELENGTH_END - WAVELENGTH_START) / WAVELENGTH_SAMPLES);

const RegularSPD SpectrumWavelengths::spd_ciez(CIE_Z, CIEstart, CIEend, nCIE,
	683.f * float(WAVELENGTH_END - WAVELENGTH_START) / WAVELENGTH_SAMPLES);
