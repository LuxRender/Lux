
/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

// Boundary Volume Hierarchy accelerator
// Based of "Efficiency Issues for Ray Tracing" by Brian Smits
// Available at http://www.cs.utah.edu/~bes/papers/fastRT/paper.html

// bvhaccel.cpp*

#include "bvhaccel.h"
#include "paramset.h"
#include "dynload.h"

#include "error.h"

using namespace lux;

// BVHAccel Method Definitions
BVHAccel::BVHAccel(const vector<boost::shared_ptr<Primitive> > &p, int treetype, int csamples, int icost, int tcost, float ebonus) :
			costSamples(csamples), isectCost(icost), traversalCost(tcost), emptyBonus(ebonus) {
	vector<boost::shared_ptr<Primitive> > vPrims;
	const PrimitiveRefinementHints refineHints(false);
	for (u_int i = 0; i < p.size(); ++i) {
		if(p[i]->CanIntersect())
			vPrims.push_back(p[i]);
		else
			p[i]->Refine(vPrims, refineHints, p[i]);
	}

	// Make sure treeType is 2, 4 or 8
	if(treetype <= 2) treeType = 2;
	else if(treetype <=4) treeType = 4;
	else treeType = 8;

	// Initialize primitives for _BVHAccel_
	nPrims = vPrims.size();
	prims = (boost::shared_ptr<Primitive>*)AllocAligned(nPrims *
			sizeof(boost::shared_ptr<Primitive>));
	for (u_int i = 0; i < nPrims; ++i)
		new (&prims[i]) boost::shared_ptr<Primitive>(vPrims[i]);

	vector<boost::shared_ptr<BVHAccelTreeNode> > bvList;
	for (u_int i = 0; i < nPrims; ++i) {
		boost::shared_ptr<BVHAccelTreeNode> ptr(new BVHAccelTreeNode());
		ptr->bbox = prims[i]->WorldBound();
		ptr->primitive = prims[i].get();
		bvList.push_back(ptr);
	}

	std::stringstream ss;
	ss << "Building Bounding Volume Hierarchy, primitives: " << nPrims;
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	boost::shared_ptr<BVHAccelTreeNode> rootNode;
	nNodes = 0;
	rootNode = BuildHierarchy(bvList, 0, bvList.size(), 2);

	ss.str("");
	ss << "Pre-processing Bounding Volume Hierarchy, total nodes: " << nNodes;
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	bvhTree = (BVHAccelArrayNode*)AllocAligned(nNodes * sizeof(BVHAccelArrayNode));
	BuildArray(rootNode, 0);

	ss.str("");
	ss << "Finished building Bounding Volume Hierarchy array";
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
}

BVHAccel::~BVHAccel() {
	for(u_int i=0; i<nPrims; i++)
		prims[i].~shared_ptr();
    FreeAligned(prims);
    FreeAligned(bvhTree);
}

// Build an array of comparators for each axis
bool bvh_lt_x(boost::shared_ptr<BVHAccelTreeNode> n1, boost::shared_ptr<BVHAccelTreeNode> n2) { return n1->bbox.pMax.x+n1->bbox.pMin.x < n2->bbox.pMax.x+n2->bbox.pMin.x; }
bool bvh_lt_y(boost::shared_ptr<BVHAccelTreeNode> n1, boost::shared_ptr<BVHAccelTreeNode> n2) { return n1->bbox.pMax.y+n1->bbox.pMin.y < n2->bbox.pMax.y+n2->bbox.pMin.y; }
bool bvh_lt_z(boost::shared_ptr<BVHAccelTreeNode> n1, boost::shared_ptr<BVHAccelTreeNode> n2) { return n1->bbox.pMax.z+n1->bbox.pMin.z < n2->bbox.pMax.z+n2->bbox.pMin.z; }
bool (* const bvh_lt[3])(boost::shared_ptr<BVHAccelTreeNode> n1, boost::shared_ptr<BVHAccelTreeNode> n2) = {bvh_lt_x, bvh_lt_y, bvh_lt_z};
bool bvh_gt_x(boost::shared_ptr<BVHAccelTreeNode> n1, boost::shared_ptr<BVHAccelTreeNode> n2) { return n1->bbox.pMax.x+n1->bbox.pMin.x > n2->bbox.pMax.x+n2->bbox.pMin.x; }
bool bvh_gt_y(boost::shared_ptr<BVHAccelTreeNode> n1, boost::shared_ptr<BVHAccelTreeNode> n2) { return n1->bbox.pMax.y+n1->bbox.pMin.y > n2->bbox.pMax.y+n2->bbox.pMin.y; }
bool bvh_gt_z(boost::shared_ptr<BVHAccelTreeNode> n1, boost::shared_ptr<BVHAccelTreeNode> n2) { return n1->bbox.pMax.z+n1->bbox.pMin.z > n2->bbox.pMax.z+n2->bbox.pMin.z; }
bool (* const bvh_gt[3])(boost::shared_ptr<BVHAccelTreeNode> n1, boost::shared_ptr<BVHAccelTreeNode> n2) = {bvh_gt_x, bvh_gt_y, bvh_gt_z};

