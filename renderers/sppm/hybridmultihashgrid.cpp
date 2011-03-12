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

HybridMultiHashGrid::HybridMultiHashGrid(HitPoints *hps) {
	hitPoints = hps;
	grid = NULL;
	kdtreeThreshold = 2;

	RefreshMutex(0);
}

HybridMultiHashGrid::~HybridMultiHashGrid() {
	for (unsigned int i = 0; i < gridSize; ++i)
		delete grid[i];
	delete[] grid;
}

void HybridMultiHashGrid::RefreshMutex(const u_int passIndex) {
	const unsigned int hitPointsCount = hitPoints->GetSize();
	const BBox &hpBBox = hitPoints->GetBBox(passIndex);

	// Calculate the size of the grid cell
	const float maxPhotonRadius2 = hitPoints->GetMaxPhotonRaidus2(passIndex);
	const float cellSize = sqrtf(maxPhotonRadius2) * 2.f;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Hybrid multi-hash grid cell size: " << cellSize;
	invCellSize = 1.f / cellSize;
	maxHashIndexX = int((hpBBox.pMax.x - hpBBox.pMin.x) * invCellSize);
	maxHashIndexY = int((hpBBox.pMax.y - hpBBox.pMin.y) * invCellSize);
	maxHashIndexZ = int((hpBBox.pMax.z - hpBBox.pMin.z) * invCellSize);
	LOG(LUX_DEBUG, LUX_NOERROR) << "Hybrid multi-hash grid cell count: (" << maxHashIndexX << ", " <<
			maxHashIndexY << ", " << maxHashIndexZ << ")";

	// TODO: add a tunable parameter for HybridMultiHashGrid size
	gridSize = hitPointsCount;
	if (!grid) {
		grid = new HashCell*[gridSize];

		for (unsigned int i = 0; i < gridSize; ++i)
			grid[i] = NULL;
	} else {
		for (unsigned int i = 0; i < gridSize; ++i) {
			delete grid[i];
			grid[i] = NULL;
		}
	}

	LOG(LUX_DEBUG, LUX_NOERROR) << "Building hit points hybrid multi-hash grid";
	unsigned int maxPathCount = 0;
	unsigned long long entryCount = 0;
	for (unsigned int i = 0; i < hitPointsCount; ++i) {
		HitPoint *hp = hitPoints->GetHitPoint(i);
		HitPointEyePass *hpep = &hp->eyePass[passIndex];

		if (hpep->type == SURFACE) {
			const float photonRadius = sqrtf(hp->accumPhotonRadius2);
			const Vector rad(photonRadius, photonRadius, photonRadius);
			const Vector bMin = ((hpep->position - rad) - hpBBox.pMin) * invCellSize;
			const Vector bMax = ((hpep->position + rad) - hpBBox.pMin) * invCellSize;

			const int ixMin = Clamp<int>(int(bMin.x), 0, maxHashIndexX);
			const int ixMax = Clamp<int>(int(bMax.x), 0, maxHashIndexX);
			const int iyMin = Clamp<int>(int(bMin.y), 0, maxHashIndexY);
			const int iyMax = Clamp<int>(int(bMax.y), 0, maxHashIndexY);
			const int izMin = Clamp<int>(int(bMin.z), 0, maxHashIndexZ);
			const int izMax = Clamp<int>(int(bMax.z), 0, maxHashIndexZ);

			for (int iz = izMin; iz <= izMax; iz++) {
				for (int iy = iyMin; iy <= iyMax; iy++) {
					for (int ix = ixMin; ix <= ixMax; ix++) {
						int hv = Hash1(ix, iy, iz);
						HashCell *hc = grid[hv];

						if (hc == NULL)
							hc = grid[hv] = new HashCell(HH_LIST);
						else {
							hv = Hash2(ix, iy, iz);
							hc = grid[hv];

							if (hc == NULL)
								hc = grid[hv] = new HashCell(HH_LIST);
							/*else {
								hv = Hash3(ix, iy, iz);
								hc = grid[hv];

								if (hc == NULL)
									hc = grid[hv] = new HashCell(HH_LIST);
							}*/
						}

						hc->AddList(hp);
						++entryCount;

						if (hc->GetSize() > maxPathCount)
							maxPathCount = hc->GetSize();
					}
				}
			}
		}
	}
	LOG(LUX_DEBUG, LUX_NOERROR) << "Max. hit points in a single hybrid multi-hash grid entry: " << maxPathCount;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Total multihash grid entry: " << entryCount;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Avg. hit points in a single hybrid multi-hash grid entry: " << entryCount / gridSize;

	/*// Debug code
	unsigned int nullCount = 0;
	for (unsigned int i = 0; i < gridSize; ++i) {
		HashCell *hc = grid[i];

		if (!hc)
			++nullCount;
	}
	std::cerr << "NULL count: " << nullCount << "/" << gridSize << "(" << 100.0 * nullCount / gridSize << "%)" << std::endl;
	for (unsigned int j = 1; j <= maxPathCount; ++j) {
		unsigned int count = 0;
		for (unsigned int i = 0; i < gridSize; ++i) {
			HashCell *hc = grid[i];

			if (hc && hc->GetSize() == j)
				++count;
		}
		std::cerr << j << " count: " << count << "/" << gridSize << "(" << 100.0 * count / gridSize << "%)" << std::endl;
	}*/

	/*// Debug code
	u_int badCells = 0;
	u_int emptyCells = 0;
	for (u_int i = 0; i < gridSize; ++i) {
		if (grid[i]) {
			if (grid[i]->GetSize() > 5) {
				//std::cerr << "HashGrid[" << i << "].size() = " << grid[i]->size() << std::endl;
				++badCells;
			}
		} else
			++emptyCells;
	}
	std::cerr << "HashMultiGrid.badCells = " << (100.f * badCells / (gridSize - emptyCells)) << "%" << std::endl;
	std::cerr << "HashMultiGrid.emptyCells = " << (100.f * emptyCells / gridSize) << "%" << std::endl;*/
}

