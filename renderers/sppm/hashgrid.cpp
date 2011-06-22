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

HashGrid::HashGrid(HitPoints *hps) {
	hitPoints = hps;
	grid = NULL;

	RefreshMutex(0);
}

HashGrid::~HashGrid() {
	for (unsigned int i = 0; i < gridSize; ++i)
		delete grid[i];
	delete[] grid;
}

void HashGrid::RefreshMutex(const u_int passIndex) {
	const unsigned int hitPointsCount = hitPoints->GetSize();
	const BBox &hpBBox = hitPoints->GetBBox(passIndex);

	// Calculate the size of the grid cell
	const float maxPhotonRadius2 = hitPoints->GetMaxPhotonRadius2(passIndex);
	const float cellSize = sqrtf(maxPhotonRadius2) * 2.f;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Hash grid cell size: " << cellSize;
	invCellSize = 1.f / cellSize;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Hash grid size: (" <<
			(hpBBox.pMax.x - hpBBox.pMin.x) * invCellSize << ", " <<
			(hpBBox.pMax.y - hpBBox.pMin.y) * invCellSize << ", " <<
			(hpBBox.pMax.z - hpBBox.pMin.z) * invCellSize << ")";

	// TODO: add a tunable parameter for hashgrid size
	gridSize = hitPointsCount;
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

	/*// HashGrid debug code
	int maxHashIndexX = int((hpBBox.pMax.x - hpBBox.pMin.x) * invCellSize);
	int maxHashIndexY = int((hpBBox.pMax.y - hpBBox.pMin.y) * invCellSize);
	int maxHashIndexZ = int((hpBBox.pMax.z - hpBBox.pMin.z) * invCellSize);
	u_int hMin = gridSize;
	u_int hMax = 0;
	u_int *hits = new u_int[gridSize];
	memset(hits, 0, sizeof(u_int) * gridSize);
	for (int iz = 0; iz <= maxHashIndexZ; ++iz) {
		for (int iy = 0; iy <= maxHashIndexY; ++iy) {
			for (int ix = 0; ix <= maxHashIndexX; ++ix) {
				u_int h = Hash(ix, iy, iz);
				hMin = min(h, hMin);
				hMax = max(h, hMax);

				hits[h] += 1;
			}
		}
	}
	std::cerr << "HashGrid.Hash.hMin = " << hMin << std::endl;
	std::cerr << "HashGrid.Hash.hMax = " << hMax << std::endl;
	for (int iz = 0; iz <= maxHashIndexZ; ++iz) {
		for (int iy = 0; iy <= maxHashIndexY; ++iy) {
			for (int ix = 0; ix <= maxHashIndexX; ++ix) {
				u_int h = Hash(ix, iy, iz);
				u_int count = hits[h];

				if (count > 1)
					std::cerr << "HashGrid.Hash.(" << ix << ", " << iy << ", " << iz << " => " << h << ") = " << count << std::endl;
			}
		}
	}*/

	LOG(LUX_DEBUG, LUX_NOERROR) << "Building hit points hash grid";
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

			for (int iz = abs(int(bMin.z)); iz <= abs(int(bMax.z)); ++iz) {
				for (int iy = abs(int(bMin.y)); iy <= abs(int(bMax.y)); ++iy) {
					for (int ix = abs(int(bMin.x)); ix <= abs(int(bMax.x)); ++ix) {
						int hv = Hash(ix, iy, iz);

						if (grid[hv] == NULL)
							grid[hv] = new std::list<HitPoint *>();

						grid[hv]->push_front(hp);
						++entryCount;

						/* Too slow:
						if (grid[hv] == NULL) {
							grid[hv] = new std::list<HitPoint *>();

							grid[hv]->push_front(hp);
							++entryCount;
						} else {
							// Check if the hit point has been already inserted
							std::list<HitPoint *>::iterator iter = grid[hv]->begin();
							bool found = false;
							while (iter != grid[hv]->end()) {
								HitPoint *lhp = *iter++;

								if (lhp == hp)
									found = true;
							}
							if (found)
								continue;

							grid[hv]->push_front(hp);
							++entryCount;

							// grid[hv]->size() is very slow to execute
							//if (grid[hv]->size() > maxPathCount)
							//	maxPathCount = grid[hv]->size();
						}*/
					}
				}
			}
		}
	}

	//std::cerr << "Max. hit points in a single hash grid entry: " << maxPathCount << std::endl;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Total hash grid entry: " << entryCount;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Avg. hit points in a single hash grid entry: " << entryCount / gridSize;

	/*// HashGrid debug code
	u_int badCells = 0;
	u_int emptyCells = 0;
	for (u_int i = 0; i < gridSize; ++i) {
		if (grid[i]) {
			if (grid[i]->size() > 5) {
				//std::cerr << "HashGrid[" << i << "].size() = " << grid[i]->size() << std::endl;
				++badCells;
			}
		} else
			++emptyCells;
	}
	std::cerr << "HashGrid.badCells = " << (100.f * badCells / (gridSize - emptyCells)) << "%" << std::endl;
	std::cerr << "HashGrid.emptyCells = " << (100.f * emptyCells / gridSize) << "%" << std::endl;*/
}

void HashGrid::AddFlux(const Point &hitPoint, const u_int passIndex, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int lightGroup) {
	// Look for eye path hit points near the current hit point
	Vector hh = (hitPoint - hitPoints->GetBBox(passIndex).pMin) * invCellSize;
	const int ix = abs(int(hh.x));
	const int iy = abs(int(hh.y));
	const int iz = abs(int(hh.z));

	std::list<HitPoint *> *hps = grid[Hash(ix, iy, iz)];

	if (hps) {
		std::list<HitPoint *>::iterator iter = hps->begin();
		while (iter != hps->end()) {
			HitPoint *hp = *iter++;

			AddFluxToHitPoint(hp, passIndex, hitPoint, wi, sw, photonFlux, lightGroup);
		}
	}
}

void HashGrid::AddFlux(SplatList *splatList, const Point &hitPoint, const u_int passIndex, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int lightGroup) {
	// Look for eye path hit points near the current hit point
	Vector hh = (hitPoint - hitPoints->GetBBox(passIndex).pMin) * invCellSize;
	const int ix = abs(int(hh.x));
	const int iy = abs(int(hh.y));
	const int iz = abs(int(hh.z));

	std::list<HitPoint *> *hps = grid[Hash(ix, iy, iz)];
	if (hps) {
		std::list<HitPoint *>::iterator iter = hps->begin();
		while (iter != hps->end()) {
			HitPoint *hp = *iter++;

			AddFluxToSplatList(splatList, hp, passIndex, hitPoint, wi, sw, photonFlux, lightGroup);
		}
	}
}
