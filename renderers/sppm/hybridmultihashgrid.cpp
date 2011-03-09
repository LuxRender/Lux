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

	RefreshMutex();
}

HybridMultiHashGrid::~HybridMultiHashGrid() {
	for (unsigned int i = 0; i < gridSize; ++i)
		delete grid[i];
	delete[] grid;
}

void HybridMultiHashGrid::RefreshMutex() {
	const unsigned int hitPointsCount = hitPoints->GetSize();
	const BBox &hpBBox = hitPoints->GetBBox();

	// Calculate the size of the grid cell
	const float maxPhotonRadius2 = hitPoints->GetMaxPhotonRaidus2();
	const float cellSize = sqrtf(maxPhotonRadius2) * 2.f;
	LOG(LUX_INFO, LUX_NOERROR) << "Hybrid multihash grid cell size: " << cellSize;
	invCellSize = 1.f / cellSize;
	maxHashIndexX = int((hpBBox.pMax.x - hpBBox.pMin.x) * invCellSize);
	maxHashIndexY = int((hpBBox.pMax.y - hpBBox.pMin.y) * invCellSize);
	maxHashIndexZ = int((hpBBox.pMax.z - hpBBox.pMin.z) * invCellSize);
	LOG(LUX_INFO, LUX_NOERROR) << "Hybrid multihash grid cell count: (" << maxHashIndexX << ", " <<
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

	LOG(LUX_INFO, LUX_NOERROR) << "Building hit points hybrid multihash grid";
	unsigned int maxPathCount = 0;
	unsigned long long entryCount = 0;
	for (unsigned int i = 0; i < hitPointsCount; ++i) {
		HitPoint *hp = hitPoints->GetHitPoint(i);

		if (hp->type == SURFACE) {
			const float photonRadius = sqrtf(hp->accumPhotonRadius2);
			const Vector rad(photonRadius, photonRadius, photonRadius);
			const Vector bMin = ((hp->position - rad) - hpBBox.pMin) * invCellSize;
			const Vector bMax = ((hp->position + rad) - hpBBox.pMin) * invCellSize;

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
							hc = grid[hv] = new HashCell(LIST);
						else {
							hv = Hash2(ix, iy, iz);
							hc = grid[hv];

							if (hc == NULL)
								hc = grid[hv] = new HashCell(LIST);
							/*else {
								hv = Hash3(ix, iy, iz);
								hc = grid[hv];

								if (hc == NULL)
									hc = grid[hv] = new HashCell(LIST);
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
	LOG(LUX_INFO, LUX_NOERROR) << "Max. hit points in a single hybrid multihash grid entry: " << maxPathCount;
	LOG(LUX_INFO, LUX_NOERROR) << "Total multihash grid entry: " << entryCount;
	LOG(LUX_INFO, LUX_NOERROR) << "Avg. hit points in a single hybrid multihash grid entry: " << entryCount / gridSize;

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

void HybridMultiHashGrid::RefreshParallel(const unsigned int index, const unsigned int count) {
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
			hc->TransformToKdTree();
			++HHGKdTreeEntries;
		} else
			++HHGlistEntries;
	}
	LOG(LUX_INFO, LUX_NOERROR) << "Hybrid hash cells storing a HHGKdTree: " << HHGKdTreeEntries << "/" << HHGlistEntries;

	// HybridMultiHashGrid debug code
	/*for (unsigned int i = 0; i < HybridHashGridSize; ++i) {
		if (HybridMultiHashGrid[i]) {
			if (HybridMultiHashGrid[i]->size() > 10) {
				std::cerr << "HybridMultiHashGrid[" << i << "].size() = " <<HybridMultiHashGrid[i]->size() << std::endl;
			}
		}
	}*/
}

void HybridMultiHashGrid::AddFlux(const Point &hitPoint, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group) {
	// Look for eye path hit points near the current hit point
	Vector hh = (hitPoint - hitPoints->GetBBox().pMin) * invCellSize;
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
		hc->AddFlux(this, hitPoint, wi, sw, photonFlux, light_group);
	else
		return;

	HashCell *hc2 = grid[Hash2(ix, iy, iz)];
	if (hc2)
		hc2->AddFlux(this, hitPoint, wi, sw, photonFlux, light_group);
	/*else
		return;

	HashCell *hc3 = grid[Hash3(ix, iy, iz)];
	if (hc3)
		hc3->AddFlux(this, hitPoint, wi, sw, photonFlux, light_group);
	 */
}

void HybridMultiHashGrid::HashCell::AddFlux(HybridMultiHashGrid *hhg, const Point &hitPoint,
		const Vector &wi, const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group) {
	switch (type) {
		case LIST: {
			std::list<HitPoint *>::iterator iter = list->begin();
			while (iter != list->end()) {
				HitPoint *hp = *iter++;
				hhg->AddFluxToHitPoint(hp, hitPoint, wi, sw, photonFlux, light_group);
			}
			break;
		}
		case KD_TREE: {
			kdtree->AddFlux(hhg, hitPoint, wi, sw, photonFlux, light_group);
			break;
		}
		default:
			assert (false);
	}
}

HybridMultiHashGrid::HHGKdTree::HHGKdTree(std::list<HitPoint *> *hps, const unsigned int count) {
	nNodes = count;
	nextFreeNode = 1;

	//std::cerr << "Building kD-Tree with " << nNodes << " nodes" << std::endl;

	nodes = new KdNode[nNodes];
	nodeData = new HitPoint*[nNodes];
	nextFreeNode = 1;

	// Begin the HHGKdTree building process
	std::vector<HitPoint *> buildNodes;
	buildNodes.reserve(nNodes);
	maxDistSquared = 0.f;
	std::list<HitPoint *>::iterator iter = hps->begin();
	for (unsigned int i = 0; i < nNodes; ++i)  {
		buildNodes.push_back(*iter++);
		maxDistSquared = max<float>(maxDistSquared, buildNodes[i]->accumPhotonRadius2);
	}
	//std::cerr << "kD-Tree search radius: " << sqrtf(maxDistSquared) << std::endl;

	RecursiveBuild(0, 0, nNodes, buildNodes);
	assert (nNodes == nextFreeNode);
}

HybridMultiHashGrid::HHGKdTree::~HHGKdTree() {
	delete[] nodes;
	delete[] nodeData;
}

bool HybridMultiHashGrid::HHGKdTree::CompareNode::operator ()(const HitPoint *d1, const HitPoint *d2) const {
	return (d1->position[axis] == d2->position[axis]) ? (d1 < d2) :
			(d1->position[axis] < d2->position[axis]);
}

void HybridMultiHashGrid::HHGKdTree::RecursiveBuild(const unsigned int nodeNum, const unsigned int start,
		const unsigned int end, std::vector<HitPoint *> &buildNodes) {
	assert (nodeNum >= 0);
	assert (start >= 0);
	assert (end >= 0);
	assert (nodeNum < nNodes);
	assert (start < nNodes);
	assert (end <= nNodes);
	assert (start < end);

	// Create leaf node of kd-tree if we've reached the bottom
	if (start + 1 == end) {
		nodes[nodeNum].initLeaf();
		nodeData[nodeNum] = buildNodes[start];
		return;
	}

	// Choose split direction and partition data
	// Compute bounds of data from start to end
	BBox bound;
	for (unsigned int i = start; i < end; ++i)
		bound = Union(bound, buildNodes[i]->position);
	unsigned int splitAxis = bound.MaximumExtent();
	unsigned int splitPos = (start + end) / 2;

	std::nth_element(buildNodes.begin() + start, buildNodes.begin() + splitPos,
		buildNodes.begin() + end, CompareNode(splitAxis));

	// Allocate kd-tree node and continue recursively
	nodes[nodeNum].init(buildNodes[splitPos]->position[splitAxis], splitAxis);
	nodeData[nodeNum] = buildNodes[splitPos];

	if (start < splitPos) {
		nodes[nodeNum].hasLeftChild = 1;
		const unsigned int childNum = nextFreeNode++;
		RecursiveBuild(childNum, start, splitPos, buildNodes);
	}

	if (splitPos + 1 < end) {
		nodes[nodeNum].rightChild = nextFreeNode++;
		RecursiveBuild(nodes[nodeNum].rightChild, splitPos + 1, end, buildNodes);
	}
}

void HybridMultiHashGrid::HHGKdTree::AddFlux(HybridMultiHashGrid *hhg, const Point &p,
		const Vector &wi, const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, u_int light_group) {
	unsigned int nodeNumStack[64];
	// Start from the first node
	nodeNumStack[0] = 0;
	int stackIndex = 0;

	while (stackIndex >= 0) {
		const unsigned int nodeNum = nodeNumStack[stackIndex--];
		KdNode *node = &nodes[nodeNum];

		const int axis = node->splitAxis;
		if (axis != 3) {
			const float dist = p[axis] - node->splitPos;
			const float dist2 = dist * dist;
			if (p[axis] <= node->splitPos) {
				if ((dist2 < maxDistSquared) && (node->rightChild < nNodes))
					nodeNumStack[++stackIndex] = node->rightChild;
				if (node->hasLeftChild)
					nodeNumStack[++stackIndex] = nodeNum + 1;
			} else {
				if (node->rightChild < nNodes)
					nodeNumStack[++stackIndex] = node->rightChild;
				if ((dist2 < maxDistSquared) && (node->hasLeftChild))
					nodeNumStack[++stackIndex] = nodeNum + 1;
			}
		}

		// Process the leaf
		HitPoint *hp = nodeData[nodeNum];
		hhg->AddFluxToHitPoint(hp, p, wi, sw, photonFlux, light_group);
	}
}
