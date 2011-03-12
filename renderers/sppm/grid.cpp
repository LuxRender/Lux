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

GridLookUpAccel::GridLookUpAccel(HitPoints *hps) {
	hitPoints = hps;
	grid = NULL;

	RefreshMutex(0);
}

GridLookUpAccel::~GridLookUpAccel() {
	for (unsigned int i = 0; i < gridSize; ++i)
		delete grid[i];
	delete[] grid;
}

void GridLookUpAccel::RefreshMutex(const u_int passIndex) {
	const unsigned int hitPointsCount = hitPoints->GetSize();
	const BBox &hpBBox = hitPoints->GetBBox(passIndex);

	// Calculate the size of the grid cell
	const float maxPhotonRadius2 = hitPoints->GetMaxPhotonRaidus2(passIndex);
	const float cellSize = sqrtf(maxPhotonRadius2) * 2.f;
	LOG(LUX_INFO, LUX_NOERROR) << "Grid cell size: " << cellSize;
	invCellSize = 1.f / cellSize;

	maxGridIndexX = int((hpBBox.pMax.x - hpBBox.pMin.x) * invCellSize);
	maxGridIndexY = int((hpBBox.pMax.y - hpBBox.pMin.y) * invCellSize);
	maxGridIndexZ = int((hpBBox.pMax.z - hpBBox.pMin.z) * invCellSize);
	gridSizeX = maxGridIndexX + 1;
	gridSizeY = maxGridIndexY + 1;
	gridSizeZ = maxGridIndexZ + 1;
	gridSize = gridSizeX * gridSizeY * gridSizeZ;

	LOG(LUX_INFO, LUX_NOERROR) << "Grid size: (" <<
			gridSizeX << ", " << gridSizeY << ", " << gridSizeZ << ")";

	// TODO: add a tunable parameter for Grid size
	if (!grid) {
		grid = new std::list<HitPoint *>*[gridSize];

		for (unsigned int i = 0; i < gridSize; ++i)
			grid[i] = NULL;
	} else {
		for (unsigned int i = 0; i < gridSize; ++i) {
			delete grid[i];
			grid[i] = NULL;
		}
	}

	LOG(LUX_INFO, LUX_NOERROR) << "Building hit points grid";
	//unsigned int maxPathCount = 0;
	unsigned long long entryCount = 0;
	for (unsigned int i = 0; i < hitPointsCount; ++i) {
		HitPoint *hp = hitPoints->GetHitPoint(i);
		HitPointEyePass *hpep = &hp->eyePass[passIndex];

		if (hpep->type == SURFACE) {
			const float photonRadius = sqrtf(hp->accumPhotonRadius2);
			const Vector rad(photonRadius, photonRadius, photonRadius);
			const Vector bMin = ((hpep->position - rad) - hpBBox.pMin) * invCellSize;
			const Vector bMax = ((hpep->position + rad) - hpBBox.pMin) * invCellSize;

			const int ixMin = Clamp<int>(int(bMin.x), 0, maxGridIndexX);
			const int ixMax = Clamp<int>(int(bMax.x), 0, maxGridIndexX);
			const int iyMin = Clamp<int>(int(bMin.y), 0, maxGridIndexY);
			const int iyMax = Clamp<int>(int(bMax.y), 0, maxGridIndexY);
			const int izMin = Clamp<int>(int(bMin.z), 0, maxGridIndexZ);
			const int izMax = Clamp<int>(int(bMax.z), 0, maxGridIndexZ);

			for (int iz = izMin; iz <= izMax; iz++) {
				for (int iy = iyMin; iy <= iyMax; iy++) {
					for (int ix = ixMin; ix <= ixMax; ix++) {

						int hv = ReduceDims(ix, iy, iz);

						if (grid[hv] == NULL)
							grid[hv] = new std::list<HitPoint *>();

						grid[hv]->push_front(hp);
						++entryCount;

						/*// grid[hv]->size() is very slow to execute
						if (grid[hv]->size() > maxPathCount)
							maxPathCount = grid[hv]->size();*/
					}
				}
			}
		}
	}
	//std::cerr << "Max. hit points in a single hash grid entry: " << maxPathCount << std::endl;
	LOG(LUX_INFO, LUX_NOERROR) << "Total grid entry: " << entryCount;
	LOG(LUX_INFO, LUX_NOERROR) << "Avg. hit points in a single grid entry: " << entryCount / gridSize;

	/*// Grid debug code
	u_int badCells = 0;
	u_int emptyCells = 0;
	for (u_int i = 0; i < gridSize; ++i) {
		if (grid[i]) {
			if (grid[i]->size() > 5) {
				//std::cerr << "Grid[" << i << "].size() = " << grid[i]->size() << std::endl;
				++badCells;
			}
		} else
			++emptyCells;
	}
	std::cerr << "Grid.badCells = " << (100.f * badCells / (gridSize - emptyCells)) << "%" << std::endl;
	std::cerr << "Grid.emptyCells = " << (100.f * emptyCells / gridSize) << "%" << std::endl;*/
}

void GridLookUpAccel::AddFlux(const Point &hitPoint, const u_int passIndex, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group) {
	// Look for eye path hit points near the current hit point
	Vector hh = (hitPoint - hitPoints->GetBBox(passIndex).pMin) * invCellSize;
	const int ix = int(hh.x);
	if ((ix < 0) || (ix > maxGridIndexX))
			return;
	const int iy = int(hh.y);
	if ((iy < 0) || (iy > maxGridIndexY))
			return;
	const int iz = int(hh.z);
	if ((iz < 0) || (iz > maxGridIndexZ))
			return;

	std::list<HitPoint *> *hps = grid[ReduceDims(ix, iy, iz)];
	if (hps) {
		std::list<HitPoint *>::iterator iter = hps->begin();
		while (iter != hps->end()) {
			HitPoint *hp = *iter++;

			AddFluxToHitPoint(hp, passIndex, hitPoint, wi, sw, photonFlux, light_group);
		}
	}
}
