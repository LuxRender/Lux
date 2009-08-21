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
 
// lowdiscrepancypx.cpp*
#include "lowdiscrepancypx.h"
#include "error.h"

using namespace lux;

// LowdiscrepancyPixelSampler Method Definitions
LowdiscrepancyPixelSampler::LowdiscrepancyPixelSampler(int xstart, int xend,
		int ystart, int yend) {
	xPixelStart = xstart;
	yPixelStart = ystart;
	xPixelEnd = xend;
	yPixelEnd = yend;
	xSeed = lux::random::uintValueP();
	ySeed = lux::random::uintValueP();

	TotalPx = (xend - xstart) * (yend - ystart);
	pixelCounter = 0;
}

u_int LowdiscrepancyPixelSampler::GetTotalPixels() {
#if defined(WIN32) && !defined(__CYGWIN__)
#undef max // before the use of vaR.max function
#endif
	return std::numeric_limits<unsigned int>::max();
}

bool LowdiscrepancyPixelSampler::GetNextPixel(int &xPos, int &yPos, u_int *use_pos) {
	bool hasMorePixel = true;
	if(pixelCounter == TotalPx) {
		pixelCounter = 0;
		hasMorePixel = false;
	}

	pixelCounter++;

	xPos = xPixelStart + Floor2Int( VanDerCorput( *use_pos, xSeed ) * (xPixelEnd - xPixelStart) );
	yPos = yPixelStart + Floor2Int( Sobol2( *use_pos, ySeed ) * (yPixelEnd - yPixelStart) );

	return hasMorePixel;
}
