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

#ifndef LUX_MITCHELL_H
#define LUX_MITCHELL_H

// mitchell.cpp*
#include "filter.h"
#include "paramset.h"

namespace lux
{

// Mitchell Filter Declarations
class MitchellFilter : public Filter {
public:
	// MitchellFilter Public Methods
	MitchellFilter(float b, float c, float xw, float yw)
		: Filter(xw, yw) { B = b; C = c; }
	virtual ~MitchellFilter() { }
	virtual float Evaluate(float x, float y) const;
	
	static Filter *CreateFilter(const ParamSet &ps);
private:
	float Mitchell1D(float x) const {
		x = fabsf(2.f * x);
		if (x > 1.f)
			return (((-B/6.f - C) * x + (B + 5.f*C)) * x +
				(-2.f*B - 8.f*C)) * x + (4.f/3.f*B + 4.f*C);
		else
			return ((2.f - 1.5f*B - C) * x +
				(-3.f + 2.f*B + C)) * x*x +
				(1.f - B/3.f);
	}
	float B, C;
};

}//namespace lux

#endif

