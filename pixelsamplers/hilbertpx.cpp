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
 
#include "hilbertpx.h"
#include "error.h"

using namespace lux;

void HilbertPixelSampler::HilberCurve(
		int n,
		int xo, int yo,
		int xd, int yd, int xp, int yp,
		int xEnd, int yEnd) {
	if (n <= 1) {
		if((xo <= xEnd) && (yo <= yEnd)) {
			PxLoc px;
			px.x = xo; px.y = yo;
			
			// Dade - sanity check
			/*bool found = false;
			for (u_int i = 0; i < TotalPx; i++) {
				if ((Pxa[i].x == px.x) && (Pxa[i].y == px.y)) {
					found = true;
					break;
				}
			}

			if (found)
				printf("Sanity check error: %d, %d\n", px.x, px.y);*/

			Pxa.push_back(px);
			TotalPx++;
		}
	} else {
		int n2 = n >> 1;

		HilberCurve(n2,
				xo,
				yo,
				xp, yp, xd, yd, xEnd, yEnd);
		HilberCurve(n2,
				xo + xp * n2,
				yo + yp * n2,
				xd, yd, xp, yp, xEnd, yEnd);
		HilberCurve(n2,
				xo + xp * n2 + xd * n2,
				yo + yp * n2 + yd * n2,
				xd, yd, xp, yp, xEnd, yEnd);
		HilberCurve(n2,
				xo + xp * (n2 - 1) + xd * (n - 1),
				yo + yp * (n2 - 1) + yd * (n - 1),
				-xp, -yp, -xd, -yd, xEnd, yEnd);
	}
}

// HilbertPixelSampler Method Definitions
HilbertPixelSampler::HilbertPixelSampler(
        int xStart, int xEnd,
        int yStart, int yEnd) {
    int xSize = xEnd - xStart + 1;
    int ySize = yEnd - yStart + 1;

	TotalPx = 0;

	int n = max<int>(xSize, ySize);
	if (!IsPowerOf2(n))
		n = RoundUpPow2(n);
	HilberCurve(n, xStart, yStart, 0, 1, 1, 0, xEnd, yEnd);

	// Dade - sanity check
	/*for(int y = yStart; y <= yEnd; y++) {
		for(int x = xStart; x <= xEnd; x++) {
			// Dade - look for the pixel
			bool found = false;
			for (u_int i = 0; i < TotalPx; i++) {
				if ((Pxa[i].x == short(x)) && (Pxa[i].y == short(y))) {
					found = true;
					break;
				}
			}

			if (!found)
				printf("Sanity check error: %d, %d\n", x, y);
		}
	}*/
}

u_int HilbertPixelSampler::GetTotalPixels() {
	return TotalPx;
}

bool HilbertPixelSampler::GetNextPixel(int &xPos, int &yPos, u_int *use_pos) {
	u_int pos = (*use_pos);
	bool hasMorePixel = true;
	if(pos == TotalPx - 1)
		hasMorePixel = false;

	xPos = Pxa[pos].x;
	yPos = Pxa[pos].y;

    return hasMorePixel;
}
