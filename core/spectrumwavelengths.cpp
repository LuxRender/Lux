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

// Spectrumwavelengths.cpp*
#include "spectrumwavelengths.h"
#include "regular.h"
#include "spd.h"
#include "memory.h"

using namespace lux;

#include "data/rgbE_32.h"

// SpectrumWavelengths Public Methods
SpectrumWavelengths::SpectrumWavelengths() {
	single = false;
	single_w = 0;

	spd_w = new RegularSPD(refrgb2spect_white, 
		refrgb2spect_start, refrgb2spect_end, refrgb2spect_bins);
	spd_w->Scale(refrgb2spect_scale);

	spd_c = new RegularSPD(refrgb2spect_cyan,
		refrgb2spect_start, refrgb2spect_end, refrgb2spect_bins);
	spd_c->Scale(refrgb2spect_scale);

	spd_m = new RegularSPD(refrgb2spect_magenta,
		refrgb2spect_start, refrgb2spect_end, refrgb2spect_bins);
	spd_m->Scale(refrgb2spect_scale);

	spd_y = new RegularSPD(refrgb2spect_yellow,
		refrgb2spect_start, refrgb2spect_end, refrgb2spect_bins);
	spd_y->Scale(refrgb2spect_scale);

	spd_r = new RegularSPD(refrgb2spect_red,
		refrgb2spect_start, refrgb2spect_end, refrgb2spect_bins);
	spd_r->Scale(refrgb2spect_scale);

	spd_g = new RegularSPD(refrgb2spect_green,
		refrgb2spect_start, refrgb2spect_end, refrgb2spect_bins);
	spd_g->Scale(refrgb2spect_scale);

	spd_b = new RegularSPD(refrgb2spect_blue,
		refrgb2spect_start, refrgb2spect_end, refrgb2spect_bins);
	spd_b->Scale(refrgb2spect_scale);
}

SpectrumWavelengths::~SpectrumWavelengths()
{
	delete spd_w;
	delete spd_c;
	delete spd_m;
	delete spd_y;
	delete spd_r;
	delete spd_g;
	delete spd_b;
}

