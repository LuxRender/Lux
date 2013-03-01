/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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
#include "sampling.h"
#include "paramset.h"
#include "film.h"

namespace lux
{

// VegasPixelSampler Declarations
class VegasPixelSampler : public PixelSampler {
public:
	// VegasPixelSampler Public Methods
	VegasPixelSampler(int xstart, int xend,
	          int ystart, int yend);
	virtual ~VegasPixelSampler() {
	}

	virtual u_int GetTotalPixels();
	virtual bool GetNextPixel(int *xPos, int *yPos, const u_int usePos);

	static PixelSampler *CreatePixelSampler(int xstart, int xend, int ystart, int yend) {
		return new VegasPixelSampler(xstart, xend, ystart, yend);
	}

private:
	// VegasPixelSampler Private Data
	u_int TotalPx;

	vector<PxLoc> Pxa; // pixel coordinate cache
};

}//namespace lux
