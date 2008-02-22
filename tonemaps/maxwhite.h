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

// maxwhite.cpp*
#include "tonemap.h"

namespace lux
{

// MaxWhiteOp Declarations
class MaxWhiteOp : public ToneMap {
public:
	// MaxWhiteOp Public Methods
	void Map(const float *y, int xRes, int yRes,
	         float maxDisplayY, float *scale) const {
		// Compute maximum luminance of all pixels
		float maxY = 0.;
		for (int i = 0; i < xRes * yRes; ++i)
			maxY = max(maxY, y[i]);
		float s = maxDisplayY / maxY;
		for (int i = 0; i < xRes * yRes; ++i)
			scale[i] = s;
	}
	
	static ToneMap *CreateToneMap(const ParamSet &ps);
};

}//namespace lux

