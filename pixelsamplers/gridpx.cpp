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
 
// gridpx.cpp*
#include "gridpx.h"
#include "error.h"

using namespace lux;

// GridPixelSampler Method Definitions
GridPixelSampler::GridPixelSampler(
        int log2SampleCount,
        int xStart, int xEnd,
        int yStart, int yEnd) {

    // Dade - debugging code
    //std::stringstream ss;
    //ss << "xstart: " << xstart << " xend: " << xend <<
    //        " ystart: " << ystart << " yend: " << yend;
    //luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

    l2SampleCount = log2SampleCount;
    int sampleCount = 1 << log2SampleCount;

    int xSize = xEnd - xStart;
    int ySize = yEnd - yStart;

    int gridXSize = xSize / GRIDPX_SIZE + ((xSize % GRIDPX_SIZE == 0) ? 0 : 1);
    int gridYSize = ySize / GRIDPX_SIZE + ((ySize % GRIDPX_SIZE == 0) ? 0 : 1);
    
    // Dade - debugging code
    //ss.str("");
    //ss << "gridXSize: " << gridXSize << " gridYSize: " << gridYSize;
    //luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

    TotalPx = 0;
    for(int yg = 0; yg < gridYSize; yg++) {
        for(int xg = 0; xg < gridXSize; xg++) {
            for(int y = yStart +  yg * GRIDPX_SIZE; y < yStart + (yg + 1) * GRIDPX_SIZE; y++) {
                for(int x = xStart + xg * GRIDPX_SIZE; x < xStart + (xg + 1) * GRIDPX_SIZE; x++) {
                    if((x <= xEnd) && (y <= yEnd)) {
                        PxLoc px;
                        px.x = x; px.y = y;
                        Pxa.push_back(px);
                        TotalPx += sampleCount;
                    }
                }
            }
        }
    }
}

u_int GridPixelSampler::GetTotalPixels() {
	return TotalPx;
}

bool GridPixelSampler::GetNextPixel(int &xPos, int &yPos, u_int *use_pos) {
    u_int pos = (*use_pos) >> l2SampleCount;
	xPos = Pxa[pos].x;
	yPos = Pxa[pos].y;

    return true;
}
