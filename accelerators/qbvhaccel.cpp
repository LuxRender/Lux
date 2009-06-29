/***************************************************************************
 *   Copyright (C) 2007 by Anthony Pajot   
 *   anthony.pajot@etu.enseeiht.fr
 *
 * This file is part of FlexRay
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***************************************************************************/

#include "qbvhaccel.h"
#include "paramset.h"
#include "dynload.h"
#include "error.h"

namespace lux
{

/***************************************************/
const boost::int16_t QBVHAccel::pathTable[] = {
	// Note that the packed indices are stored in reverse
	// order, that is first index is in the first 4 bits.
	// visit = 0000
	0x4444, 	0x4444, 	0x4444, 	0x4444,
	0x4444, 	0x4444, 	0x4444, 	0x4444,
	// visit = 0001
	0x4440, 	0x4440, 	0x4440, 	0x4440,
	0x4440, 	0x4440, 	0x4440, 	0x4440,
	// visit = 0010
	0x4441, 	0x4441, 	0x4441, 	0x4441,
	0x4441, 	0x4441, 	0x4441, 	0x4441,
	// visit = 0011
	0x4410, 	0x4410, 	0x4401, 	0x4401,
	0x4410, 	0x4410, 	0x4401, 	0x4401,
	// visit = 0100
	0x4442, 	0x4442, 	0x4442, 	0x4442,
	0x4442, 	0x4442, 	0x4442, 	0x4442,
	// visit = 0101
	0x4420, 	0x4420, 	0x4420, 	0x4420,
	0x4402, 	0x4402, 	0x4402, 	0x4402,
	// visit = 0110
	0x4421, 	0x4421, 	0x4421, 	0x4421,
	0x4412, 	0x4412, 	0x4412, 	0x4412,
	// visit = 0111
	0x4210, 	0x4210, 	0x4201, 	0x4201,
	0x4102, 	0x4102, 	0x4012, 	0x4012,
	// visit = 1000
	0x4443, 	0x4443, 	0x4443, 	0x4443,
	0x4443, 	0x4443, 	0x4443, 	0x4443,
	// visit = 1001
	0x4430, 	0x4430, 	0x4430, 	0x4430,
	0x4403, 	0x4403, 	0x4403, 	0x4403,
	// visit = 1010
	0x4431, 	0x4431, 	0x4431, 	0x4431,
	0x4413, 	0x4413, 	0x4413, 	0x4413,
	// visit = 1011
	0x4310, 	0x4310, 	0x4301, 	0x4301,
	0x4103, 	0x4103, 	0x4013, 	0x4013,
	// visit = 1100
	0x4432, 	0x4423, 	0x4432, 	0x4423,
	0x4432, 	0x4432, 	0x4423, 	0x4423,
	// visit = 1101
	0x4320, 	0x4230, 	0x4320, 	0x4230,
	0x4032, 	0x4023, 	0x4032, 	0x4023,
	// visit = 1110
	0x4321, 	0x4231, 	0x4321, 	0x4231,
	0x4132, 	0x4123, 	0x4132, 	0x4123,
	// visit = 1111
	0x3210, 	0x2310, 	0x3201, 	0x2301,
	0x1032, 	0x1023, 	0x0132, 	0x0123
};


/***************************************************/
QBVHAccel::QBVHAccel(const vector<boost::shared_ptr<Primitive> > &p, int mp, float fst, int sf) : fullSweepThreshold(fst), skipFactor(sf), maxPrimsPerLeaf(mp)
{
	// Refine all primitives
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

	// Temporary data for building
	u_int *primsIndexes = new u_int[nPrims + 3]; // For the case where
	// the last quad would begin at the last primitive
	// (or the second or third last primitive)

	// The number of nodes depends on the number of primitives,
	// and is bounded by 2 * nPrims - 1.
	// Even if there will normally have at least 4 primitives per leaf,
	// it is not always the case => continue to use the normal bounds.
	nNodes = 0;
	maxNodes = 1;
	for (u_int layer = ((nPrims + maxPrimsPerLeaf - 1) / maxPrimsPerLeaf + 3) / 4; layer != 1; layer = (layer + 3) / 4)
		maxNodes += layer;
	nodes = AllocAligned<QBVHNode>(maxNodes);
	for (u_int i = 0; i < maxNodes; ++i)
		nodes[i] = QBVHNode();

	// The arrays that will contain
	// - the bounding boxes for all triangles
	// - the centroids for all triangles	
	BBox *primsBboxes = new BBox[nPrims];
	Point *primsCentroids = new Point[nPrims];
	// The bouding volume of all the centroids
	BBox centroidsBbox;
	
	// Fill each base array
	for (u_int i = 0; i < vPrims.size(); ++i) {
		// This array will be reorganized during construction. 
		primsIndexes[i] = i;

		// Compute the bounding box for the triangle
		primsBboxes[i] = vPrims[i]->WorldBound();
		primsBboxes[i].Expand(RAY_EPSILON);
		primsCentroids[i] = (primsBboxes[i].pMin +
			primsBboxes[i].pMax) * .5f;

		// Update the global bounding boxes
		worldBound = Union(worldBound, primsBboxes[i]);
		centroidsBbox = Union(centroidsBbox, primsCentroids[i]);
	}

	// Arbitrarily take the last primitive for the last 3
	primsIndexes[nPrims] = nPrims - 1;
	primsIndexes[nPrims + 1] = nPrims - 1;
	primsIndexes[nPrims + 2] = nPrims - 1;

	// Recursively build the tree
	std::stringstream ss;
	ss << "Building QBVH, primitives: " << nPrims << ", initial nodes: " << maxNodes;
	luxError(LUX_NOERROR, LUX_DEBUG, ss.str().c_str());
	nQuads = 0;
	BuildTree(0, nPrims, primsIndexes, primsBboxes, primsCentroids,
		worldBound, centroidsBbox, -1, 0, 0);

	prims = AllocAligned<boost::shared_ptr<Primitive> >(4 * nQuads);
	nQuads = 0;
	PreSwizzle(0, primsIndexes, vPrims);
	ss.str("");
	ss << "QBVH completed with " << nNodes << "/" << maxNodes << " nodes";
	luxError(LUX_NOERROR, LUX_DEBUG, ss.str().c_str());
	
	// Release temporary memory
	delete[] primsBboxes;
	delete[] primsCentroids;
	delete[] primsIndexes;
}

/***************************************************/
void QBVHAccel::BuildTree(u_int start, u_int end, u_int *primsIndexes,
	BBox *primsBboxes, Point *primsCentroids, const BBox &nodeBbox,
	const BBox &centroidsBbox, int32_t parentIndex, int32_t childIndex, int depth)
{
	// Create a leaf ?
	//********
	if (end - start <= maxPrimsPerLeaf) {
		CreateTempLeaf(parentIndex, childIndex, start, end, nodeBbox);
		return;
	}

	int32_t currentNode = parentIndex;
	int32_t leftChildIndex = childIndex;
	int32_t rightChildIndex = childIndex + 1;
	
	// Number of primitives in each bin
	int bins[NB_BINS];
	// Bbox of the primitives in the bin
	BBox binsBbox[NB_BINS];
	
	//--------------
	// Fill in the bins, considering all the primitives when a given
	// threshold is reached, else considering only a portion of the
	// primitives for the binned-SAH process. Also compute the bins bboxes
	// for the primitives. 

	for (int i = 0; i < NB_BINS; ++i)
		bins[i] = 0;

	u_int step = (end - start < fullSweepThreshold) ? 1 : skipFactor;

	// Choose the split axis, taking the axis of maximum extent for the
	// centroids (else weird cases can occur, where the maximum extent axis
	// for the nodeBbox is an axis of 0 extent for the centroids one.).
	int axis = centroidsBbox.MaximumExtent();

	// Precompute values that are constant with respect to the current
	// primitive considered.
	float k0 = centroidsBbox.pMin[axis];
	float k1 = NB_BINS / (centroidsBbox.pMax[axis] - k0);
	
	// If the bbox is a point, create a leaf, hoping there are not more
	// than 64 primitives that share the same center.
	if (isinf(k1)) {
		CreateTempLeaf(parentIndex, childIndex, start, end, nodeBbox);
		return;
	}

	for (u_int i = start; i < end; i += step) {
		u_int primIndex = primsIndexes[i];
		
		// Binning is relative to the centroids bbox and to the
		// primitives' centroid.
		int binId = min(NB_BINS - 1, Floor2Int(k1 * (primsCentroids[primIndex][axis] - k0)));

		bins[binId]++;
		binsBbox[binId] = Union(binsBbox[binId], primsBboxes[primIndex]);
	}

	//--------------
	// Evaluate where to split.

	// Cumulative number of primitives in the bins from the first to the
	// ith, and from the last to the ith.
	int nbPrimsLeft[NB_BINS];
	int nbPrimsRight[NB_BINS];
	// The corresponding cumulative bounding boxes.
	BBox bboxesLeft[NB_BINS];
	BBox bboxesRight[NB_BINS];

	// The corresponding volumes.
	float vLeft[NB_BINS];
	float vRight[NB_BINS];	

	BBox currentBboxLeft, currentBboxRight;
	int currentNbLeft = 0, currentNbRight = 0;

	for (int i = 0; i < NB_BINS; ++i) {
		//-----
		// Left side
		// Number of prims
		currentNbLeft += bins[i];
		nbPrimsLeft[i] = currentNbLeft;
		// Prims bbox
		currentBboxLeft = Union(currentBboxLeft, binsBbox[i]);
		bboxesLeft[i] = currentBboxLeft;
		// Volume
		vLeft[i] = currentBboxLeft.Volume();
		

		//-----
		// Right side
		// Number of prims
		int rightIndex = NB_BINS - 1 - i;
		currentNbRight += bins[rightIndex];
		nbPrimsRight[rightIndex] = currentNbRight;
		// Prims bbox
		currentBboxRight = Union(currentBboxRight, binsBbox[rightIndex]);
		bboxesRight[rightIndex] = currentBboxRight;
		// Volume
		vRight[rightIndex] = currentBboxRight.Volume();
	}

	int minBin = -1;
	float minCost = INFINITY;
	// Find the best split axis,
	// there must be at least a bin on the right side
	for (int i = 0; i < NB_BINS - 1; ++i) {
		float cost = vLeft[i] * nbPrimsLeft[i] + vRight[i + 1] * nbPrimsRight[i + 1];
		if (cost < minCost) {
			minBin = i;
			minCost = cost;
		}
	}

	// Create an intermediate node if the depth indicates to do so.
	// Register the split axis.
	if (depth % 2 == 0) {
		currentNode = CreateIntermediateNode(parentIndex, childIndex, nodeBbox);
		leftChildIndex = 0;
		rightChildIndex = 2;

		nodes[currentNode].axisMain = axis;
	} else {
		if (childIndex == 0) {
			//left subnode
			nodes[currentNode].axisSubLeft = axis;
		} else {
			//right subnode
			nodes[currentNode].axisSubRight = axis;
		}
	}

	//-----------------
	// Make the partition, in a "quicksort partitioning" way,
	// the pivot being the position of the split plane
	// (no more binId computation)
	// track also the bboxes (primitives and centroids)
	// for the left and right halves.

	// The split plane coordinate is the coordinate of the end of
	// the chosen bin along the split axis
	float splitPos = centroidsBbox.pMin[axis] + (minBin + 1) *
		(centroidsBbox.pMax[axis] - centroidsBbox.pMin[axis]) / NB_BINS;


	BBox leftChildBbox, rightChildBbox;
	BBox leftChildCentroidsBbox, rightChildCentroidsBbox;

	u_int storeIndex = start;
	for (u_int i = start; i < end; ++i) {
		u_int primIndex = primsIndexes[i];
		
		if (primsCentroids[primIndex][axis] <= splitPos) {
			// Swap
			primsIndexes[i] = primsIndexes[storeIndex];
			primsIndexes[storeIndex] = primIndex;
			++storeIndex;
			
			// Update the bounding boxes,
			// this triangle is on the left side
			leftChildBbox = Union(leftChildBbox, primsBboxes[primIndex]);
			leftChildCentroidsBbox = Union(leftChildCentroidsBbox, primsCentroids[primIndex]);
		} else {
			// Update the bounding boxes,
			// this triangle is on the right side.
			rightChildBbox = Union(rightChildBbox, primsBboxes[primIndex]);
			rightChildCentroidsBbox = Union(rightChildCentroidsBbox, primsCentroids[primIndex]);
		}
	}

	// Build recursively
	BuildTree(start, storeIndex, primsIndexes, primsBboxes, primsCentroids,
		leftChildBbox, leftChildCentroidsBbox, currentNode,
		leftChildIndex, depth + 1);
	BuildTree(storeIndex, end, primsIndexes, primsBboxes, primsCentroids,
		rightChildBbox, rightChildCentroidsBbox, currentNode,
		rightChildIndex, depth + 1);
}

/***************************************************/
void QBVHAccel::CreateTempLeaf(int32_t parentIndex, int32_t childIndex,
	u_int start, u_int end, const BBox &nodeBbox)
{
	// The leaf is directly encoded in the intermediate node.
	if (parentIndex < 0) {
		// The entire tree is a leaf
		nNodes = 1;
		parentIndex = 0;
	}

	// Encode the leaf in the original way,
	// it will be transformed to a preswizzled format in a post-process.
	
	u_int nbPrimsTotal = end - start;
	
	QBVHNode &node = nodes[parentIndex];

	node.SetBBox(childIndex, nodeBbox);

	if (nbPrimsTotal == 0) {
		node.InitializeLeaf(childIndex, 0, 0);
		return;
	}

	// Next multiple of 4, divided by 4
	u_int quads = (nbPrimsTotal + 3) >> 2;
	
	// Use the same encoding as the final one, but with a different meaning.
	node.InitializeLeaf(childIndex, quads, start);

	nQuads += quads;
}

void QBVHAccel::PreSwizzle(int32_t nodeIndex, u_int *primsIndexes,
	const vector<boost::shared_ptr<Primitive> > &vPrims)
{
	for (int i = 0; i < 4; ++i) {
		if (nodes[nodeIndex].ChildIsLeaf(i))
			CreateSwizzledLeaf(nodeIndex, i, primsIndexes, vPrims);
		else
			PreSwizzle(nodes[nodeIndex].children[i], primsIndexes, vPrims);
	}
}

void QBVHAccel::CreateSwizzledLeaf(int32_t parentIndex, int32_t childIndex,
	u_int *primsIndexes, const vector<boost::shared_ptr<Primitive> > &vPrims)
{
	QBVHNode &node = nodes[parentIndex];
	if (node.LeafIsEmpty(childIndex))
		return;
	const u_int startQuad = nQuads;
	const u_int nbQuads = node.NbQuadsInLeaf(childIndex);

	u_int primOffset = node.FirstQuadIndexForLeaf(childIndex);
	u_int primNum = 4 * nQuads;

	for (u_int q = 0; q < nbQuads; ++q) {
		for (int i = 0; i < 4; ++i) {
			new (&prims[primNum]) boost::shared_ptr<Primitive>(vPrims[primsIndexes[primOffset]]);
			++primOffset;
			++primNum;
		}
	}
	nQuads += nbQuads;
	node.InitializeLeaf(childIndex, nbQuads, startQuad);
}

int32_t QBVHNode::BBoxIntersect(__m128 sseOrig[3], __m128 sseInvDir[3],
	const __m128 &sseTMin, const __m128 &sseTMax,
	const int sign[3]) const
{
	__m128 tMin = sseTMin;
	__m128 tMax = sseTMax;

	// X coordinate
	tMin = _mm_max_ps(tMin, _mm_mul_ps(_mm_sub_ps(bboxes[sign[0]][0],
		sseOrig[0]), sseInvDir[0]));
	tMax = _mm_min_ps(tMax, _mm_mul_ps(_mm_sub_ps(bboxes[1 - sign[0]][0],
		sseOrig[0]), sseInvDir[0]));

	// Y coordinate
	tMin = _mm_max_ps(tMin, _mm_mul_ps(_mm_sub_ps(bboxes[sign[1]][1],
		sseOrig[1]), sseInvDir[1]));
	tMax = _mm_min_ps(tMax, _mm_mul_ps(_mm_sub_ps(bboxes[1 - sign[1]][1],
		sseOrig[1]), sseInvDir[1]));

	// Z coordinate
	tMin = _mm_max_ps(tMin, _mm_mul_ps(_mm_sub_ps(bboxes[sign[2]][2],
		sseOrig[2]), sseInvDir[2]));
	tMax = _mm_min_ps(tMax, _mm_mul_ps(_mm_sub_ps(bboxes[1 - sign[2]][2],
		sseOrig[2]), sseInvDir[2]));

	//return the visit flags
	return _mm_movemask_ps(_mm_cmpge_ps(tMax, tMin));
		
	//--------------------
	//sort the subnodes using the axis directions
	//(not the actual intersection distance, the article reporting
	//better performances - less intersections - when using that)...
	/*if (sign_[m_axis_main] == 1) {
		//positive => childrens will be {0,1}, {2,3}.
		if (sign_[m_axis_subleft] == 1) {
			//it will be 0,1
			bbox_order_[3] = 0;
			bbox_order_[2] = 1;
		} else {
			bbox_order_[3] = 1;
			bbox_order_[2] = 0;
		}

		if (sign_[m_axis_subright] == 1) {
			//it will be 2,3
			bbox_order_[1] = 2;
			bbox_order_[0] = 3;
		} else {
			bbox_order_[1] = 3;
			bbox_order_[0] = 2;
		}
	} else {
		//negative => childrens will be {2,3}, {0,1}
		if (sign_[m_axis_subright] == 1) {
			//it will be 2,3
			bbox_order_[3] = 2;
			bbox_order_[2] = 3;
		} else {
			bbox_order_[3] = 3;
			bbox_order_[2] = 2;
		}
		
		if (sign_[m_axis_subleft] == 1) {
			//it will be 0,1
			bbox_order_[1] = 0;
			bbox_order_[0] = 1;
		} else {
			bbox_order_[1] = 1;
			bbox_order_[0] = 0;
		}
		}*/
}

/***************************************************/
bool QBVHAccel::Intersect(const Ray &ray, Intersection *isect) const
{
	//------------------------------
	// Prepare the ray for intersection
	__m128 sseTMin = _mm_set1_ps(ray.mint);
	__m128 sseTMax = _mm_set1_ps(ray.maxt);
	__m128 sseInvDir[3];
	sseInvDir[0] = _mm_set1_ps(1.f / ray.d.x);
	sseInvDir[1] = _mm_set1_ps(1.f / ray.d.y);
	sseInvDir[2] = _mm_set1_ps(1.f / ray.d.z);

	__m128 sseOrig[3];
	sseOrig[0] = _mm_set1_ps(ray.o.x);
	sseOrig[1] = _mm_set1_ps(ray.o.y);
	sseOrig[2] = _mm_set1_ps(ray.o.z);

	int signs[3];
	ray.GetDirectionSigns(signs);

	//------------------------------
	// Main loop
	bool hit = false;
	// The nodes stack, 256 nodes should be enough
	int todoNode = 0; // the index in the stack
	int32_t nodeStack[64];
	nodeStack[0] = 0; // first node to handle: root node
	
	while (todoNode >= 0) {
		// Leaves are identified by a negative index
		if (!QBVHNode::IsLeaf(nodeStack[todoNode])) {
			QBVHNode &node = nodes[nodeStack[todoNode]];
			--todoNode;
			
			const int32_t visit = node.BBoxIntersect(sseOrig, sseInvDir,
				sseTMin, sseTMax, signs);

			const int32_t nodeIdx = (signs[node.axisMain] << 2) |
				(signs[node.axisSubLeft] << 1) |
				(signs[node.axisSubRight]);
			
			boost::int16_t bboxOrder = pathTable[visit * 8 + nodeIdx];

			// Push on the stack, if the bbox is hit by the ray
			for (int i = 0; i < 4; ++i) {
				if (bboxOrder & 0x4)
					break;
				++todoNode;
				nodeStack[todoNode] = node.children[bboxOrder & 0x3];
				bboxOrder >>= 4;
			}
		} else {
			//----------------------
			// It is a leaf,
			// all the informations are encoded in the index
			const int32_t leafData = nodeStack[todoNode];
			--todoNode;
			
			if (QBVHNode::IsEmpty(leafData))
				continue;

			// Perform intersection
			const u_int nbQuadPrimitives = QBVHNode::NbQuadPrimitives(leafData);
			
			const u_int offset = QBVHNode::FirstQuadIndex(leafData);

			for (u_int primNumber = 4 * offset; primNumber < 4 * (offset + nbQuadPrimitives); ++primNumber) {
				hit |= prims[primNumber]->Intersect(ray, isect);
			}

			//update the t max value.
			sseTMax = _mm_min_ps(sseTMax, _mm_set1_ps(ray.maxt));
		}//end of the else
	}

	return hit;
}

/***************************************************/
bool QBVHAccel::IntersectP(const Ray &ray) const
{
	//------------------------------
	// Prepare the ray for intersection
	__m128 sseTMin = _mm_set1_ps(ray.mint);
	__m128 sseTMax = _mm_set1_ps(ray.maxt);
	__m128 sseInvDir[3];
	sseInvDir[0] = _mm_set1_ps(1.f / ray.d.x);
	sseInvDir[1] = _mm_set1_ps(1.f / ray.d.y);
	sseInvDir[2] = _mm_set1_ps(1.f / ray.d.z);

	__m128 sseOrig[3];
	sseOrig[0] = _mm_set1_ps(ray.o.x);
	sseOrig[1] = _mm_set1_ps(ray.o.y);
	sseOrig[2] = _mm_set1_ps(ray.o.z);

	//------------------------------
	// Main loop
	// The nodes stack, 256 nodes should be enough
	int todoNode = 0; // the index in the stack
	int32_t nodeStack[64];
	nodeStack[0] = 0; // first node to handle: root node

	int signs[3];
	ray.GetDirectionSigns(signs);

	while (todoNode >= 0) {
		// Leaves are identified by a negative index
		if (!QBVHNode::IsLeaf(nodeStack[todoNode])) {
			QBVHNode &node = nodes[nodeStack[todoNode]];
			--todoNode;

			const int32_t visit = node.BBoxIntersect(sseOrig, sseInvDir,
				sseTMin, sseTMax, signs);

			const int32_t nodeIdx = (signs[node.axisMain] << 2) |
				(signs[node.axisSubLeft] << 1) |
				(signs[node.axisSubRight]);
			
			boost::int16_t bboxOrder = pathTable[visit * 8 + nodeIdx];

			// Push on the stack, if the bbox is hit by the ray
			for (int i = 0; i < 4; i++) {
				if (bboxOrder & 0x4)
					break;
				++todoNode;
				nodeStack[todoNode] = node.children[bboxOrder & 0x3];
				bboxOrder = bboxOrder >> 4;
			}
		} else {
			//----------------------
			// It is a leaf,
			// all the informations are encoded in the index
			const int32_t leafData = nodeStack[todoNode];
			--todoNode;
			
			if (QBVHNode::IsEmpty(leafData))
				continue;

			// Perform intersection
			const u_int nbQuadPrimitives = QBVHNode::NbQuadPrimitives(leafData);
			
			const u_int offset = QBVHNode::FirstQuadIndex(leafData);

			for (u_int primNumber = 4 * offset; primNumber < 4 * (offset + nbQuadPrimitives); ++primNumber) {
				if (prims[primNumber]->IntersectP(ray))
					return true;
			}
		} // end of the else
	}

	return false;
}

/***************************************************/
QBVHAccel::~QBVHAccel()
{
	for (u_int i = 0; i < nPrims; ++i)
		prims[i].~shared_ptr();
	FreeAligned(prims);
	FreeAligned(nodes);
}

/***************************************************/
BBox QBVHAccel::WorldBound() const
{
	return worldBound;
}

void QBVHAccel::GetPrimitives(vector<boost::shared_ptr<Primitive> > &primitives)
{
	primitives.reserve(nPrims);
	for(u_int i = 0; i < nPrims; ++i)
		primitives.push_back(prims[i]);
}

Aggregate* QBVHAccel::CreateAccelerator(const vector<boost::shared_ptr<Primitive> > &prims, const ParamSet &ps)
{
	int maxPrimsPerLeaf = ps.FindOneInt("maxprimsperleaf", 4);
	float fullSweepThreshold = ps.FindOneFloat("fullsweepthreshold", 4 * maxPrimsPerLeaf);
	int skipFactor = ps.FindOneInt("skipfactor", 1);
	return new QBVHAccel(prims, maxPrimsPerLeaf, fullSweepThreshold, skipFactor);

}

static DynamicLoader::RegisterAccelerator<QBVHAccel> r("qbvh");

}
