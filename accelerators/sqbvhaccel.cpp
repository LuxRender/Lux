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

#include "sqbvhaccel.h"
#include "shapes/mesh.h"
#include "paramset.h"
#include "dynload.h"
#include "error.h"
#include "qbvhaccel.h"

namespace lux
{

SQBVHAccel::SQBVHAccel(const vector<boost::shared_ptr<Primitive> > &p,
	u_int mp, u_int fst, u_int sf, float a) : alpha(a) {
	maxPrimsPerLeaf = mp;
	fullSweepThreshold = fst;
	skipFactor = sf;

	// Refine all primitives
	vector<boost::shared_ptr<Primitive> > vPrims;
	const PrimitiveRefinementHints refineHints(false);
	for (u_int i = 0; i < p.size(); ++i) {
		if(p[i]->CanIntersect())
			vPrims.push_back(p[i]);
		else
			p[i]->Refine(vPrims, refineHints, p[i]);
	}

	// Initialize primitives for _QBVHAccel_
	nPrims = vPrims.size();

	// The number of nodes depends on the number of primitives,
	// and is bounded by 2 * nPrims - 1.
	// Even if there will normally have at least 4 primitives per leaf,
	// it is not always the case => continue to use the normal bounds.
	nNodes = 0;
	maxNodes = 1;
	for (u_int layer = ((nPrims + maxPrimsPerLeaf - 1) / maxPrimsPerLeaf + 3) / 4; layer > 1; layer = (layer + 3) / 4)
		maxNodes += layer;
	nodes = AllocAligned<QBVHNode>(maxNodes);
	for (int i = 0; i < 4; ++i)
		nodesPrims[i].resize(maxNodes);
	for (u_int i = 0; i < maxNodes; ++i)
		nodes[i] = QBVHNode();

	// Temporary data for building
	std::vector<u_int> primsIndexesList;
	// The arrays that will contain
	// - the bounding boxes for all triangles
	// - the centroids for all triangles	
	BBox *primsBboxes = new BBox[nPrims];
	Point *primsCentroids = new Point[nPrims];
	// The bouding volume of all the centroids
	BBox centroidsBbox;
	
	// Fill each base array
	for (u_int i = 0; i < nPrims; ++i) {
		// This array will be reorganized during construction. 
		primsIndexesList.push_back(i);

		// Compute the bounding box for the triangle
		primsBboxes[i] = vPrims[i]->WorldBound();
		primsBboxes[i].Expand(MachineEpsilon::E(primsBboxes[i]));
		primsCentroids[i] = (primsBboxes[i].pMin +
			primsBboxes[i].pMax) * .5f;

		// Update the global bounding boxes
		worldBound = Union(worldBound, primsBboxes[i]);
		centroidsBbox = Union(centroidsBbox, primsCentroids[i]);
	}

	// Recursively build the tree
	LOG(LUX_DEBUG,LUX_NOERROR) << "Building SQBVH, primitives: " << nPrims << ", initial nodes: " << maxNodes;

	nNodes = 0;
	nQuads = 0;
	objectSplitCount = 0;
	spatialSplitCount = 0;
	BuildTree(primsIndexesList, vPrims, primsBboxes, primsCentroids, worldBound, centroidsBbox,
			-1, 0, 0);

	prims = AllocAligned<boost::shared_ptr<QuadPrimitive> >(nQuads);
	nQuads = 0;
	// Temporary data for building
	u_int refCount = 0;
	for (int i = 0; i < 4; ++i) {
		for(u_int j = 0; j < nNodes; ++j)
			refCount += nodesPrims[i][j].size();
	}
	u_int *primsIndexes = new u_int[refCount + 3]; // For the case where
	// the last quad would begin at the last primitive
	// (or the second or third last primitive)
	u_int index = 0;
	for(int i = 0; i < 4; ++i) {
		for (u_int j = 0; j < nNodes; ++j) {
			u_int nbPrimsTotal = nodesPrims[i][j].size();

			if (nbPrimsTotal > 0) {
				const u_int start = index;
				for (u_int k = 0; k < nbPrimsTotal; ++k)
					primsIndexes[index++] = nodesPrims[i][j][k];
				
				QBVHNode &node = nodes[j];
				// Next multiple of 4, divided by 4
				u_int quads = (nbPrimsTotal + 3) / 4;
				// Use the same encoding as the final one, but with a different meaning.
				node.InitializeLeaf(i, quads, start);
			}
		}
	}
	primsIndexes[index++] = nPrims - 1;
	primsIndexes[index++] = nPrims - 1;
	primsIndexes[index++] = nPrims - 1;
	// Free memory
	for (int i = 0; i < 4; ++i)
		nodesPrims[i].clear();
	
	PreSwizzle(0, primsIndexes, vPrims);
	LOG(LUX_DEBUG,LUX_NOERROR) << "QBVH completed with " << nNodes << "/" << maxNodes << " nodes";
	
	// Collect statistics
	SAHCost = 0.f;
	avgLeafPrimReferences = 0.f;
	maxDepth = 0;
	nodeCount = 0;
	noEmptyLeafCount = 0;
	emptyLeafCount = 0;
	primReferences = CollectStatistics(0);
	avgLeafPrimReferences /= noEmptyLeafCount;
	
	// Print the statistics
	LOG(LUX_DEBUG, LUX_NOERROR) << "SQBVH SAH total cost: " << SAHCost;
	LOG(LUX_DEBUG, LUX_NOERROR) << "SQBVH object split counts: " << objectSplitCount;
	LOG(LUX_DEBUG, LUX_NOERROR) << "SQBVH spatial split counts: " << spatialSplitCount;
	LOG(LUX_DEBUG, LUX_NOERROR) << "SQBVH max. depth: " << maxDepth;
	LOG(LUX_DEBUG, LUX_NOERROR) << "SQBVH node count: " << nodeCount;
	LOG(LUX_DEBUG, LUX_NOERROR) << "SQBVH empty leaf count: " << emptyLeafCount;
	LOG(LUX_DEBUG, LUX_NOERROR) << "SQBVH not empty leaf count: " << noEmptyLeafCount;
	LOG(LUX_DEBUG, LUX_NOERROR) << "SQBVH avg. primitive references per leaf: " << avgLeafPrimReferences;
	LOG(LUX_DEBUG, LUX_NOERROR) << "SQBVH primitive references: " << primReferences << "/" << nPrims;

	// Release temporary memory
	delete[] primsBboxes;
	delete[] primsCentroids;
	delete[] primsIndexes;
}

void SQBVHAccel::BuildTree(const std::vector<u_int> &primsIndexes,
		const vector<boost::shared_ptr<Primitive> > &vPrims,
		const BBox *primsBboxes, const Point *primsCentroids,
		const BBox &nodeBbox, const BBox &centroidsBbox,
		const int32_t parentIndex, const int32_t childIndex,
		const int depth) {
	const u_int nPrimsIndexes = primsIndexes.size();

	// Create a leaf ?
	//********
	if (depth > 64 || nPrimsIndexes <= maxPrimsPerLeaf) {
		if (depth > 64) {
			LOG(LUX_WARNING,LUX_LIMIT) << "Maximum recursion depth reached while constructing SQBVH, forcing a leaf node";
			if (nPrimsIndexes > 64) {
				LOG(LUX_ERROR, LUX_LIMIT) << "SQBVH unable to handle geometry, too many primitives in leaf";
			}
		}

		CreateTempLeaf(parentIndex, childIndex, 0, primsIndexes.size(), nodeBbox);
		nodesPrims[childIndex][parentIndex].insert(nodesPrims[childIndex][parentIndex].begin(),
			primsIndexes.begin(), primsIndexes.end());
		return;
	}

	//--------------------------------------------------------------------------
	// Look for object split
	//--------------------------------------------------------------------------

	int objectSplitAxis;
	const float objectSplitPos = BuildObjectSplit(0, primsIndexes.size(), &primsIndexes[0], primsBboxes,
		primsCentroids, centroidsBbox, objectSplitAxis);

	if (isnan(objectSplitPos)) {
		if (nPrimsIndexes > 64) {
			LOG(LUX_ERROR, LUX_LIMIT) << "SQBVH unable to handle geometry, too many primitives with the same centroid";
		}

		CreateTempLeaf(parentIndex, childIndex, 0, primsIndexes.size(), nodeBbox);
		nodesPrims[childIndex][parentIndex].insert(nodesPrims[childIndex][parentIndex].begin(),
			primsIndexes.begin(), primsIndexes.end());
		return;
	}

	BBox objectLeftChildBbox, objectRightChildBbox;
	int objectLeftChildReferences = 0;
	int objectRightChildReferences = 0;
	for (u_int i = 0; i < primsIndexes.size(); ++i) {
		const u_int primIndex = primsIndexes[i];

		if (primsCentroids[primIndex][objectSplitAxis] <= objectSplitPos) {
			objectLeftChildBbox = Union(objectLeftChildBbox, primsBboxes[primIndex]);
			++objectLeftChildReferences;
		} else {
			objectRightChildBbox = Union(objectRightChildBbox, primsBboxes[primIndex]);
			++objectRightChildReferences;
		}
	}
	
	if (!Overlaps(objectLeftChildBbox, objectLeftChildBbox, nodeBbox) ||
			!Overlaps(objectRightChildBbox, objectRightChildBbox, nodeBbox)) {
		LOG(LUX_WARNING,LUX_LIMIT) << "Not overlapping parent/children in a SQBVH node, forcing a leaf node";

		if (nPrimsIndexes > 64) {
			LOG(LUX_ERROR, LUX_LIMIT) << "SQBVH unable to handle geometry, too many primitives in a not overlapping parent/children node";
		}

		CreateTempLeaf(parentIndex, childIndex, 0, primsIndexes.size(), nodeBbox);
		nodesPrims[childIndex][parentIndex].insert(nodesPrims[childIndex][parentIndex].begin(),
			primsIndexes.begin(), primsIndexes.end());
		return;
	}

	//--------------------------------------------------------------------------
	// Check if a spatial split is worth trying spatial split
	// Test: (SAH intersection left/right child) / (SAH _root_ node) > alpha
	//--------------------------------------------------------------------------

	bool doObjectSplit = true;

	float spatialSplitPos;
	int spatialSplitAxis;
	int spatialLeftChildReferences, spatialRightChildReferences;
	BBox spatialLeftChildBbox, spatialRightChildBbox;

	BBox childIntersectionBbox;
	if (Overlaps(childIntersectionBbox, objectLeftChildBbox, objectRightChildBbox) &&
			(childIntersectionBbox.SurfaceArea() / worldBound.SurfaceArea() > alpha)) {
		// It is worth trying a spatial split

		spatialSplitPos = BuildSpatialSplit(0, primsIndexes.size(), &primsIndexes[0],
				vPrims, primsBboxes, primsCentroids, nodeBbox, centroidsBbox,
				spatialSplitAxis, spatialLeftChildBbox, spatialRightChildBbox,
				spatialLeftChildReferences, spatialRightChildReferences);

		if (!isnan(spatialSplitPos)) {
			// Check if spatial split is better than object split

			const float objectSplitCost = objectLeftChildBbox.SurfaceArea() * QuadCount(objectLeftChildReferences) +
				objectRightChildBbox.SurfaceArea() * QuadCount(objectRightChildReferences);
			const float spatialSplitCost = spatialLeftChildBbox.SurfaceArea() * QuadCount(spatialLeftChildReferences) +
				spatialRightChildBbox.SurfaceArea() * QuadCount(spatialRightChildReferences);

			doObjectSplit = (spatialSplitCost >= objectSplitCost);
		}
	}

	std::vector<u_int> leftPrimsIndexes, rightPrimsIndexes;
	BBox *leftChildBbox, *rightChildBbox;
	BBox leftChildCentroidsBbox, rightChildCentroidsBbox;
	if (doObjectSplit) {
		// Do object split
		for (u_int i = 0; i < primsIndexes.size(); ++i) {
			const u_int primIndex = primsIndexes[i];

			if (primsCentroids[primIndex][objectSplitAxis] <= objectSplitPos) {
				leftPrimsIndexes.push_back(primIndex);

				// Update the bounding boxes,
				// this triangle is on the left side
				leftChildCentroidsBbox = Union(leftChildCentroidsBbox, primsCentroids[primIndex]);
			} else {
				rightPrimsIndexes.push_back(primIndex);

				// Update the bounding boxes,
				// this triangle is on the right side.
				rightChildCentroidsBbox = Union(rightChildCentroidsBbox, primsCentroids[primIndex]);
			}
		}
		
		leftChildBbox = &objectLeftChildBbox;
		rightChildBbox = &objectRightChildBbox;
		++objectSplitCount;
	} else {
		// Do spatial split
		for (u_int i = 0; i < primsIndexes.size(); ++i) {
			u_int primIndex = primsIndexes[i];

			if (primsBboxes[primIndex].Overlaps(spatialLeftChildBbox)) {
				leftPrimsIndexes.push_back(primIndex);

				// Update the bounding boxes,
				// this triangle is on the left side
				leftChildCentroidsBbox = Union(leftChildCentroidsBbox, primsCentroids[primIndex]);
			}

			if (primsBboxes[primIndex].Overlaps(spatialRightChildBbox)) {
				rightPrimsIndexes.push_back(primIndex);

				// Update the bounding boxes,
				// this triangle is on the right side.
				rightChildCentroidsBbox = Union(rightChildCentroidsBbox, primsCentroids[primIndex]);
			}
		}
		
		leftChildBbox = &spatialLeftChildBbox;
		rightChildBbox = &spatialRightChildBbox;
		++spatialSplitCount;
	}

	int32_t currentNode = parentIndex;
	int32_t leftChildIndex = childIndex;
	int32_t rightChildIndex = childIndex + 1;

	// Create an intermediate node if the depth indicates to do so.
	// Register the split axis.
	if (depth % 2 == 0) {
		currentNode = CreateIntermediateNode(parentIndex, childIndex, nodeBbox);
		if (maxNodes != nodesPrims[0].size()) {
			for (int i = 0; i < 4; ++i)
				nodesPrims[i].resize(maxNodes);
		}

		leftChildIndex = 0;
		rightChildIndex = 2;
	}

	// Build recursively
	BuildTree(leftPrimsIndexes, vPrims, primsBboxes, primsCentroids,
		*leftChildBbox, leftChildCentroidsBbox, currentNode,
		leftChildIndex, depth + 1);
	BuildTree(rightPrimsIndexes, vPrims, primsBboxes, primsCentroids,
		*rightChildBbox, rightChildCentroidsBbox, currentNode,
		rightChildIndex, depth + 1);
}

float SQBVHAccel::BuildSpatialSplit(const u_int start, const u_int end,
		const u_int *primsIndexes, const vector<boost::shared_ptr<Primitive> > &vPrims,
		const BBox *primsBboxes, const Point *primsCentroids, const BBox &nodeBbox, 
		const BBox &centroidsBbox, int &axis, BBox &leftChildBBox, BBox &rightChildBBox,
		int &spatialLeftChildReferences, int &spatialRightChildReferences) {
	// Choose the split axis, taking the axis of maximum extent for the
	// centroids (else weird cases can occur, where the maximum extent axis
	// for the nodeBbox is an axis of 0 extent for the centroids one.).
	axis = centroidsBbox.MaximumExtent();
	
	// Precompute values that are constant with respect to the current
	// primitive considered.
	const float k0 = nodeBbox.pMin[axis];
	const float k1 = (nodeBbox.pMax[axis] - k0) / SPATIAL_SPLIT_BINS;

	// If the bbox is a point
	if (k1 == 0.f)
		return std::numeric_limits<float>::quiet_NaN();

	// Entry and Exit counters as described in SBVH paper
	int entryBins[SPATIAL_SPLIT_BINS];
	int exitBins[SPATIAL_SPLIT_BINS];
	// Bbox of bins
	BBox binsBbox[SPATIAL_SPLIT_BINS];
	BBox binsPrimBbox[SPATIAL_SPLIT_BINS];

	for (int j = 0; j < SPATIAL_SPLIT_BINS; ++j) {
		entryBins[j] = 0;
		exitBins[j] = 0;

		binsBbox[j] = nodeBbox;
		if (j != 0)
			binsBbox[j].pMin[axis] = binsBbox[j - 1].pMax[axis];
		if (j != SPATIAL_SPLIT_BINS - 1)
			binsBbox[j].pMax[axis] = k0 + k1 * (j + 1);
	}

	// Bbox of primitives inside the bins
	for (u_int i = start; i < end; ++i) {
		const u_int primIndex = primsIndexes[i];

		bool entryFound = false;
		bool exitFound = false;
		for (int j = 0; j < SPATIAL_SPLIT_BINS && (!entryFound || !exitFound); ++j) {
			// Check if the primitive is all outside the bin bounding box
			if (!binsBbox[j].Overlaps(primsBboxes[primIndex]))
				continue;

			// Update entry and exit counters
			if (!entryFound && (primsBboxes[primIndex].pMin[axis] <= binsBbox[j].pMax[axis])) {
				entryBins[j] += 1;
				entryFound = true;
			}
			if (!exitFound && (primsBboxes[primIndex].pMax[axis] <= binsBbox[j].pMax[axis])) {
				exitBins[j] += 1;
				exitFound = true;
			}

			// Safety check
			MeshBaryTriangle *tri = dynamic_cast<MeshBaryTriangle *>(vPrims[primIndex].get());
			if (tri == NULL) {
				// SQBVH is able to work only with triangles, it will fall back to
				// standard object split in the case a no-triangle primitive is
				// found
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "A primitive used in a SQBVH isn't a triangle, falling back to 'object split'-only building";
				return std::numeric_limits<float>::quiet_NaN();
			}

			// Clip triangle with bin bounding box
			vector<Point> vertexList;
			vertexList.push_back(tri->GetP(0));
			vertexList.push_back(tri->GetP(1));
			vertexList.push_back(tri->GetP(2));

			vector<Point> clipVertexList = binsBbox[j].ClipPolygon(vertexList);
			if (clipVertexList.size() == 0) // All vertices are outside the bounding box
				continue;

			BBox binPrimBbox;
			for (u_int k = 0; k < clipVertexList.size(); ++k)
				binPrimBbox = Union(binPrimBbox, clipVertexList[k]);
			binPrimBbox.Expand(MachineEpsilon::E(binPrimBbox));
			
			binsPrimBbox[j] = Union(binsPrimBbox[j], binPrimBbox);
		}
	}

	// Evaluate where to split
	int nbPrimsLeft[SPATIAL_SPLIT_BINS];
	int nbPrimsRight[SPATIAL_SPLIT_BINS];
	BBox bboxesLeft[SPATIAL_SPLIT_BINS];
	BBox bboxesRight[SPATIAL_SPLIT_BINS];
	BBox currentBboxLeft, currentBboxRight;
	float areaLeft[SPATIAL_SPLIT_BINS];
	float areaRight[SPATIAL_SPLIT_BINS];
	int currentPrimsLeft = 0;
	int currentPrimsRight = 0;
	for (int j = 0; j < SPATIAL_SPLIT_BINS; ++j) {
		// Left child
		currentPrimsLeft += entryBins[j];
		nbPrimsLeft[j]= currentPrimsLeft;
		currentBboxLeft = Union(currentBboxLeft, binsPrimBbox[j]);
		bboxesLeft[j] = currentBboxLeft;
		areaLeft[j] = bboxesLeft[j].SurfaceArea();
		
		// Right child
		const int rightIndex = SPATIAL_SPLIT_BINS - 1 - j;
		currentPrimsRight += exitBins[rightIndex];
		nbPrimsRight[rightIndex] = currentPrimsRight;
		currentBboxRight = Union(currentBboxRight, binsPrimBbox[rightIndex]);
		bboxesRight[rightIndex] = currentBboxRight;
		areaRight[rightIndex] = bboxesRight[rightIndex].SurfaceArea();
	}

	int minBin = -1;
	float minCost = INFINITY;
	// Find the best split axis,
	// there must be at least a bin on the right side
	for (int j = 0; j < SPATIAL_SPLIT_BINS - 1; ++j) {
		float cost = areaLeft[j] * QuadCount(nbPrimsLeft[j]) +
			areaRight[j + 1] * QuadCount(nbPrimsRight[j + 1]);
		if (cost < minCost) {
			minBin = j;
			minCost = cost;
		}
	}

	leftChildBBox = bboxesLeft[minBin];
	rightChildBBox = bboxesRight[minBin + 1];
	spatialLeftChildReferences = nbPrimsLeft[minBin];
	spatialRightChildReferences = nbPrimsRight[minBin + 1];

	return leftChildBBox.pMax[axis];
}

Aggregate *SQBVHAccel::CreateAccelerator(const vector<boost::shared_ptr<Primitive> > &prims, const ParamSet &ps)
{
	int maxPrimsPerLeaf = ps.FindOneInt("maxprimsperleaf", 4);
	int fullSweepThreshold = ps.FindOneInt("fullsweepthreshold", 4 * maxPrimsPerLeaf);
	int skipFactor = ps.FindOneInt("skipfactor", 1);
	float alpha = ps.FindOneFloat("alpha", 1e-5f);
	return new SQBVHAccel(prims, maxPrimsPerLeaf, fullSweepThreshold, skipFactor, alpha);
}

static DynamicLoader::RegisterAccelerator<SQBVHAccel> r("sqbvh");

}
