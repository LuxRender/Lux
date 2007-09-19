
/*
 * pbrt source code Copyright(c) 1998-2007 Matt Pharr and Greg Humphreys
 *
 * All Rights Reserved.
 * For educational use only; commercial use expressly forbidden.
 * NO WARRANTY, express or implied, for this software.
 * (See file License.txt for complete license)
 */

#ifndef PBRT_OCTREE_H
#define PBRT_OCTREE_H
// octree.h*
#include "pbrt.h"
#include "geometry.h"
// Octree Declarations
template <class NodeData> struct OctNode {
	OctNode() {
		for (int i = 0; i < 8; ++i)
			children[i] = NULL;
	}
	~OctNode() {
		for (int i = 0; i < 8; ++i)
			delete children[i];
	}
	OctNode *children[8];
	vector<NodeData> data;
};
template <class NodeData, class LookupProc> class Octree {
public:
	// Octree Public Methods
	Octree(const BBox &b, int md = 16)
		: bound(b) {
		maxDepth = md;
	}
	void Add(const NodeData &dataItem, const BBox &dataBound) {
		addPrivate(&root, bound, dataItem, dataBound,
			DistanceSquared(dataBound.pMin, dataBound.pMax));
	}
	void Lookup(const Point &p, const LookupProc &process) {
		if (!bound.Inside(p)) return;
		lookupPrivate(&root, bound, p, process);
	}
private:
	// Octree Private Methods
	void addPrivate(OctNode<NodeData> *node, const BBox &nodeBound,
		const NodeData &dataItem, const BBox &dataBound, float diag2,
		int depth = 0);
	void lookupPrivate(OctNode<NodeData> *node, const BBox &nodeBound, const Point &P,
			const LookupProc &process);
	// Octree Private Data
	int maxDepth;
	BBox bound;
	OctNode<NodeData> root;
};
// Octree Method Definitions
template <class NodeData, class LookupProc>
void Octree<NodeData, LookupProc>::addPrivate(
		OctNode<NodeData> *node, const BBox &nodeBound,
		const NodeData &dataItem, const BBox &dataBound,
		float diag2, int depth) {
	// Possibly add data item to current octree node
	if (depth == maxDepth ||
		DistanceSquared(nodeBound.pMin,
		                nodeBound.pMax) < diag2) {
		node->data.push_back(dataItem);
		return;
	}
	// Otherwise add data item to octree children
	Point pMid = .5 * nodeBound.pMin + .5 * nodeBound.pMax;
	// Determine which children the item overlaps
	bool over[8];
	over[0] = over[1] =
			  over[2] =
			  over[3] = (dataBound.pMin.x <= pMid.x);
	over[4] = over[5] =
			  over[6] =
			  over[7] = (dataBound.pMax.x  > pMid.x);
	over[0] &= (dataBound.pMin.y <= pMid.y);
	over[1] &= (dataBound.pMin.y <= pMid.y);
	over[4] &= (dataBound.pMin.y <= pMid.y);
	over[5] &= (dataBound.pMin.y <= pMid.y);
	over[2] &= (dataBound.pMax.y  > pMid.y);
	over[3] &= (dataBound.pMax.y  > pMid.y);
	over[6] &= (dataBound.pMax.y  > pMid.y);
	over[7] &= (dataBound.pMax.y  > pMid.y);
	over[0] &= (dataBound.pMin.z <= pMid.z);
	over[2] &= (dataBound.pMin.z <= pMid.z);
	over[4] &= (dataBound.pMin.z <= pMid.z);
	over[6] &= (dataBound.pMin.z <= pMid.z);
	over[1] &= (dataBound.pMax.z  > pMid.z);
	over[3] &= (dataBound.pMax.z  > pMid.z);
	over[5] &= (dataBound.pMax.z  > pMid.z);
	over[7] &= (dataBound.pMax.z  > pMid.z);
	for (int child = 0; child < 8; ++child) {
		if (!over[child]) continue;
		if (!node->children[child])
			node->children[child] = new OctNode<NodeData>;
		// Compute _childBound_ for octree child _child_
		BBox childBound;
		childBound.pMin.x = (child & 4) ? pMid.x : nodeBound.pMin.x;
		childBound.pMax.x = (child & 4) ? nodeBound.pMax.x : pMid.x;
		childBound.pMin.y = (child & 2) ? pMid.y : nodeBound.pMin.y;
		childBound.pMax.y = (child & 2) ? nodeBound.pMax.y : pMid.y;
		childBound.pMin.z = (child & 1) ? pMid.z : nodeBound.pMin.z;
		childBound.pMax.z = (child & 1) ? nodeBound.pMax.z : pMid.z;
		addPrivate(node->children[child], childBound,
		           dataItem, dataBound, diag2, depth+1);
	}
}
template <class NodeData, class LookupProc>
void Octree<NodeData, LookupProc>::lookupPrivate(
		OctNode<NodeData> *node, const BBox &nodeBound,
		const Point &p, const LookupProc &process) {
	for (u_int i = 0; i < node->data.size(); ++i)
		process(p, node->data[i]);
	// Determine which octree child node _p_ is inside
	Point pMid = .5f * nodeBound.pMin + .5f * nodeBound.pMax;
	int child = (p.x > pMid.x ? 4 : 0) +
		(p.y > pMid.y ? 2 : 0) + (p.z > pMid.z ? 1 : 0);
	if (node->children[child]) {
		// Compute _childBound_ for octree child _child_
		BBox childBound;
		childBound.pMin.x = (child & 4) ? pMid.x : nodeBound.pMin.x;
		childBound.pMax.x = (child & 4) ? nodeBound.pMax.x : pMid.x;
		childBound.pMin.y = (child & 2) ? pMid.y : nodeBound.pMin.y;
		childBound.pMax.y = (child & 2) ? nodeBound.pMax.y : pMid.y;
		childBound.pMin.z = (child & 1) ? pMid.z : nodeBound.pMin.z;
		childBound.pMax.z = (child & 1) ? nodeBound.pMax.z : pMid.z;
		lookupPrivate(node->children[child], childBound, p,
			process);
	}
}
#endif // PBRT_OCTREE_H
