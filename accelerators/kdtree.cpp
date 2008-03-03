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

// kdtree.cpp*
#include "kdtreeaccel.h"
#include "paramset.h"

using namespace lux;

// KdTreeAccel Method Definitions
KdTreeAccel::
    KdTreeAccel(const vector<Primitive* > &p,
		int icost, int tcost,
		float ebonus, int maxp, int maxDepth)
	: isectCost(icost), traversalCost(tcost),
	maxPrims(maxp), emptyBonus(ebonus) {
	vector<Primitive* > prims;
	for (u_int i = 0; i < p.size(); ++i)
		p[i]->FullyRefine(prims);
	// Initialize mailboxes for _KdTreeAccel_
	curMailboxId = 0;
	nMailboxes = prims.size();
	mailboxPrims = (MailboxPrim *)AllocAligned(nMailboxes *
		sizeof(MailboxPrim));
	for (u_int i = 0; i < nMailboxes; ++i)
		new (&mailboxPrims[i]) MailboxPrim((const Primitive*&)prims[i]);
	// Build kd-tree for accelerator
	nextFreeNode = nAllocedNodes = 0;
	if (maxDepth <= 0)
		maxDepth =
		    Round2Int(8 + 1.3f * Log2Int(float(prims.size())));
	// Compute bounds for kd-tree construction
	vector<BBox> primBounds;
	primBounds.reserve(prims.size());
	for (u_int i = 0; i < prims.size(); ++i) {
		BBox b = prims[i]->WorldBound();
		bounds = Union(bounds, b);
		primBounds.push_back(b);
	}
	// Allocate working memory for kd-tree construction
	BoundEdge *edges[3];
	for (int i = 0; i < 3; ++i)
		edges[i] = new BoundEdge[2*prims.size()];
	int *prims0 = new int[prims.size()];
	int *prims1 = new int[(maxDepth+1) * prims.size()];
	// Initialize _primNums_ for kd-tree construction
	int *primNums = new int[prims.size()];
	for (u_int i = 0; i < prims.size(); ++i)
		primNums[i] = i;
	// Start recursive construction of kd-tree
	buildTree(0, bounds, primBounds, primNums,
	          prims.size(), maxDepth, edges,
			  prims0, prims1);
	// Free working memory for kd-tree construction
	delete[] primNums;
	for (int i = 0; i < 3; ++i)
		delete[] edges[i];
	delete[] prims0;
	delete[] prims1;
}
KdTreeAccel::~KdTreeAccel() {
	for (u_int i = 0; i < nMailboxes; ++i)
		mailboxPrims[i].~MailboxPrim();
	FreeAligned(mailboxPrims);
	FreeAligned(nodes);
}
void KdTreeAccel::buildTree(int nodeNum,
        const BBox &nodeBounds,
		const vector<BBox> &allPrimBounds, int *primNums,
		int nPrims, int depth, BoundEdge *edges[3],
		int *prims0, int *prims1, int badRefines) {
	BOOST_ASSERT(nodeNum == nextFreeNode); // NOBOOK
	// Get next free node from _nodes_ array
	if (nextFreeNode == nAllocedNodes) {
		int nAlloc = max(2 * nAllocedNodes, 512);
		KdAccelNode *n = (KdAccelNode *)AllocAligned(nAlloc *
			sizeof(KdAccelNode));
		if (nAllocedNodes > 0) {
			memcpy(n, nodes,
			       nAllocedNodes * sizeof(KdAccelNode));
			FreeAligned(nodes);
		}
		nodes = n;
		nAllocedNodes = nAlloc;
	}
	++nextFreeNode;
	// Initialize leaf node if termination criteria met
	if (nPrims <= maxPrims || depth == 0) {
		nodes[nodeNum].initLeaf(primNums, nPrims,
		                       mailboxPrims, arena);
		return;
	}
	// Initialize interior node and continue recursion
	// Choose split axis position for interior node
	int bestAxis = -1, bestOffset = -1;
	float bestCost = INFINITY;
	float oldCost = isectCost * float(nPrims);
	Vector d = nodeBounds.pMax - nodeBounds.pMin;
	float totalSA = (2.f * (d.x*d.y + d.x*d.z + d.y*d.z));
	float invTotalSA = 1.f / totalSA;
	// Choose which axis to split along
	int axis;
	if (d.x > d.y && d.x > d.z) axis = 0;
	else axis = (d.y > d.z) ? 1 : 2;
	int retries = 0;
	retrySplit:
	// Initialize edges for _axis_
	for (int i = 0; i < nPrims; ++i) {
		int pn = primNums[i];
		const BBox &bbox = allPrimBounds[pn];
		edges[axis][2*i] =
		    BoundEdge(bbox.pMin[axis], pn, true);
		edges[axis][2*i+1] =
			BoundEdge(bbox.pMax[axis], pn, false);
	}
	sort(&edges[axis][0], &edges[axis][2*nPrims]);
	// Compute cost of all splits for _axis_ to find best
	int nBelow = 0, nAbove = nPrims;
	for (int i = 0; i < 2*nPrims; ++i) {
		if (edges[axis][i].type == BoundEdge::END) --nAbove;
		float edget = edges[axis][i].t;
		if (edget > nodeBounds.pMin[axis] &&
			edget < nodeBounds.pMax[axis]) {
			// Compute cost for split at _i_th edge
			int otherAxis[3][2] = { {1,2}, {0,2}, {0,1} };
			int otherAxis0 = otherAxis[axis][0];
			int otherAxis1 = otherAxis[axis][1];
			float belowSA = 2 * (d[otherAxis0] * d[otherAxis1] +
			             		(edget - nodeBounds.pMin[axis]) *
				                (d[otherAxis0] + d[otherAxis1]));
			float aboveSA = 2 * (d[otherAxis0] * d[otherAxis1] +
								(nodeBounds.pMax[axis] - edget) *
								(d[otherAxis0] + d[otherAxis1]));
			float pBelow = belowSA * invTotalSA;
			float pAbove = aboveSA * invTotalSA;
			float eb = (nAbove == 0 || nBelow == 0) ? emptyBonus : 0.f;
			float cost = traversalCost + isectCost * (1.f - eb) *
				(pBelow * nBelow + pAbove * nAbove);
			// Update best split if this is lowest cost so far
			if (cost < bestCost)  {
				bestCost = cost;
				bestAxis = axis;
				bestOffset = i;
			}
		}
		if (edges[axis][i].type == BoundEdge::START) ++nBelow;
	}
	BOOST_ASSERT(nBelow == nPrims && nAbove == 0); // NOBOOK
	// Create leaf if no good splits were found
	if (bestAxis == -1 && retries < 2) {
		++retries;
		axis = (axis+1) % 3;
		goto retrySplit;
	}
	if (bestCost > oldCost) ++badRefines;
	if ((bestCost > 4.f * oldCost && nPrims < 16) ||
		bestAxis == -1 || badRefines == 3) {
		nodes[nodeNum].initLeaf(primNums, nPrims,
		                     mailboxPrims, arena);
		return;
	}
	// Classify primitives with respect to split
	int n0 = 0, n1 = 0;
	for (int i = 0; i < bestOffset; ++i)
		if (edges[bestAxis][i].type == BoundEdge::START)
			prims0[n0++] = edges[bestAxis][i].primNum;
	for (int i = bestOffset+1; i < 2*nPrims; ++i)
		if (edges[bestAxis][i].type == BoundEdge::END)
			prims1[n1++] = edges[bestAxis][i].primNum;
	// Recursively initialize children nodes
	float tsplit = edges[bestAxis][bestOffset].t;
	nodes[nodeNum].initInterior(bestAxis, tsplit);
	BBox bounds0 = nodeBounds, bounds1 = nodeBounds;
	bounds0.pMax[bestAxis] = bounds1.pMin[bestAxis] = tsplit;
	buildTree(nodeNum+1, bounds0,
		allPrimBounds, prims0, n0, depth-1, edges,
		prims0, prims1 + nPrims, badRefines);
	nodes[nodeNum].aboveChild = nextFreeNode;
	buildTree(nodes[nodeNum].aboveChild, bounds1, allPrimBounds,
		prims1, n1, depth-1, edges,
		prims0, prims1 + nPrims, badRefines);
}
bool KdTreeAccel::Intersect(const Ray &ray,
		Intersection *isect) const {
	// Compute initial parametric range of ray inside kd-tree extent
	float tmin, tmax;
	if (!bounds.IntersectP(ray, &tmin, &tmax))
		return false;
	// Prepare to traverse kd-tree for ray
	int rayId = curMailboxId++;
	Vector invDir(1.f/ray.d.x, 1.f/ray.d.y, 1.f/ray.d.z);
	#define MAX_TODO 64
	KdToDo todo[MAX_TODO];
	int todoPos = 0;
	// Traverse kd-tree nodes in order for ray
	bool hit = false;
	const KdAccelNode *node = &nodes[0];
	while (node != NULL) {
		// Bail out if we found a hit closer than the current node
		if (ray.maxt < tmin) break;
		// radiance - disabled for threading // static StatsCounter nodesTraversed("Kd-Tree Accelerator", 
		// radiance - disabled for threading // "Number of kd-tree nodes traversed by normal rays"); //NOBOOK 
		// radiance - disabled for threading // ++nodesTraversed; //NOBOOK
		if (!node->IsLeaf()) {
			// Process kd-tree interior node
			// Compute parametric distance along ray to split plane
			int axis = node->SplitAxis();
			float tplane = (node->SplitPos() - ray.o[axis]) *
				invDir[axis];
			// Get node children pointers for ray
			const KdAccelNode *firstChild, *secondChild;
			// NOTE - ratow - added direction test for when ray origin is in split plane (fixes bands/artifacts)
			int belowFirst = (ray.o[axis] < node->SplitPos()) ||
				(ray.o[axis] == node->SplitPos() && ray.d[axis] < 0);
			if (belowFirst) {
				firstChild = node + 1;
				secondChild = &nodes[node->aboveChild];
			}
			else {
				firstChild = &nodes[node->aboveChild];
				secondChild = node + 1;
			}
			// Advance to next child node, possibly enqueue other child
			// NOTE - radiance - applied bugfix for bands/artifacts on planes (found by ratow)
			//if (tplane > tmax || tplane < 0)
			if (tplane > tmax || tplane <= 0)
				node = firstChild;
			else if (tplane < tmin)
				node = secondChild;
			else {
				// Enqueue _secondChild_ in todo list
				todo[todoPos].node = secondChild;
				todo[todoPos].tmin = tplane;
				todo[todoPos].tmax = tmax;
				++todoPos;
				node = firstChild;
				tmax = tplane;
			}
		}
		else {
			// Check for intersections inside leaf node
			u_int nPrimitives = node->nPrimitives();
			if (nPrimitives == 1) {
				MailboxPrim *mp = node->onePrimitive;
				// Check one primitive inside leaf node
				if (mp->lastMailboxId != rayId) {
					mp->lastMailboxId = rayId;
					if (mp->primitive->Intersect(ray, isect))
						hit = true;
				}
			}
			else {
				MailboxPrim **prims = node->primitives;
				for (u_int i = 0; i < nPrimitives; ++i) {
					MailboxPrim *mp = prims[i];
					// Check one primitive inside leaf node
					if (mp->lastMailboxId != rayId) {
						mp->lastMailboxId = rayId;
						if (mp->primitive->Intersect(ray, isect))
							hit = true;
					}
				}
			}
			// Grab next node to process from todo list
			if (todoPos > 0) {
				--todoPos;
				node = todo[todoPos].node;
				tmin = todo[todoPos].tmin;
				tmax = todo[todoPos].tmax;
			}
			else
				break;
		}
	}
	return hit;
}
bool KdTreeAccel::IntersectP(const Ray &ray) const {
	// Compute initial parametric range of ray inside kd-tree extent
	float tmin, tmax;
	if (!bounds.IntersectP(ray, &tmin, &tmax))
		return false;
	// Prepare to traverse kd-tree for ray
	int rayId = curMailboxId++;
	Vector invDir(1.f/ray.d.x, 1.f/ray.d.y, 1.f/ray.d.z);
	#define MAX_TODO 64
	KdToDo todo[MAX_TODO];
	int todoPos = 0;
	const KdAccelNode *node = &nodes[0];
	while (node != NULL) {
		// Update kd-tree shadow ray traversal statistics
		// radiance - disabled for threading // static StatsCounter nodesTraversed("Kd-Tree Accelerator",
		// radiance - disabled for threading // "Number of kd-tree nodes traversed by shadow rays");
		// radiance - disabled for threading // ++nodesTraversed;
		if (node->IsLeaf()) {
			// Check for shadow ray intersections inside leaf node
			u_int nPrimitives = node->nPrimitives();
			if (nPrimitives == 1) {
				MailboxPrim *mp = node->onePrimitive;
				if (mp->lastMailboxId != rayId) {
					mp->lastMailboxId = rayId;
					if (mp->primitive->IntersectP(ray))
						return true;
				}
			}
			else {
				MailboxPrim **prims = node->primitives;
				for (u_int i = 0; i < nPrimitives; ++i) {
					MailboxPrim *mp = prims[i];
					if (mp->lastMailboxId != rayId) {
						mp->lastMailboxId = rayId;
						if (mp->primitive->IntersectP(ray))
							return true;
					}
				}
			}
			// Grab next node to process from todo list
			if (todoPos > 0) {
				--todoPos;
				node = todo[todoPos].node;
				tmin = todo[todoPos].tmin;
				tmax = todo[todoPos].tmax;
			}
			else
				break;
		}
		else {
			// Process kd-tree interior node
			// Compute parametric distance along ray to split plane
			int axis = node->SplitAxis();
			float tplane = (node->SplitPos() - ray.o[axis]) *
				invDir[axis];
			// Get node children pointers for ray
			const KdAccelNode *firstChild, *secondChild;
			// NOTE - ratow - added direction test for when ray origin is in split plane (fixes bands/artifacts)
			int belowFirst = (ray.o[axis] < node->SplitPos()) ||
				(ray.o[axis] == node->SplitPos() && ray.d[axis] < 0);
			if (belowFirst) {
				firstChild = node + 1;
				secondChild = &nodes[node->aboveChild];
			}
			else {
				firstChild = &nodes[node->aboveChild];
				secondChild = node + 1;
			}
			// Advance to next child node, possibly enqueue other child
			// NOTE - radiance - applied bugfix for bands/artifacts on planes (found by ratow)
			//if (tplane > tmax || tplane < 0)
			if (tplane > tmax || tplane <= 0)
				node = firstChild;
			else if (tplane < tmin)
				node = secondChild;
			else {
				// Enqueue _secondChild_ in todo list
				todo[todoPos].node = secondChild;
				todo[todoPos].tmin = tplane;
				todo[todoPos].tmax = tmax;
				++todoPos;
				node = firstChild;
				tmax = tplane;
			}
		}
	}
	return false;
}
Primitive* KdTreeAccel::CreateAccelerator(const vector<Primitive* > &prims,
		const ParamSet &ps) {
	int isectCost = ps.FindOneInt("intersectcost", 80);
	int travCost = ps.FindOneInt("traversalcost", 1);
	float emptyBonus = ps.FindOneFloat("emptybonus", 0.5f);
	int maxPrims = ps.FindOneInt("maxprims", 1);
	int maxDepth = ps.FindOneInt("maxdepth", -1);
	return new KdTreeAccel(prims, isectCost, travCost,
		emptyBonus, maxPrims, maxDepth);
}
