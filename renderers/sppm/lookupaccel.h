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

#ifndef LUX_LOOKUPACCEL_H
#define	LUX_LOOKUPACCEL_H

#include "osfunc.h"

namespace lux
{

//------------------------------------------------------------------------------
// Hit points look up accelerators
//------------------------------------------------------------------------------

class HitPoint;
class HitPoints;

enum LookUpAccelType {
	HASH_GRID, KD_TREE, HYBRID_HASH_GRID, STOCHASTIC_HASH_GRID, GRID,
	CUCKOO_HASH_GRID
};

class HitPointsLookUpAccel {
public:
	HitPointsLookUpAccel() { }
	virtual ~HitPointsLookUpAccel() { }

	virtual void RefreshMutex() = 0;
	virtual void RefreshParallel(const unsigned int index, const unsigned int count) { }

	virtual void AddFlux(const Point &hitPoint, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group) = 0;

protected:
	void AddFluxToHitPoint(HitPoint *hp,
		const Point &hitPoint, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group);
};

inline void SpectrumAtomicAdd(SWCSpectrum &s, SWCSpectrum &a) {
	for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
		osAtomicAdd(&s.c[i], a.c[i]);
}

inline void XYZColorAtomicAdd(XYZColor &s, XYZColor &a) {
	for (int i = 0; i < COLOR_SAMPLES; ++i)
		osAtomicAdd(&s.c[i], a.c[i]);
}

//------------------------------------------------------------------------------
// HashGrid accelerator
//------------------------------------------------------------------------------

class HashGrid : public HitPointsLookUpAccel {
public:
	HashGrid(HitPoints *hps);

	~HashGrid();

	void RefreshMutex();

	void AddFlux(const Point &hitPoint, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group);

private:
	unsigned int Hash(const int ix, const int iy, const int iz) {
		return (unsigned int)((ix * 73856093) ^ (iy * 19349663) ^ (iz * 83492791)) % gridSize;
	}
	/*unsigned int Hash(const int ix, const int iy, const int iz) {
		return (unsigned int)((ix * 997 + iy) * 443 + iz) % gridSize;
	}*/

	HitPoints *hitPoints;
	unsigned int gridSize;
	float invCellSize;
	std::list<HitPoint *> **grid;
};

//------------------------------------------------------------------------------
// Grid accelerator
//------------------------------------------------------------------------------

class GridLookUpAccel : public HitPointsLookUpAccel {
public:
	GridLookUpAccel(HitPoints *hps);

	~GridLookUpAccel();

	void RefreshMutex();

	void AddFlux(const Point &hitPoint, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group);

private:
	unsigned int ReduceDims(const int ix, const int iy, const int iz) {
		return ix + iy * gridSizeX + iz * gridSizeX * gridSizeY;
	}

	HitPoints *hitPoints;
	u_int gridSize, gridSizeX, gridSizeY, gridSizeZ;
	int maxGridIndexX, maxGridIndexY, maxGridIndexZ;
	float invCellSize;
	std::list<HitPoint *> **grid;
};

//------------------------------------------------------------------------------
// StochasticHashGrid accelerator
//------------------------------------------------------------------------------

class StochasticHashGrid : public HitPointsLookUpAccel {
public:
	StochasticHashGrid(HitPoints *hps);

	~StochasticHashGrid();

	void RefreshMutex();

	void AddFlux(const Point &hitPoint, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group);

private:
	struct GridCell {
		HitPoint *hitPoint;
		u_int count;
	};

	unsigned int Hash(const int ix, const int iy, const int iz) {
		return (unsigned int)((ix * 73856093) ^ (iy * 19349663) ^ (iz * 83492791)) % gridSize;
	}

	HitPoints *hitPoints;
	unsigned int gridSize;
	float invCellSize;
	GridCell *grid;
};

//------------------------------------------------------------------------------
// CuckooHashGrid accelerator
//------------------------------------------------------------------------------

class CuckooHashGrid : public HitPointsLookUpAccel {
public:
	CuckooHashGrid(HitPoints *hps);

	~CuckooHashGrid();

	void RefreshMutex();

	void AddFlux(const Point &hitPoint, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group);

private:
	struct GridCell {
		int ix, iy, iz;
		HitPoint *hitPoint;
	};

	void Insert(const int ix, const int iy, const int iz, HitPoint *hitPoint);

	unsigned int Hash1(const int ix, const int iy, const int iz) {
		return (unsigned int)((ix * 73856093) ^ (iy * 19349663) ^ (iz * 83492791)) % gridSize;
	}
	unsigned int Hash2(const int ix, const int iy, const int iz) {
		return (unsigned int)((ix * 49979693) ^ (iy * 86028157) ^ (iz * 15485867)) % gridSize;
	}
	unsigned int Hash3(const int ix, const int iy, const int iz) {
		return (unsigned int)((ix * 15485863) ^ (iy * 49979687) ^ (iz * 32452867)) % gridSize;
	}
	/*unsigned int Hash4(const int ix, const int iy, const int iz) {
		return (unsigned int)((ix * 67867979) ^ (iy * 32452843) ^ (iz * 67867967)) % gridSize;
	}*/

	HitPoints *hitPoints;
	unsigned int gridSize;
	float invCellSize;

