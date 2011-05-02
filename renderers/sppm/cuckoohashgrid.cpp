/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

#include "hitpoints.h"
#include "lookupaccel.h"
#include "bxdf.h"

using namespace lux;

CuckooHashGrid::CuckooHashGrid(HitPoints *hps) {
	hitPoints = hps;
	grid1 = NULL;
	grid2 = NULL;
	grid3 = NULL;

	RefreshMutex(0);
}

CuckooHashGrid::~CuckooHashGrid() {
	delete grid1;
	delete grid2;
	delete grid3;
}

void CuckooHashGrid::RefreshMutex(const u_int passIndex) {
	const unsigned int hitPointsCount = hitPoints->GetSize();
	const BBox &hpBBox = hitPoints->GetBBox(passIndex);

	// Calculate the size of the grid cell
	const float maxPhotonRadius2 = hitPoints->GetMaxPhotonRaidus2(passIndex);
	const float cellSize = sqrtf(maxPhotonRadius2) * 2.f;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Cuckoo Hash grid cell size: " << cellSize;
	invCellSize = 1.f / cellSize;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Hash grid size: (" <<
			(hpBBox.pMax.x - hpBBox.pMin.x) * invCellSize << ", " <<
			(hpBBox.pMax.y - hpBBox.pMin.y) * invCellSize << ", " <<
			(hpBBox.pMax.z - hpBBox.pMin.z) * invCellSize << ")";

	if (!grid1) {
		// TODO: add a tunable parameter for hashgrid size
		gridSize = 10 * hitPointsCount;
		grid1 = new GridCell[gridSize];
		grid2 = new GridCell[gridSize];
		grid3 = new GridCell[gridSize];
	}
	memset(grid1, 0, sizeof(GridCell) * gridSize);
	memset(grid2, 0, sizeof(GridCell) * gridSize);
	memset(grid3, 0, sizeof(GridCell) * gridSize);

	LOG(LUX_DEBUG, LUX_NOERROR) << "Building hit points cuckoo hash grid";
	unsigned long long entryCount = 0;
	for (unsigned int i = 0; i < hitPointsCount; ++i) {
		HitPoint *hp = hitPoints->GetHitPoint(i);
		HitPointEyePass *hpep = &hp->eyePass[passIndex];

		if (hpep->type == SURFACE) {
			const float photonRadius = sqrtf(hp->accumPhotonRadius2);
			const Vector rad(photonRadius, photonRadius, photonRadius);
			const Vector bMin = ((hpep->position - rad) - hpBBox.pMin) * invCellSize;
			const Vector bMax = ((hpep->position + rad) - hpBBox.pMin) * invCellSize;

			for (int iz = abs(int(bMin.z)); iz <= abs(int(bMax.z)); ++iz) {
				for (int iy = abs(int(bMin.y)); iy <= abs(int(bMax.y)); ++iy) {
					for (int ix = abs(int(bMin.x)); ix <= abs(int(bMax.x)); ++ix) {
						++entryCount;

						Insert(ix, iy, iz, hp);
					}
				}
			}
		}
	}

	LOG(LUX_DEBUG, LUX_NOERROR) << "Total cukoo hash grid entry: " << entryCount;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Avg. hit points in a single cukoo hash grid entry: " << entryCount / gridSize;

	/*// CuckooHashGrid debug code
	u_int emptyCells = 0;
	for (u_int i = 0; i < gridSize; ++i) {
		if (!grid1[i].hitPoint)
			++emptyCells;
	}
	std::cerr << "CuckooHashGrid.Grid1.emptyCells = " << (100.f * emptyCells / gridSize) << "%" << std::endl;

	emptyCells = 0;
	for (u_int i = 0; i < gridSize; ++i) {
		if (!grid2[i].hitPoint)
			++emptyCells;
	}
	std::cerr << "CuckooHashGrid.Grid2.emptyCells = " << (100.f * emptyCells / gridSize) << "%" << std::endl;

	emptyCells = 0;
	for (u_int i = 0; i < gridSize; ++i) {
		if (!grid3[i].hitPoint)
			++emptyCells;
	}
	std::cerr << "CuckooHashGrid.Grid3.emptyCells = " << (100.f * emptyCells / gridSize) << "%" << std::endl;*/
}

void CuckooHashGrid::Insert(const int x, const int y, const int z, HitPoint *hp) {
	int ix = x;
	int iy = y;
	int iz = z;
	HitPoint *hitPoint = hp;

	// Check if we have already inserted this hit point
	const GridCell &gc1(grid1[Hash1(ix, iy, iz)]);
	if ((gc1.hitPoint == hitPoint) &&
			(gc1.ix == ix) && (gc1.iy == iy) && (gc1.iz == iz))
		return;

	const GridCell &gc2(grid2[Hash2(ix, iy, iz)]);
	if ((gc2.hitPoint == hitPoint) &&
			(gc2.ix == ix) && (gc2.iy == iy) && (gc2.iz == iz))
		return;

	const GridCell &gc3(grid3[Hash3(ix, iy, iz)]);
	if ((gc3.hitPoint == hitPoint) &&
			(gc3.ix == ix) && (gc3.iy == iy) && (gc3.iz == iz))
		return;

	GridCell *grid = grid1;
	u_int pos = Hash1(ix, iy, iz);
	for (int i = 0; i < 32; ++i) {
		if (grid[pos].hitPoint == NULL) {
			grid[pos].ix = ix;
			grid[pos].iy = iy;
			grid[pos].iz = iz;
			grid[pos].hitPoint = hitPoint;
			return;
		}

		// Kick the value out of its place
		swap<HitPoint *>(grid[pos].hitPoint, hitPoint);
		swap<int>(grid[pos].ix, ix);
		swap<int>(grid[pos].iy, iy);
		swap<int>(grid[pos].iz, iz);

		if (pos == Hash1(ix, iy, iz)) {
			grid = grid2;
			pos = Hash2(ix, iy, iz);
		} else {
			if (pos == Hash2(ix, iy, iz)) {
				grid = grid3;
				pos = Hash3(ix, iy, iz);
			} else {
				grid = grid1;
				pos = Hash1(ix, iy, iz);
			}
		}
	}

	// Mmmm, I should reash all the table but I just give up and discard the hit point
	//LOG(LUX_DEBUG, LUX_NOERROR) << "Discarded hit point in CuckooHashGrid::Insert()";
}

void CuckooHashGrid::AddFlux(const Point &hitPoint, const u_int passIndex, const BSDF &bsdf, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group) {
	// Look for eye path hit points near the current hit point
	Vector hh = (hitPoint - hitPoints->GetBBox(passIndex).pMin) * invCellSize;
	const int ix = abs(int(hh.x));
	const int iy = abs(int(hh.y));
	const int iz = abs(int(hh.z));

	HitPoint *hp = grid1[Hash1(ix, iy, iz)].hitPoint;
	if (hp)
		AddFluxToHitPoint(hp, passIndex, bsdf, hitPoint, wi, sw, photonFlux, light_group);

	hp = grid2[Hash2(ix, iy, iz)].hitPoint;
	if (hp)
		AddFluxToHitPoint(hp, passIndex, bsdf, hitPoint, wi, sw, photonFlux, light_group);

	hp = grid3[Hash3(ix, iy, iz)].hitPoint;
	if (hp)
		AddFluxToHitPoint(hp, passIndex, bsdf, hitPoint, wi, sw, photonFlux, light_group);
}
