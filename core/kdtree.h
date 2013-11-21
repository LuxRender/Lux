/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

#ifndef LUX_KDTREE_H
#define LUX_KDTREE_H
// kdtree.h*
#include "lux.h"
#include "luxrays/core/geometry/bbox.h"
using luxrays::BBox;
// KdTree Declarations

namespace lux
{

struct KdNode {
	void init(float p, u_int a) {
		splitPos = p;
		splitAxis = a;
		// Dade - in order to avoid a gcc warning
		rightChild = 0;
		rightChild = ~rightChild;
		hasLeftChild = 0;
	}
	void initLeaf() {
		init(0.0f, 3);
	}
	// KdNode Data
	float splitPos;
	u_int splitAxis:2;
	u_int hasLeftChild:1;
	u_int rightChild:29;
};
template <class NodeData, class LookupProc> class KdTree {
public:
	// KdTree Public Methods
	KdTree(const vector<NodeData> &data);
	~KdTree() {
		luxrays::FreeAligned(nodes);
		delete[] nodeData;
	}
	void recursiveBuild(u_int nodeNum, u_int start, u_int end,
		vector<const NodeData *> &buildNodes);
	void Lookup(const Point &p, const LookupProc &process,
			float &maxDistSquared) const;
	NodeData *getNodeData() { return nodeData; }

private:
	// KdTree Private Methods
	void privateLookup(u_int nodeNum, const Point &p,
		const LookupProc &process, float &maxDistSquared) const;
	// KdTree Private Data
	KdNode *nodes;
	NodeData *nodeData;
	u_int nNodes, nextFreeNode;
};
template<class NodeData> struct CompareNode {
	CompareNode(int a) { axis = a; }
	int axis;
	bool operator()(const NodeData *d1,
			const NodeData *d2) const {
		return d1->p[axis] == d2->p[axis] ? (d1 < d2) :
			d1->p[axis] < d2->p[axis];
	}
};
// KdTree Method Definitions
template <class NodeData, class LookupProc>
KdTree<NodeData,
       LookupProc>::KdTree(const vector<NodeData> &d) {
	nNodes = d.size();
	nextFreeNode = 1;
	nodes = luxrays::AllocAligned<KdNode>(nNodes);
	nodeData = new NodeData[nNodes];
	vector<const NodeData *> buildNodes;
	for (u_int i = 0; i < nNodes; ++i)
		buildNodes.push_back(&d[i]);
	// Begin the KdTree building process
	recursiveBuild(0, 0, nNodes, buildNodes);
}
template <class NodeData, class LookupProc> void
KdTree<NodeData, LookupProc>::recursiveBuild(u_int nodeNum,
		u_int start, u_int end,
		vector<const NodeData *> &buildNodes) {
	// Create leaf node of kd-tree if we've reached the bottom
	if (start + 1 == end) {
		nodes[nodeNum].initLeaf();
		nodeData[nodeNum] = *buildNodes[start];
		return;
	}
	// Choose split direction and partition data
	// Compute bounds of data from _start_ to _end_
	BBox bound;
	for (u_int i = start; i < end; ++i)
		bound = Union(bound, buildNodes[i]->p);
	u_int splitAxis = bound.MaximumExtent();
	u_int splitPos = (start + end) / 2;
	// buildNodes[buildNodes.size()] may not exist
	std::nth_element(buildNodes.begin()+start, buildNodes.begin()+splitPos,
		buildNodes.begin()+end, CompareNode<NodeData>(splitAxis));

	// Allocate kd-tree node and continue recursively
	nodes[nodeNum].init(buildNodes[splitPos]->p[splitAxis],
		splitAxis);
	nodeData[nodeNum] = *buildNodes[splitPos];
	if (start < splitPos) {
		nodes[nodeNum].hasLeftChild = 1;
		u_int childNum = nextFreeNode++;
		recursiveBuild(childNum, start, splitPos, buildNodes);
	}
	if (splitPos+1 < end) {
		nodes[nodeNum].rightChild = nextFreeNode++;
		recursiveBuild(nodes[nodeNum].rightChild, splitPos+1,
		               end, buildNodes);
	}
}
template <class NodeData, class LookupProc> void
KdTree<NodeData, LookupProc>::Lookup(const Point &p,
		const LookupProc &proc,
		float &maxDistSquared) const {
	privateLookup(0, p, proc, maxDistSquared);
}
template <class NodeData, class LookupProc> void
KdTree<NodeData, LookupProc>::privateLookup(u_int nodeNum,
		const Point &p,	const LookupProc &process,
		float &maxDistSquared) const {
	KdNode *node = &nodes[nodeNum];
	// Process kd-tree node's children
	int axis = node->splitAxis;
	if (axis != 3) {
		float dist = p[axis] - node->splitPos;
		float dist2 = dist * dist;
		if (p[axis] <= node->splitPos) {
			if (node->hasLeftChild)
				privateLookup(nodeNum+1, p,
				              process, maxDistSquared);
			if (dist2 < maxDistSquared &&
			    node->rightChild < nNodes)
				privateLookup(node->rightChild,
				              p,
							  process,
							  maxDistSquared);
		}
		else {
			if (node->rightChild < nNodes)
				privateLookup(node->rightChild,
				              p,
							  process,
							  maxDistSquared);
			if (dist2 < maxDistSquared && node->hasLeftChild)
				privateLookup(nodeNum+1,
				              p,
							  process,
							  maxDistSquared);
		}
	}
	// Hand kd-tree node to processing function
	float dist2 = DistanceSquared(nodeData[nodeNum].p, p);
	if (dist2 < maxDistSquared)
		process(nodeData[nodeNum], dist2, maxDistSquared);
}

}//namespace lux

#endif // LUX_KDTREE_H
