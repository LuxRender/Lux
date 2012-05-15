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
 
// vegas.cpp*
#include "vegas.h"
#include "error.h"
#include "dynload.h"

using namespace lux;

// VegasPixelSampler Method Definitions
VegasPixelSampler::VegasPixelSampler(int xstart, int xend,
		int ystart, int yend) {
	int xPos = xstart;
	int yPos = ystart;
	// fill Pxa array in film pixel order
	int x = xPos;
	int y = yPos;
	TotalPx = 0;
	while(true) {
		PxLoc px;
		px.x = x; px.y = y;
		Pxa.push_back(px);
		x++;
		TotalPx++;
		if(x == xend) {
			x = xstart;
			y++;
			if(y == yend)
				break;
		}
	}

	// Shuffle elements by randomly exchanging each with one other.
	for (u_int i=0; i<TotalPx; i++) {
		u_int r = Floor2UInt(lux::random::floatValueP() * TotalPx);
		swap(Pxa[i], Pxa[r]);
    } 
}

u_int VegasPixelSampler::GetTotalPixels() {
	return TotalPx;
}

bool VegasPixelSampler::GetNextPixel(int *xPos, int *yPos, const u_int use_pos) {
	bool hasMorePixel = true;
	if(use_pos == TotalPx - 1)
		hasMorePixel = false;

	*xPos = Pxa[use_pos].x;
	*yPos = Pxa[use_pos].y;

	return hasMorePixel;
}

static DynamicLoader::RegisterPixelSampler<VegasPixelSampler> r("vegas");
