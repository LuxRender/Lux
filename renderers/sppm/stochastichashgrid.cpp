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

StochasticHashGrid::StochasticHashGrid(HitPoints *hps) {
	hitPoints = hps;
	grid = NULL;

	RefreshMutex(0);
}

StochasticHashGrid::~StochasticHashGrid() {
	delete grid;
}

void StochasticHashGrid::RefreshMutex(const u_int passIndex) {
	const unsigned int hitPointsCount = hitPoints->GetSize();
	const BBox &hpBBox = hitPoints->GetBBox(passIndex);

	// Calculate the size of the grid cell
	const float maxPhotonRadius2 = hitPoints->GetMaxPhotonRaidus2(passIndex);
	const float cellSize = sqrtf(maxPhotonRadius2) * 2.f;
	LOG(LUX_INFO, LUX_NOERROR) << "Stochastic Hash grid cell size: " << cellSize;
	invCellSize = 1.f / cellSize;

	if (!grid) {
		// TODO: add a tunable parameter for hashgrid size
		gridSize = hitPointsCount;
		grid = new GridCell[gridSize];
	}
	memset(grid, 0, sizeof(GridCell) * gridSize);

	LOG(LUX_INFO, LUX_NOERROR) << "Building hit points stochastic hash grid";
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
						int hv = Hash(ix, iy, iz);

						grid[hv].hitPoint = hp;
						grid[hv].count += 1;
					}
				}
			}
		}
	}

	/*// StochasticHashGrid debug code
	u_int badCells = 0;
	u_int emptyCells = 0;
	for (u_int i = 0; i < gridSize; ++i) {
		if (grid[i].count == 0)
			++emptyCells;
		else if (grid[i].count > 5) {
			//std::cerr << "StochasticHashGrid[" << i << "].count = " << grid[i].count << std::endl;
			++badCells;
		}
	}
	std::cerr << "StochasticHashGrid.badCells = " << (100.f * badCells / (gridSize - emptyCells)) << "%" << std::endl;
	std::cerr << "StochasticHashGrid.emptyCells = " << (100.f * emptyCells / gridSize) << "%" << std::endl;*/
}

void StochasticHashGrid::AddFlux(const Point &hitPoint, const u_int passIndex, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group) {
	// Look for eye path hit points near the current hit point
	Vector hh = (hitPoint - hitPoints->GetBBox(passIndex).pMin) * invCellSize;
	const int ix = abs(int(hh.x));
	const int iy = abs(int(hh.y));
	const int iz = abs(int(hh.z));

	GridCell &cell(grid[Hash(ix, iy, iz)]);
	if (cell.count > 0)
		AddFluxToHitPoint(cell.hitPoint, passIndex, hitPoint, wi, sw, photonFlux * cell.count, light_group);
}
