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

void HilbertPixelSampler::HilberCurve(u_int n, int xo, int yo,
	int xd, int yd, int xp, int yp, int xEnd, int yEnd)
{
	if (n <= 1) {
		if((xo <= xEnd) && (yo <= yEnd)) {
			PxLoc px;
			px.x = xo;
			px.y = yo;
			
			Pxa.push_back(px);
			TotalPx++;
		}
	} else {
		const u_int n2 = n >> 1;

		HilberCurve(n2,
			xo,
			yo,
			xp, yp, xd, yd, xEnd, yEnd);
		HilberCurve(n2,
			xo + xd * static_cast<int>(n2),
			yo + yd * static_cast<int>(n2),
			xd, yd, xp, yp, xEnd, yEnd);
		HilberCurve(n2,
			xo + (xp + xd) * static_cast<int>(n2),
			yo + (yp + yd) * static_cast<int>(n2),
			xd, yd, xp, yp, xEnd, yEnd);
		HilberCurve(n2,
			xo + xd * static_cast<int>(n2 - 1) + xp * static_cast<int>(n - 1),
			yo + yd * static_cast<int>(n2 - 1) + yp * static_cast<int>(n - 1),
			-xp, -yp, -xd, -yd, xEnd, yEnd);
	}
}

// HilbertPixelSampler Method Definitions
HilbertPixelSampler::HilbertPixelSampler(int xStart, int xEnd,
	int yStart, int yEnd)
{
	u_int xSize = static_cast<u_int>(xEnd - xStart + 1);
	u_int ySize = static_cast<u_int>(yEnd - yStart + 1);

	TotalPx = 0;

	u_int n = max(xSize, ySize);
	if (!IsPowerOf2(n))
		n = RoundUpPow2(n);
	HilberCurve(n, xStart, yStart, 0, 1, 1, 0, xEnd, yEnd);
	if (TotalPx != xSize * ySize)
		LOG(LUX_DEBUG, LUX_BUG) << "Hilbert sampler generated wrong number of samples";
}

u_int HilbertPixelSampler::GetTotalPixels()
{
	return TotalPx;
}

bool HilbertPixelSampler::GetNextPixel(int *xPos, int *yPos, const u_int usePos)
{
	*xPos = Pxa[usePos].x;
	*yPos = Pxa[usePos].y;

	return usePos != TotalPx - 1;
}
