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

// maxwhite.cpp*
#include "lux.h"
#include "tonemap.h"
#include "color.h"

namespace lux
{

// MaxWhiteOp Declarations
class MaxWhiteOp : public ToneMap {
public:
	// MaxWhiteOp Public Methods
	virtual ~MaxWhiteOp() { }
	virtual void Map(vector<Color> &xyz, int xRes, int yRes, float maxDisplayY) const {
		const int numPixels = xRes * yRes;
		// Compute maximum luminance of all pixels
		float maxY = 0.f;
		for (int i = 0; i < numPixels; ++i)
			maxY = max(maxY, xyz[i].c[1]);
		float s = 1.f / maxY;
		for (int i = 0; i < numPixels; ++i)
			xyz[i] *= s;
	}
	
	static ToneMap *CreateToneMap(const ParamSet &ps);
};

}//namespace lux