boost::shared_ptr<BVHAccelTreeNode> BVHAccel::BuildHierarchy(vector<boost::shared_ptr<BVHAccelTreeNode> > &list, u_int begin, u_int end, u_int axis) {
	u_int splitIndex, splitAxis = axis;

	nNodes += 1;
	if(end-begin == 1) // Only a single item in list so return it
		return list[begin];

	boost::shared_ptr<BVHAccelTreeNode> parent(new BVHAccelTreeNode());
	parent->primitive = NULL;

	vector<u_int> splits; splits.reserve(treeType+1);
	splits.push_back(begin); splits.push_back(end);
	for(u_int i = 1, begin = 0; i < treeType; i++) {  //Calculate splits, according to tree type and partially sort

		/* These are the splits and inserts done, depending on the treeType:
		// Binary-tree
		FindBestSplit(list, splits[0], splits[1], &splitIndex, &splitAxis);
		splits.insert(splits.begin()+1, splitIndex);
		// Quad-tree
		FindBestSplit(list, splits[0], splits[1], &splitAxis);
		splits.insert(splits.begin()+1, splitIndex);
		FindBestSplit(list, splits[2], splits[3] &splitAxis);
		splits.insert(splits.begin()+3, splitIndex);
		// Oc-tree
		FindBestSplit(list, splits[0], splits[1], &splitAxis);
		splits.insert(splits.begin()+1, splitIndex);
		FindBestSplit(list, splits[2], splits[3], &splitAxis);
		splits.insert(splits.begin()+3, splitIndex);
		FindBestSplit(list, splits[4], splits[5], &splitAxis);
		splits.insert(splits.begin()+5, splitIndex);
		FindBestSplit(list, splits[6], splits[7], &splitAxis);
		splits.insert(splits.begin()+7, splitIndex);
		*/

		if(splits[begin+1] - splits[begin] < 2) {
			begin = (begin+1)%(i+1);
			continue; // Less than two elements: no need to split
		}

		FindBestSplit(list, splits[begin], splits[begin+1], &splitIndex, &splitAxis);
		if(splits[begin+1]-splitIndex <= splitIndex-splits[begin]) {
			splits.insert(splits.begin()+begin+1, splitIndex);
			partial_sort(list.begin()+splits[begin], list.begin()+splits[begin+1], list.begin()+splits[begin+2], bvh_lt[splitAxis]); // sort on axis
		} else {
			// Rearrange so that partial_sort does less work
			splitIndex = splits[begin] + splits[begin+1] - splitIndex;
			splits.insert(splits.begin()+begin+1, splitIndex);
			partial_sort(list.begin()+splits[begin], list.begin()+splits[begin+1], list.begin()+splits[begin+2], bvh_gt[splitAxis]); // sort on axis
		}

		begin = (begin+2)%(i+1); // Generates the sequence (0  0 2  0 2 4 6)
	}

	boost::shared_ptr<BVHAccelTreeNode> child, lastChild;
	//Left Child
	child = BuildHierarchy(list, splits[0], splits[1], splitAxis);
	parent->leftChild = child;
	parent->bbox = Union(parent->bbox, child->bbox);
	lastChild = child;

	// Add remaining children
	for(u_int i = 1; i < splits.size()-1; i++) {
		child = BuildHierarchy(list, splits[i], splits[i+1], splitAxis);
		lastChild->rightSibling = child;
		parent->bbox = Union(parent->bbox, child->bbox);
		lastChild = child;
	}

	return parent;
}