	GridCell *grid1;
	GridCell *grid2;
	GridCell *grid3;
};

//------------------------------------------------------------------------------
// KdTree accelerator
//------------------------------------------------------------------------------

class KdTree : public HitPointsLookUpAccel {
public:
	KdTree(HitPoints *hps);

	~KdTree();

	void RefreshMutex();

	void AddFlux(const Point &hitPoint,  const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group);

private:
	struct KdNode {
		void init(const float p, const unsigned int a) {
			splitPos = p;
			splitAxis = a;
			// Dade - in order to avoid a gcc warning
			rightChild = 0;
			rightChild = ~rightChild;
			hasLeftChild = 0;
		}

		void initLeaf() {
			splitAxis = 3;
			// Dade - in order to avoid a gcc warning
			rightChild = 0;
			rightChild = ~rightChild;
			hasLeftChild = 0;
		}

		// KdNode Data
		float splitPos;
		unsigned int splitAxis : 2;
		unsigned int hasLeftChild : 1;
		unsigned int rightChild : 29;
	};

	struct CompareNode {
		CompareNode(int a) { axis = a; }

		int axis;

		bool operator()(const HitPoint *d1, const HitPoint *d2) const;
	};

	void RecursiveBuild(const unsigned int nodeNum, const unsigned int start,
		const unsigned int end, std::vector<HitPoint *> &buildNodes);

	HitPoints *hitPoints;

	KdNode *nodes;
	HitPoint **nodeData;
	unsigned int nNodes, nextFreeNode;
	float maxDistSquared;
};

//------------------------------------------------------------------------------
// HybridHashGrid accelerator
//------------------------------------------------------------------------------

class HybridHashGrid : public HitPointsLookUpAccel {
public:
	HybridHashGrid(HitPoints *hps);

	~HybridHashGrid();

	void RefreshMutex();
	void RefreshParallel(const unsigned int index, const unsigned int count);

	void AddFlux(const Point &hitPoint, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group);

private:
	unsigned int Hash(const int ix, const int iy, const int iz) {
		return (unsigned int)((ix * 73856093) ^ (iy * 19349663) ^ (iz * 83492791)) % gridSize;
	}

	class HHGKdTree {
	public:
		HHGKdTree(std::list<HitPoint *> *hps, const unsigned int count);
		~HHGKdTree();

		void AddFlux(HybridHashGrid *hhg, const Point &hitPoint, const Vector &wi,
			const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group);

	private:
		struct KdNode {
			void init(const float p, const unsigned int a) {
				splitPos = p;
				splitAxis = a;
				// Dade - in order to avoid a gcc warning
				rightChild = 0;
				rightChild = ~rightChild;
				hasLeftChild = 0;
			}

			void initLeaf() {
				splitAxis = 3;
				// Dade - in order to avoid a gcc warning
				rightChild = 0;
				rightChild = ~rightChild;
				hasLeftChild = 0;
			}

			// KdNode Data
			float splitPos;
			unsigned int splitAxis : 2;
			unsigned int hasLeftChild : 1;
			unsigned int rightChild : 29;
		};

		struct CompareNode {
			CompareNode(int a) {
				axis = a;
			}

			int axis;

			bool operator()(const HitPoint *d1, const HitPoint * d2) const;
		};

		void RecursiveBuild(const unsigned int nodeNum, const unsigned int start,
				const unsigned int end, std::vector<HitPoint *> &buildNodes);

		KdNode *nodes;
		HitPoint **nodeData;
		unsigned int nNodes, nextFreeNode;
		float maxDistSquared;
	};

	enum HashCellType {
		LIST, KD_TREE
	};

	class HashCell {
	public:
		HashCell(const HashCellType t) {
			type = LIST;
			size = 0;
			list = new std::list<HitPoint *>();
		}
		~HashCell() {
			switch (type) {
				case LIST:
					delete list;
					break;
				case KD_TREE:
					delete kdtree;
					break;
				default:
					assert (false);
			}
		}

		void AddList(HitPoint *hp) {
			assert (type == LIST);

			/* Too slow:
			// Check if the hit point has been already inserted
			std::list<HitPoint *>::iterator iter = list->begin();
			while (iter != list->end()) {
				HitPoint *lhp = *iter++;

				if (lhp == hp)
					return;
			}*/

			list->push_front(hp);
			++size;
		}

		void TransformToKdTree() {
			assert (type == LIST);

			std::list<HitPoint *> *hplist = list;
			kdtree = new HHGKdTree(hplist, size);
			delete hplist;
			type = KD_TREE;
		}

		void AddFlux(HybridHashGrid *hhg, const Point &hitPoint, const Vector &wi,
			const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group);

		unsigned int GetSize() const { return size; }

	private:
		HashCellType type;
		unsigned int size;
		union {
			std::list<HitPoint *> *list;
			HHGKdTree *kdtree;
		};
	};

	unsigned int kdtreeThreshold;
	HitPoints *hitPoints;
	unsigned int gridSize;
	float invCellSize;
	int maxHashIndexX, maxHashIndexY, maxHashIndexZ;
	HashCell **grid;
};

}//namespace lux

#endif	/* LUX_LOOKUPACCEL_H */
