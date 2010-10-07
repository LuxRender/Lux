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
 
#include "tilepx.h"
#include "error.h"

using namespace lux;

// TilePixelSampler Method Definitions
TilePixelSampler::TilePixelSampler(
        int xStart, int xEnd,
        int yStart, int yEnd) {
    // Dade - debugging code
    //LOG(LUX_NOERROR, LUX_INFO) << "xstart: " << xstart << " xend: " << xend <<
    //        " ystart: " << ystart << " yend: " << yend;

    int xSize = xEnd - xStart;
    int ySize = yEnd - yStart;

    int tileXSize = xSize / TILEPX_SIZE + ((xSize % TILEPX_SIZE == 0) ? 0 : 1);
    int tileYSize = ySize / TILEPX_SIZE + ((ySize % TILEPX_SIZE == 0) ? 0 : 1);
    
    // Dade - debugging code
    //LOG(LUX_NOERROR, LUX_INFO) << "tileXSize: " << tileXSize << " tileYSize: " << tileYSize;

    TotalPx = 0;
    for(int yg = 0; yg < tileYSize; yg++) {
        for(int xg = 0; xg < tileXSize; xg++) {
            for(int y = yStart +  yg * TILEPX_SIZE; y < yStart + (yg + 1) * TILEPX_SIZE; y++) {
                for(int x = xStart + xg * TILEPX_SIZE; x < xStart + (xg + 1) * TILEPX_SIZE; x++) {
                    if((x <= xEnd) && (y <= yEnd)) {
                        PxLoc px;
                        px.x = x; px.y = y;
                        Pxa.push_back(px);
                        TotalPx++;
                    }
                }
            }
        }
    }
}

u_int TilePixelSampler::GetTotalPixels() {
	return TotalPx;
}

bool TilePixelSampler::GetNextPixel(int *xPos, int *yPos, const u_int use_pos) {
	bool hasMorePixel = true;
	if(use_pos == TotalPx - 1)
		hasMorePixel = false;

	*xPos = Pxa[use_pos].x;
	*yPos = Pxa[use_pos].y;

	return hasMorePixel;
}
