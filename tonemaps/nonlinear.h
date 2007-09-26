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

// nonlinear.cpp*
#include "tonemap.h"
#include "paramset.h"
// NonLinearOp Declarations
class NonLinearOp : public ToneMap {
public:
	// NonLinearOp Public Methods
	NonLinearOp(float my) { maxY = my; }
	void Map(const float *y, int xRes, int yRes,
			float maxDisplayY, float *scale) const {
		float invY2;
		if (maxY <= 0.f) {
			// Compute world adaptation luminance, _Ywa_
			float Ywa = 0.;
			for (int i = 0; i < xRes * yRes; ++i)
				if (y[i] > 0) Ywa += logf(y[i]);
			Ywa = expf(Ywa / (xRes * yRes));
			Ywa /= 683.f;
			invY2 = 1.f / (Ywa * Ywa);
		}
		else invY2 = 1.f / (maxY * maxY);
		for (int i = 0; i < xRes * yRes; ++i) {
			float ys = y[i] / 683.f;
			scale[i] = maxDisplayY / 683.f *
				(1.f + ys * invY2) / (1.f + ys);
		}
	}
	
	static ToneMap *CreateToneMap(const ParamSet &ps);
private:
	float maxY;
};
