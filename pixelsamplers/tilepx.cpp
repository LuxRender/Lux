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
 
#include "tilepx.h"
#include "error.h"

using namespace lux;

// TilePixelSampler Method Definitions
TilePixelSampler::TilePixelSampler(
        int xStart, int xEnd,
        int yStart, int yEnd) {
    // Dade - debugging code
    //std::stringstream ss;
    //ss << "xstart: " << xstart << " xend: " << xend <<
    //        " ystart: " << ystart << " yend: " << yend;
    //luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

    int xSize = xEnd - xStart;
    int ySize = yEnd - yStart;

    int tileXSize = xSize / TILEPX_SIZE + ((xSize % TILEPX_SIZE == 0) ? 0 : 1);
    int tileYSize = ySize / TILEPX_SIZE + ((ySize % TILEPX_SIZE == 0) ? 0 : 1);
    
    // Dade - debugging code
    //ss.str("");
    //ss << "tileXSize: " << tileXSize << " tileYSize: " << tileYSize;
    //luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

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

	pixelCounter = 0;
}

u_int TilePixelSampler::GetTotalPixels() {
	return TotalPx;
}

bool TilePixelSampler::GetNextPixel(int &xPos, int &yPos, u_int *use_pos) {
    if(pixelCounter == TotalPx)
		return false;

	pixelCounter++;

    u_int pos = (*use_pos);
	xPos = Pxa[pos].x;
	yPos = Pxa[pos].y;

    return true;
}
