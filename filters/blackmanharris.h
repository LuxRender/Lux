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

#ifndef LUX_BLACKMANHARRIS_H

#define LUX_BLACKMANHARRIS_H


//blackmanharris.cpp
#include "filter.h"
#include "paramset.h"

namespace lux
{
	
	//BlackmanHarris Filter Declarations
	class BlackmanHarrisFilter : public Filter {
	public:
		BlackmanHarrisFilter(float xw, float yw) : Filter(xw, yw), xri(2.f / xw), yri(2.f / yw) {
			AddStringConstant(*this, "type", "Filter type", "blackmanharris");
		}
		virtual ~BlackmanHarrisFilter() { }
		virtual float Evaluate(float x, float y) const;
		
		static Filter *CreateFilter(const ParamSet &ps);
	private:
		float xri;
		float yri;
		static float BlackmanHarris1D(float x)  {
			if (x < -1.f || x > 1.f)
				return 0.f;
			x = (x + 1.f) * 0.5f;
			x *= M_PI;
			const float A0 =  0.35875f;
			const float A1 = -0.48829f;
			const float A2 =  0.14128f;
			const float A3 = -0.01168f;
			return A0 + A1 * cosf(2.f * x) + A2 * cosf(4.f * x) + A3 * cosf(6.f * x);
		}
	};
	
}//namspace lux

#endif