/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_KDTREEACCEL_H
#define LUX_KDTREEACCEL_H

// kdtree.cpp*
#include "lux.h"
#include "primitive.h"

// KdAccelNode Declarations

struct MailboxPrim {
	MailboxPrim(const Primitive* &p) {
		primitive = (Primitive*) p;
		lastMailboxId = -1;
	}
	Primitive* primitive;
	int lastMailboxId;
};
struct KdAccelNode {
	// KdAccelNode Methods
	void initLeaf(int *primNums, int np,
			MailboxPrim *mailboxPrims, MemoryArena &arena) {
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
		// Store _MailboxPrim *_s for leaf node
		if (np == 0)
			onePrimitive = NULL;
		else if (np == 1)
			onePrimitive = &mailboxPrims[primNums[0]];
		else {
			primitives = (MailboxPrim **)arena.Alloc(np *
				sizeof(MailboxPrim *));
			for (int i = 0; i < np; ++i)
				primitives[i] = &mailboxPrims[primNums[i]];
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
	union {
		u_int flags;   // Both
		float split;   // Interior
		u_int nPrims;  // Leaf
	};
	union {
		u_int aboveChild;           // Interior
		MailboxPrim *onePrimitive;  // Leaf
		MailboxPrim **primitives;   // Leaf
	};
};
struct BoundEdge {
	// BoundEdge Public Methods
	BoundEdge() { }
	BoundEdge(float tt, int pn, bool starting) {
		t = tt;
		primNum = pn;
		type = starting ? START : END;
	}
	bool operator<(const BoundEdge &e) const {
		if (t == e.t)
			return (int)type < (int)e.type;
		else return t < e.t;
	}
	float t;
	int primNum;
	enum { START, END } type;
};
// KdTreeAccel Declarations
struct KdAccelNode;
class  KdTreeAccel : public Aggregate {
public:
	// KdTreeAccel Public Methods
	KdTreeAccel(const vector<Primitive* > &p,
		int icost, int scost,
		float ebonus, int maxp, int maxDepth);
	BBox WorldBound() const { return bounds; }
	bool CanIntersect() const { return true; }
	~KdTreeAccel();
	void buildTree(int nodeNum, const BBox &bounds,
	    const vector<BBox> &primBounds,
		int *primNums, int nprims, int depth,
		BoundEdge *edges[3],
		int *prims0, int *prims1, int badRefines = 0);
	bool Intersect(const Ray &ray, Intersection *isect) const;
	bool IntersectP(const Ray &ray) const;
	
	static Primitive *CreateAccelerator(const vector<Primitive* > &prims, const ParamSet &ps);
	

	
private:
	// KdTreeAccel Private Data
	BBox bounds;
	int isectCost, traversalCost, maxPrims;
	float emptyBonus;
	u_int nMailboxes;
	MailboxPrim *mailboxPrims;
	mutable int curMailboxId;
	KdAccelNode *nodes;
	int nAllocedNodes, nextFreeNode;
	
	MemoryArena arena;
};
struct KdToDo {
	const KdAccelNode *node;
	float tmin, tmax;
};

#endif

