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

// lineartonemap.h*
#include "lux.h"
#include "tonemap.h"
#include "color.h"

namespace lux
{

// MaxWhiteOp Declarations
class LinearOp : public ToneMap {
public:
	// LinearOp Public Methods
	LinearOp(float sensitivity, float exposure, float fstop, float gamma) :
		factor(exposure / (fstop * fstop) * sensitivity / 10.f * powf(118.f / 255.f, gamma)) { }
	virtual ~LinearOp() { }
	virtual void Map(vector<Color> &xyz, int xRes, int yRes, float maxDisplayY) const {
		const int numPixels = xRes * yRes;
		for (int i = 0; i < numPixels; ++i)
			xyz[i] *= factor;
	}
	
	static ToneMap *CreateToneMap(const ParamSet &ps);
private:
	// LinearOp Private Data
	const float factor;
};

}//namespace lux

