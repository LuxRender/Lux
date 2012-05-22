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

HybridHashGrid::HybridHashGrid(HitPoints *hps): HitPointsLookUpAccel(hps) {
	grid = NULL;
	kdtreeThreshold = 2;
}

HybridHashGrid::~HybridHashGrid() {
	for (unsigned int i = 0; i < gridSize; ++i)
		delete grid[i];
	delete[] grid;
}

void HybridHashGrid::RefreshMutex() {
	const unsigned int hitPointsCount = hitPoints->GetSize();
	const BBox &hpBBox = hitPoints->GetBBox();

	// Calculate the size of the grid cell
	const float maxPhotonRadius2 = hitPoints->GetMaxPhotonRadius2();
	const float cellSize = sqrtf(maxPhotonRadius2) * 2.f;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Hybrid hash grid cell size: " << cellSize;
	invCellSize = 1.f / cellSize;
	maxHashIndexX = int((hpBBox.pMax.x - hpBBox.pMin.x) * invCellSize);
	maxHashIndexY = int((hpBBox.pMax.y - hpBBox.pMin.y) * invCellSize);
	maxHashIndexZ = int((hpBBox.pMax.z - hpBBox.pMin.z) * invCellSize);
	LOG(LUX_DEBUG, LUX_NOERROR) << "Hybrid hash grid cell count: (" << maxHashIndexX << ", " <<
			maxHashIndexY << ", " << maxHashIndexZ << ")";

	// TODO: add a tunable parameter for HybridHashGrid size
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

	LOG(LUX_DEBUG, LUX_NOERROR) << "Building hit points hybrid hash grid";
	unsigned int maxPathCount = 0;
	unsigned long long entryCount = 0;
	for (unsigned int i = 0; i < hitPointsCount; ++i) {
		HitPoint *hp = hitPoints->GetHitPoint(i);

		if (hp->IsSurface()) {
			const float photonRadius = sqrtf(hp->accumPhotonRadius2);
			const Vector rad(photonRadius, photonRadius, photonRadius);
			const Vector bMin = ((hp->GetPosition() - rad) - hpBBox.pMin) * invCellSize;
			const Vector bMax = ((hp->GetPosition() + rad) - hpBBox.pMin) * invCellSize;

			const int ixMin = Clamp<int>(int(bMin.x), 0, maxHashIndexX);
			const int ixMax = Clamp<int>(int(bMax.x), 0, maxHashIndexX);
			const int iyMin = Clamp<int>(int(bMin.y), 0, maxHashIndexY);
			const int iyMax = Clamp<int>(int(bMax.y), 0, maxHashIndexY);
			const int izMin = Clamp<int>(int(bMin.z), 0, maxHashIndexZ);
			const int izMax = Clamp<int>(int(bMax.z), 0, maxHashIndexZ);

			for (int iz = izMin; iz <= izMax; iz++) {
				for (int iy = iyMin; iy <= iyMax; iy++) {
					for (int ix = ixMin; ix <= ixMax; ix++) {
						int hv = Hash(ix, iy, iz);

						if (grid[hv] == NULL)
							grid[hv] = new HashCell(HH_LIST);

						grid[hv]->AddList(hp);
						++entryCount;

						if (grid[hv]->GetSize() > maxPathCount)
							maxPathCount = grid[hv]->GetSize();
					}
				}
			}
		}
	}
	LOG(LUX_DEBUG, LUX_NOERROR) << "Max. hit points in a single hybrid hash grid entry: " << maxPathCount;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Total hash grid entry: " << entryCount;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Avg. hit points in a single hybrid hash grid entry: " << entryCount / gridSize;

	// Debug code
	/*unsigned int nullCount = 0;
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
}

void HybridHashGrid::ConvertKdTree(scheduling::Range *range)
{
	unsigned int HHGKdTreeEntries = 0;
	unsigned int HHGlistEntries = 0;

	for(unsigned i = range->begin();
			i != range->end();
			i = range->next()) {
		HashCell *hc = grid[i];

		if (hc && hc->GetSize() > kdtreeThreshold) {
			hc->TransformToKdTree();
			++HHGKdTreeEntries;
		} else
			++HHGlistEntries;
	}
	LOG(LUX_DEBUG, LUX_NOERROR) << "Hybrid hash cells storing a HHGKdTree: " << HHGKdTreeEntries << "/" << HHGlistEntries;

	// HybridHashGrid debug code
	/*for (unsigned int i = 0; i < HybridHashGridSize; ++i) {
		if (HybridHashGrid[i]) {
			if (HybridHashGrid[i]->size() > 10) {
				std::cerr << "HybridHashGrid[" << i << "].size() = " <<HybridHashGrid[i]->size() << std::endl;
			}
		}
	}*/
}

void HybridHashGrid::Refresh(scheduling::Scheduler *scheduler) {
	RefreshMutex();

	if (gridSize == 0)
		return;

	scheduler->Launch(boost::bind(&HybridHashGrid::ConvertKdTree, this, _1), 0, gridSize);
}

void HybridHashGrid::AddFlux(Sample& sample, const PhotonData &photon) {
	// Look for eye path hit points near the current hit point
	Vector hh = (photon.p - hitPoints->GetBBox().pMin) * invCellSize;
	const int ix = int(hh.x);
	if ((ix < 0) || (ix > maxHashIndexX))
			return;
	const int iy = int(hh.y);
	if ((iy < 0) || (iy > maxHashIndexY))
			return;
	const int iz = int(hh.z);
	if ((iz < 0) || (iz > maxHashIndexZ))
			return;

	HashCell *hc = grid[Hash(ix, iy, iz)];
	if (hc)
		hc->AddFlux(sample, this, photon);
}
