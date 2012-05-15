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

// exponential.cpp*
#include "volume.h"

namespace lux
{

// ExponentialDensity Declarations
class ExponentialDensity : public DensityVolume<RGBVolume> {
public:
	// ExponentialDensity Public Methods
	ExponentialDensity(const RGBColor &sa, const RGBColor &ss,
			float gg, const RGBColor &emit, const BBox &e,
			const Transform &v2w, float aa, float bb,
			const Vector &up)
		: DensityVolume<RGBVolume>(RGBVolume(sa, ss, emit, gg)),
		  base(v2w(e.pMin)), dir(Normalize(v2w(up))), a(aa), b(bb) { }
	virtual ~ExponentialDensity() { }
	virtual float Density(const Point &Pobj) const {
		const float height = Dot(Pobj - base, dir);
		return a * expf(-b * height);
	}
	
	static Region *CreateVolumeRegion(const Transform &volume2world,
		const ParamSet &params);
private:
	// ExponentialDensity Private Data
	Point base;
	Vector dir;
	float a, b;
};

}//namespace lux

