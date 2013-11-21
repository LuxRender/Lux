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
#include "volumegrid.h"
#include "paramset.h"
#include "dynload.h"
#include "error.h"

#include <cstring>
using std::memset;

using namespace lux;

// VolumeGrid Method Definitions
VolumeGrid::VolumeGrid(const RGBColor &sa, const RGBColor &ss, float gg,
	const RGBColor &emit, const BBox &e, const Transform &v2w,
	int x, int y, int z, const float *d) :
	DensityVolume<RGBVolume>("VolumeGrid-"  + boost::lexical_cast<string>(this),
		RGBVolume(sa, ss, emit, gg)),
	nx(x), ny(y), nz(z), extent(e), VolumeToWorld(v2w)
{
	density.assign(d, d+(nx*ny*nz));
}
float VolumeGrid::Density(const Point &p) const
{
	const Point pp(Inverse(VolumeToWorld) * p);
	if (!extent.Inside(pp))
		return 0.f;
	// Compute voxel coordinates and offsets for _pp_
	float voxx = (pp.x - extent.pMin.x) /
		(extent.pMax.x - extent.pMin.x) * nx - .5f;
	float voxy = (pp.y - extent.pMin.y) /
		(extent.pMax.y - extent.pMin.y) * ny - .5f;
	float voxz = (pp.z - extent.pMin.z) /
		(extent.pMax.z - extent.pMin.z) * nz - .5f;
	int vx = luxrays::Floor2Int(voxx);
	int vy = luxrays::Floor2Int(voxy);
	int vz = luxrays::Floor2Int(voxz);
	float dx = voxx - vx, dy = voxy - vy, dz = voxz - vz;
	// Trilinearly interpolate density values to compute local density
	float d00 = luxrays::Lerp(dx, D(vx, vy, vz), D(vx + 1, vy, vz));
	float d10 = luxrays::Lerp(dx, D(vx, vy + 1, vz), D(vx + 1, vy + 1, vz));
	float d01 = luxrays::Lerp(dx, D(vx, vy, vz+1), D(vx+1, vy, vz+1));
	float d11 = luxrays::Lerp(dx, D(vx, vy + 1, vz + 1), D(vx + 1, vy + 1, vz + 1));
	float d0 = luxrays::Lerp(dy, d00, d10);
	float d1 = luxrays::Lerp(dy, d01, d11);
	return luxrays::Lerp(dz, d0, d1);
}
Region * VolumeGrid::CreateVolumeRegion(const Transform &volume2world,
		const ParamSet &params) {
	// Initialize common volume region parameters
	RGBColor sigma_a = params.FindOneRGBColor("sigma_a", 0.);
	RGBColor sigma_s = params.FindOneRGBColor("sigma_s", 0.);
	float g = params.FindOneFloat("g", 0.);
	RGBColor Le = params.FindOneRGBColor("Le", 0.);
	Point p0 = params.FindOnePoint("p0", Point(0,0,0));
	Point p1 = params.FindOnePoint("p1", Point(1,1,1));
	u_int nitems;
	const float *data = params.FindFloat("density", &nitems);
	if (!data) {
		LOG(LUX_ERROR,LUX_MISSINGDATA)<< "No \"density\" values provided for volume grid?";
		return NULL;
	}
	int nx = params.FindOneInt("nx", 1);
	int ny = params.FindOneInt("ny", 1);
	int nz = params.FindOneInt("nz", 1);
	if (nitems != static_cast<u_int>(nx * ny * nz)) {
		LOG(LUX_ERROR,LUX_CONSISTENCY)<<"VolumeGrid has "<<nitems<<" density values but nx*ny*nz = "<<nx*ny*nz;
		return NULL;
	}
	return new VolumeRegion<VolumeGrid>(volume2world, BBox(p0, p1),
		VolumeGrid(sigma_a, sigma_s, g, Le, BBox(p0, p1),
		volume2world, nx, ny, nz, data));
}

static DynamicLoader::RegisterVolumeRegion<VolumeGrid> r("volumegrid");
