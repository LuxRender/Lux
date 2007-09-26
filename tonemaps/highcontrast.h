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

// highcontrast.cpp*
#include "tonemap.h"
#include "mipmap.h"
// HighContrastOp Declarations
class HighContrastOp : public ToneMap {
public:
	void Map(const float *y,
	         int xRes, int yRes,
			 float maxDisplayY, float *scale) const;
			 
	static ToneMap *CreateToneMap(const ParamSet &ps);
private:
	// HighContrastOp Utility Methods
	static float C(float y) {
		if (y < 0.0034f)
			return y / 0.0014f;
		else if (y < 1)
			return 2.4483f + log10f(y/0.0034f)/0.4027f;
		else if (y < 7.2444f)
			return 16.563f + (y - 1)/0.4027f;
		else
			return 32.0693f + log10f(y / 7.2444f)/0.0556f;
	}
	static float T(float y, float CYmin, float CYmax,
			float maxDisplayY) {
		return maxDisplayY * (C(y) - CYmin) / (CYmax - CYmin);
	}
};
