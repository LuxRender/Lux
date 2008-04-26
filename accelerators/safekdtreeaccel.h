/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

#ifndef LUX_SAFEKDTREEACCEL_H
#define LUX_SAFEKDTREEACCEL_H

// safekdtree.cpp*
#include "lux.h"
#include "primitive.h"
#include "memory.h"
#include "kdtreeaccel.h"

namespace lux
{

struct SafeKdAccelNode {
	// SafeKdAccelNode Methods
	void initLeaf(int *primNums, int np,
			Primitive **prims, MemoryArena &arena) {
		// Update kd leaf node allocation statistics
		// radiance - disabled for threading // static StatsCounter numLeafMade("Kd-Tree Accelerator","Leaf kd-tree nodes made");
		// radiance - disabled for threading // static StatsCounter maxDepth("Kd-Tree Accelerator","Maximum kd-tree depth");
		// radiance - disabled for threading // static StatsCounter maxLeafPrims("Kd-Tree Accelerator","Maximum number of primitives in leaf node");
		// radiance - disabled for threading // ++numLeafMade;
		//maxDepth.Max(depth);
		// radiance - disabled for threading // maxLeafPrims.Max(np);
		// radiance - disabled for threading // static StatsRatio leafPrims("Kd-Tree Accelerator","Avg. number of primitives in leaf nodes");
		// radiance - disabled for threading // leafPrims.Add(np, 1);
		nPrims = np << 2;
		flags |= 3;
		// Store _Primitive *_s for leaf node
		if (np == 0)
			onePrimitive = NULL;
		else if (np == 1)
			onePrimitive = prims[primNums[0]];
		else {
			primitives = (Primitive **)arena.Alloc(np *
				sizeof(Primitive **));
			for (int i = 0; i < np; ++i)
				primitives[i] = prims[primNums[i]];
		}
	}
	void initInterior(int axis, float s) {
		// radiance - disabled for threading // static StatsCounter nodesMade("Kd-Tree Accelerator", "Interior kd-tree nodes made"); // NOBOOK
		// radiance - disabled for threading // ++nodesMade; // NOBOOK
		split = s;
		flags &= ~3;
		flags |= axis;
	}
	float SplitPos() const { return split; }
	int nPrimitives() const { return nPrims >> 2; }
	int SplitAxis() const { return flags & 3; }
	bool IsLeaf() const { return (flags & 3) == 3; }

	// NOTE - radiance - applied bugfix for light leaks on planes (found by ratow)
	// moved flags outside of union
	u_int flags;   // Both
	union {
		//u_int flags;   // Both
		float split;   // Interior
		u_int nPrims;  // Leaf
	};
	union {
		u_int aboveChild;           // Interior
		Primitive *onePrimitive;  // Leaf
		Primitive **primitives;   // Leaf
	};
};

// Dade - inverse mailbox support. I use a ring buffer in order to
// store already intersected primitives.
// This implementation could be improved by using only MAILBOX_SIZE
// power of 2 (in that case X  & MAILBOX_SIZE_MASK = X % MAILBOX_SIZE)
#define MAILBOX_SIZE 8
struct Mailbox {
	int indexFirstUsed, indexFirstFree;
	Primitive *mailboxes[MAILBOX_SIZE];

	Mailbox() {
		indexFirstUsed = -1;
		indexFirstFree = -1;
	}

	void addChecked(Primitive *p) {
		// Dade - check if the mailboxes are empty
		if(indexFirstFree == -1) {
			mailboxes[0] = p;
			indexFirstUsed = 0;
			indexFirstFree = 1;
		} else {
			mailboxes[indexFirstFree] = p;

			if(indexFirstFree == indexFirstUsed) {
				// Dade - all mailboxes are full

				indexFirstFree++;
				if(indexFirstFree >= MAILBOX_SIZE)
					indexFirstFree = 0;

				indexFirstUsed = indexFirstFree;
			} else {
				indexFirstFree++;

				if(indexFirstFree >= MAILBOX_SIZE)
					indexFirstFree = 0;
			}
		}
	}

	bool alreadyChecked(const Primitive *p) const {
		// Dade - check if the mailboxes are empty
		if(indexFirstUsed == -1)
			return false;

		// Dade - check if the mailboxes are full
		if(indexFirstFree == indexFirstUsed) {
			for(int i = 0; i < MAILBOX_SIZE; i++)
				if(mailboxes[i] == p)
					return true;

			return false;
		}

		// Dade - the mailboxes can be half full only at beginning.
		// It means indexFirstUsed is 0.
		BOOST_ASSERT(indexFirstUsed == 0);

		for(int i = 0; i < indexFirstFree; i++)
			if(mailboxes[i] == p)
				return true;

		return false;
	}
};

// SafeKdTreeAccel Declarations
class  SafeKdTreeAccel : public Aggregate {
public:
	// SafeKdTreeAccel Public Methods
	SafeKdTreeAccel(const vector<Primitive* > &p,
		int icost, int scost,
		float ebonus, int maxp, int maxDepth);
	BBox WorldBound() const { return bounds; }
	bool CanIntersect() const { return true; }
	~SafeKdTreeAccel();
	void buildTree(int nodeNum, const BBox &bounds,
	    const vector<BBox> &primBounds,
		int *primNums, int nprims, int depth,
		BoundEdge *edges[3],
		int *prims0, int *prims1, int badRefines = 0);
	bool Intersect(const Ray &ray, Intersection *isect) const;
	bool IntersectP(const Ray &ray) const;
	
	static Primitive *CreateAccelerator(const vector<Primitive* > &prims, const ParamSet &ps);

private:
	// SafeKdTreeAccel Private Data
	BBox bounds;
	int isectCost, traversalCost, maxPrims;
	float emptyBonus;

	u_int nPrims;
	Primitive **prims;
	SafeKdAccelNode *nodes;
	int nAllocedNodes, nextFreeNode;
	
	MemoryArena arena;
};

struct SafeKdToDo {
	const SafeKdAccelNode *node;
	float tmin, tmax;
};

}//namespace lux

#endif

