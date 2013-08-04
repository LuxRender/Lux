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

#ifndef LUX_CATMULLROM_H

#define LUX_CATMULLROM_H


//catmullrom.cpp
#include "filter.h"
#include "paramset.h"

namespace lux
{
	
	//CatmullRom Filter Declarations
	class CatmullRomFilter : public Filter {
	public:
		CatmullRomFilter(float xw, float yw) : Filter(xw, yw) {
			AddStringConstant(*this, "type", "Filter type", "catmullrom");
		}
		virtual ~CatmullRomFilter() { }
		virtual float Evaluate(float x, float y) const;
		
		static Filter *CreateFilter(const ParamSet &ps);
	private:
		static float CatmullRom1D(float x) {
			x = fabsf(x);
			float x2 = x * x;
			return (x >= 2.f) ? 0.f : ((x < 1.f) ?
									   (3.f * (x * x2) - 5.f * x2 + 2.f) :
									   (-(x * x2) + 5.f * x2 - 8.f * x + 4.f));
		}
	};
	
}//namspace lux

#endif