/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_MITCHELL_H
#define LUX_MITCHELL_H

// mitchell.cpp*
#include "sampling.h"
#include "paramset.h"

namespace lux
{

// Mitchell Filter Declarations
class MitchellFilter : public Filter {
public:
	// MitchellFilter Public Methods
	MitchellFilter(float b, float c, float xw, float yw)
		: Filter(xw, yw) { B = b; C = c; }
	float Evaluate(float x, float y) const;
	float Mitchell1D(float x) const {
		x = fabsf(2.f * x);
		if (x > 1.f)
			return ((-B - 6*C) * x*x*x + (6*B + 30*C) * x*x +
				(-12*B - 48*C) * x + (8*B + 24*C)) * (1.f/6.f);
		else
			return ((12 - 9*B - 6*C) * x*x*x +
				(-18 + 12*B + 6*C) * x*x +
				(6 - 2*B)) * (1.f/6.f);
	}
	
	static Filter *CreateFilter(const ParamSet &ps);
private:
	float B, C;
};

}//namespace lux

#endif

