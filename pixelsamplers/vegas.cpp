/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

using namespace lux;

// VegasPixelSampler Method Definitions
VegasPixelSampler::VegasPixelSampler(int xstart, int xend,
		int ystart, int yend) {
	u_int xPos = xstart - 1;
	u_int yPos = ystart;

	// fill Pxa array in film pixel order
	unsigned short int x = (unsigned short int) xPos;
	unsigned short int y = (unsigned short int) yPos;
	TotalPx = 0;
	while(true) {
		PxLoc px;
		px.x = x; px.y = y;
		Pxa.push_back(px);
		x++;
		if(x == xend) {
			x = 0;
			y++;
			if(y == yend) break;
		}
		TotalPx++;
	}

	// Shuffle elements by randomly exchanging each with one other.
    for (u_int i=0; i<TotalPx; i++) {
		u_int r = Ceil2Int( lux::random::floatValue() * TotalPx );
		// swap
		unsigned short int temp = Pxa[i].x; Pxa[i].x = Pxa[r].x; Pxa[r].x = temp;
		temp = Pxa[i].y; Pxa[i].y = Pxa[r].y; Pxa[r].y = temp;
    } 
}

u_int VegasPixelSampler::GetTotalPixels() {
	return TotalPx;
}

bool VegasPixelSampler::GetNextPixel(int &xPos, int &yPos, u_int *use_pos) {
	xPos = Pxa[*use_pos].x;
	yPos = Pxa[*use_pos].y;
	return true;
}
