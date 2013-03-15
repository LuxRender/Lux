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

// volumegrid.cpp*
#include "volume.h"

namespace lux
{

// VolumeGrid Declarations
class VolumeGrid : public DensityVolume<RGBVolume> {
public:
	// VolumeGrid Public Methods
	VolumeGrid(const RGBColor &sa, const RGBColor &ss, float gg,
 		const RGBColor &emit, const BBox &e, const Transform &v2w,
		int nx, int ny, int nz, const float *d);
	virtual ~VolumeGrid() { }
	virtual float Density(const Point &Pobj) const;
	float D(int x, int y, int z) const {
		x = Clamp(x, 0, nx - 1);
		y = Clamp(y, 0, ny - 1);
		z = Clamp(z, 0, nz - 1);
		return density[z * nx * ny + y * nx + x];
	}
	
	static Region *CreateVolumeRegion(const Transform &volume2world, const ParamSet &params);
private:
	// VolumeGrid Private Data
	std::vector<float> density;
	const int nx, ny, nz;
	const BBox extent;
	Transform VolumeToWorld;
};

}//namespace lux

