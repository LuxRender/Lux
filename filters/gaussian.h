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

#ifndef LUX_GAUSSIAN_H
#define LUX_GAUSSIAN_H

// gaussian.cpp*
#include "filter.h"
#include "paramset.h"

namespace lux
{

// Gaussian Filter Declarations
class GaussianFilter : public Filter {
public:
	// GaussianFilter Public Methods
	GaussianFilter(float xw, float yw, float a)
		: Filter(xw, yw) {
		alpha = a;
		expX = expf(-alpha * xWidth * xWidth);
		expY = expf(-alpha * yWidth * yWidth);
	}
	virtual ~GaussianFilter() { }
	virtual float Evaluate(float x, float y) const;
	
	static Filter *CreateFilter(const ParamSet &ps);
private:
	// GaussianFilter Private Data
	float alpha;
	float expX, expY;
	// GaussianFilter Utility Functions
	float Gaussian(float d, float expv) const {
		return max(0.f, expf(-alpha * d * d) - expv);
	}
};

}//namespace lux

#endif
