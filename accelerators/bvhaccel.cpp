
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

using namespace lux;

// BVHAccel Method Definitions
BVHAccel::BVHAccel(const vector<boost::shared_ptr<Primitive> > &p) {
	vector<boost::shared_ptr<Primitive> > vPrims;
	const PrimitiveRefinementHints refineHints(false);
	for (u_int i = 0; i < p.size(); ++i) {
		if(p[i]->CanIntersect())
			vPrims.push_back(p[i]);
		else
			p[i]->Refine(vPrims, refineHints, p[i]);
	}

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
	rootNode = BuildHierarchy(bvList, 0, bvList.size()-1, 0);

	bvhTree = (BVHAccelArrayNode*)AllocAligned(nNodes * sizeof(BVHAccelArrayNode));
	BuildArray(rootNode, 0);
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

boost::shared_ptr<BVHAccelTreeNode> BVHAccel::BuildHierarchy(vector<boost::shared_ptr<BVHAccelTreeNode> > &list, u_int begin, u_int end, u_int axis) {
	nNodes += 1;
	if(begin == end) // Only a single item in list so return it
		return list[begin];

	boost::shared_ptr<BVHAccelTreeNode> parent(new BVHAccelTreeNode());
	parent->primitive = NULL;

	sort(list.begin()+begin, list.begin()+end, bvh_lt[axis]); // sort on axis
	axis = (axis+1)%3; // next axis

	boost::shared_ptr<BVHAccelTreeNode> child;
	//Left Child
	child = BuildHierarchy(list, begin, (begin+end)/2, axis);
	parent->leftChild = child;
	parent->bbox = Union(parent->bbox, child->bbox);

	//Right Child
	child = BuildHierarchy(list, 1+(begin+end)/2, end, axis);
	parent->leftChild->rightSibling = child;
	parent->bbox = Union(parent->bbox, child->bbox);

	//std::cout << parent->leftChild->bbox << " - " << child->bbox << std::endl;

	return parent;
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
	return new BVHAccel(prims);
}

static DynamicLoader::RegisterAccelerator<BVHAccel> r("bvh");