void BVHAccel::FindBestSplit(vector<boost::shared_ptr<BVHAccelTreeNode> > &list, u_int begin, u_int end, u_int *bestIndex, u_int *bestAxis) {
	float bestCost = INFINITY;

	if(end-begin == 2) {
		// Trivial case with two elements
		*bestIndex = begin+1;
		*bestAxis = 0;
	} else {
		BBox nodeBounds;
		for(u_int i = begin; i < end; i++)
			nodeBounds = Union(nodeBounds, list[i]->bbox);
		*bestAxis = nodeBounds.MaximumExtent();

		if(costSamples > 1) {
			Vector d = nodeBounds.pMax - nodeBounds.pMin;
			float totalSA = (2.f * (d.x*d.y + d.x*d.z + d.y*d.z));
			float invTotalSA = 1.f / totalSA;

			// Sample cost for split at some points
			float increment = d[*bestAxis]/(costSamples+1);
			for(float splitVal = nodeBounds.pMin[*bestAxis]+increment; splitVal < nodeBounds.pMax[*bestAxis]; splitVal += increment) {
				int nBelow = 0, nAbove = 0;
				BBox bbBelow, bbAbove;
				for(u_int j = begin; j < end; j++) {
					if((list[j]->bbox.pMax[*bestAxis]+list[j]->bbox.pMin[*bestAxis]) < 2*splitVal) {
						nBelow++;
						bbBelow = Union(bbBelow, list[j]->bbox);
					} else {
						nAbove++;
						bbAbove = Union(bbAbove, list[j]->bbox);
					}
				}
				Vector dBelow = bbBelow.pMax - bbBelow.pMax;
				Vector dAbove = bbAbove.pMax - bbAbove.pMin;
				float belowSA = 2 * ((dBelow.x*dBelow.y + dBelow.x*dBelow.z + dBelow.y*dBelow.z));
				float aboveSA = 2 * ((dAbove.x*dAbove.y + dAbove.x*dAbove.z + dAbove.y*dAbove.z));
				float pBelow = belowSA * invTotalSA;
				float pAbove = aboveSA * invTotalSA;
				float eb = (nAbove == 0 || nBelow == 0) ? emptyBonus : 0.f;
				float cost = traversalCost + isectCost * (1.f - eb) * (pBelow * nBelow + pAbove * nAbove);
				// Update best split if this is lowest cost so far
				if (cost < bestCost)  {
					bestCost = cost;
					*bestIndex = begin+nBelow;
				}
			}
		} else {
			// Split in half
			*bestIndex = (begin+end)/2;
		}

		// Make sure the split is valid
		*bestIndex = max(*bestIndex,begin+1);
		*bestIndex = min(*bestIndex,end-1);
	}
}

u_int BVHAccel::BuildArray(boost::shared_ptr<BVHAccelTreeNode> node, u_int offset) {
	// Build array by recursively traversing the tree depth-first
	while(node) {
		BVHAccelArrayNode* p = &bvhTree[offset];

		p->bbox = node->bbox;
		p->primitive = node->primitive;
		offset = BuildArray(node->leftChild, offset+1);
		p->skipIndex = offset;

		node = node->rightSibling;
	}
	return offset;
}

BBox BVHAccel::WorldBound() const {
	return bvhTree[0].bbox;
}

bool BVHAccel::Intersect(const Ray &ray,
                          Intersection *isect) const {
	u_int currentNode = 0; // Root Node
	u_int stopNode = bvhTree[0].skipIndex; // Non-existent
	bool hit = false;

	while(currentNode < stopNode) {
		if(bvhTree[currentNode].bbox.IntersectP(ray)) {
			if(bvhTree[currentNode].primitive != NULL)
				if(bvhTree[currentNode].primitive->Intersect(ray, isect))
					hit = true; // Continue testing for closer intersections
			currentNode++;
		} else {
			currentNode = bvhTree[currentNode].skipIndex;
		}
	}

	return hit;
}

bool BVHAccel::IntersectP(const Ray &ray) const {
	u_int currentNode = 0; // Root Node
	u_int stopNode = bvhTree[0].skipIndex; // Non-existent

	while(currentNode < stopNode) {
		if(bvhTree[currentNode].bbox.IntersectP(ray)) {
			if(bvhTree[currentNode].primitive != NULL)
				if(bvhTree[currentNode].primitive->IntersectP(ray))
					return true;
			currentNode++;
		} else {
			currentNode = bvhTree[currentNode].skipIndex;
		}
	}

	return false;
}

void BVHAccel::GetPrimitives(vector<boost::shared_ptr<Primitive> > &primitives) {
	primitives.reserve(nPrims);
	for(u_int i=0; i<nPrims; i++) {
		primitives.push_back(prims[i]);
	}
}

Aggregate* BVHAccel::CreateAccelerator(const vector<boost::shared_ptr<Primitive> > &prims,
		const ParamSet &ps) {
	int treeType = ps.FindOneInt("treetype", 4); // Tree type to generate (2 = binary, 4 = quad, 8 = octree)
	int costSamples = ps.FindOneInt("costsamples", 0); // Samples to get for cost minimization
	int isectCost = ps.FindOneInt("intersectcost", 80);
	int travCost = ps.FindOneInt("traversalcost", 10);
	float emptyBonus = ps.FindOneFloat("emptybonus", 0.5f);
	return new BVHAccel(prims, treeType, costSamples, isectCost, travCost, emptyBonus);

}

static DynamicLoader::RegisterAccelerator<BVHAccel> r("bvh");
