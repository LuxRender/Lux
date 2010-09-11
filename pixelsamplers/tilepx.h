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
 
#include "sampling.h"
#include "paramset.h"
#include "film.h"

namespace lux
{

// Dade - TilePixelSampler Declarations
class TilePixelSampler : public PixelSampler {
    #define TILEPX_SIZE 32

public:
	// TilePixelSampler Public Methods
	TilePixelSampler(
            int xStart, int xEnd,
            int yStart, int yEnd);
	virtual ~TilePixelSampler() { }

	virtual u_int GetTotalPixels();
	virtual bool GetNextPixel(int *xPos, int *yPos, const u_int usePos);

private:
	// TilePixelSampler Private Data
	u_int TotalPx;

	vector<PxLoc> Pxa; // pixel coordinate cache
};

}//namespace lux