void HybridMultiHashGrid::RefreshParallel(const u_int passIndex, const unsigned int index, const unsigned int count) {
	if (gridSize == 0)
		return;

	// Calculate the index of work this thread has to do
	const unsigned int workSize = gridSize / count;
	const unsigned int first = workSize * index;
	const unsigned int last = (index == count - 1) ? gridSize : (first + workSize);
	assert (first >= 0);
	assert (last <= gridSize);

	unsigned int HHGKdTreeEntries = 0;
	unsigned int HHGlistEntries = 0;
	for (unsigned int i = first; i < last; ++i) {
		HashCell *hc = grid[i];

		if (hc && hc->GetSize() > kdtreeThreshold) {
			hc->TransformToKdTree(passIndex);
			++HHGKdTreeEntries;
		} else
			++HHGlistEntries;
	}
	LOG(LUX_DEBUG, LUX_NOERROR) << "Hybrid multi-hash cells storing a HHGKdTree: " << HHGKdTreeEntries << "/" << HHGlistEntries;

	// HybridMultiHashGrid debug code
	/*for (unsigned int i = 0; i < HybridHashGridSize; ++i) {
		if (HybridMultiHashGrid[i]) {
			if (HybridMultiHashGrid[i]->size() > 10) {
				std::cerr << "HybridMultiHashGrid[" << i << "].size() = " <<HybridMultiHashGrid[i]->size() << std::endl;
			}
		}
	}*/
}

void HybridMultiHashGrid::AddFlux(const Point &hitPoint, const u_int passIndex, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group) {
	// Look for eye path hit points near the current hit point
	Vector hh = (hitPoint - hitPoints->GetBBox(passIndex).pMin) * invCellSize;
	const int ix = int(hh.x);
	if ((ix < 0) || (ix > maxHashIndexX))
			return;
	const int iy = int(hh.y);
	if ((iy < 0) || (iy > maxHashIndexY))
			return;
	const int iz = int(hh.z);
	if ((iz < 0) || (iz > maxHashIndexZ))
			return;

	HashCell *hc = grid[Hash1(ix, iy, iz)];
	if (hc)
		hc->AddFlux(this, passIndex, hitPoint, wi, sw, photonFlux, light_group);
	else
		return;

	HashCell *hc2 = grid[Hash2(ix, iy, iz)];
	if (hc2)
		hc2->AddFlux(this, passIndex, hitPoint, wi, sw, photonFlux, light_group);
	/*else
		return;

	HashCell *hc3 = grid[Hash3(ix, iy, iz)];
	if (hc3)
		hc3->AddFlux(this, passIndex, hitPoint, wi, sw, photonFlux, light_group);
	 */
}
