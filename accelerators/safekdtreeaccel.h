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
	u_int nMailboxes;
	MailboxPrim *mailboxPrims;
	mutable int curMailboxId;
	KdAccelNode *nodes;
	int nAllocedNodes, nextFreeNode;
	
	MemoryArena arena;
};

}//namespace lux

#endif

