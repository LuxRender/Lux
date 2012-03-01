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
	std::vector<u_int> primsIndexesList(nPrims);
	// The arrays that will contain
	// - the bounding boxes for all triangles
	// - the centroids for all triangles	
	std::vector<BBox> primsBboxes(nPrims);
	
	// Fill each base array
	for (u_int i = 0; i < nPrims; ++i) {
		// This array will be reorganized during construction. 
		primsIndexesList[i] = i;

		// Compute the bounding box for the triangle
		primsBboxes[i] = vPrims[i]->WorldBound();

		// Update the global bounding boxes
		worldBound = Union(worldBound, primsBboxes[i]);
	}
	worldBound.Expand(MachineEpsilon::E(worldBound));

	// Recursively build the tree
	LOG(LUX_DEBUG, LUX_NOERROR) << "Building SQBVH, primitives: " << nPrims << ", initial nodes: " << maxNodes;

	nNodes = 0;
	nQuads = 0;
	objectSplitCount = 0;
	spatialSplitCount = 0;
	BuildTree(primsIndexesList, vPrims, primsBboxes, worldBound, -1, 0, 0);

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
				u_int quads = QuadCount(nbPrimsTotal);
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
	LOG(LUX_DEBUG, LUX_NOERROR) << "SQBVH completed with " << nNodes << "/" << maxNodes << " nodes";
	
	// Collect statistics
	SAHCost = 0.f;
	maxDepth = 0;
	nodeCount = 0;
	noEmptyLeafCount = 0;
	emptyLeafCount = 0;
	CollectStatistics(0, 0, worldBound.SurfaceArea(), worldBound);
	avgLeafPrimReferences = primReferences / noEmptyLeafCount;
	
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
	delete[] primsIndexes;
}

void SQBVHAccel::BuildTree(const std::vector<u_int> &primsIndexes,
			const vector<boost::shared_ptr<Primitive> > &vPrims,
			const std::vector<BBox> &primsBboxes, const BBox &nodeBbox,
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

		CreateTempLeaf(parentIndex, childIndex, 0, nPrimsIndexes, nodeBbox);
		nodesPrims[childIndex][parentIndex].insert(nodesPrims[childIndex][parentIndex].begin(),
			primsIndexes.begin(), primsIndexes.end());
		return;
	}

	//--------------------------------------------------------------------------
	// Look for object split
	//--------------------------------------------------------------------------

	int objectSplitAxis;
	BBox objectLeftChildBbox, objectRightChildBbox;
	u_int objectLeftChildReferences, objectRightChildReferences;
	const float objectSplitPos = BuildObjectSplit(primsBboxes, objectSplitAxis,
			objectLeftChildBbox, objectRightChildBbox,
			objectLeftChildReferences, objectRightChildReferences);

	if (isnan(objectSplitPos)) {
		if (nPrimsIndexes > 64) {
			LOG(LUX_ERROR, LUX_LIMIT) << "SQBVH unable to handle geometry, too many primitives with the same centroid";
		}

		CreateTempLeaf(parentIndex, childIndex, 0, nPrimsIndexes, nodeBbox);
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
	u_int spatialLeftChildReferences, spatialRightChildReferences;
	BBox spatialLeftChildBbox, spatialRightChildBbox;

	BBox childIntersectionBbox;
	if (Overlaps(childIntersectionBbox, objectLeftChildBbox, objectRightChildBbox) &&
			(childIntersectionBbox.SurfaceArea() / worldBound.SurfaceArea() > alpha)) {
		// It is worth trying a spatial split

		spatialSplitPos = BuildSpatialSplit(primsIndexes, vPrims,
				primsBboxes, nodeBbox, spatialSplitAxis,
				spatialLeftChildBbox, spatialRightChildBbox,
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
	std::vector<BBox> leftPrimsBbox, rightPrimsBbox;
	BBox *leftBbox, *rightBbox;
	if (doObjectSplit) {
		// Do object split
		DoObjectSplit(primsIndexes, primsBboxes, objectSplitPos, objectSplitAxis,
				objectLeftChildReferences, objectRightChildReferences,
				leftPrimsIndexes, rightPrimsIndexes,
				leftPrimsBbox, rightPrimsBbox);

		leftBbox = &objectLeftChildBbox;
		rightBbox = &objectRightChildBbox;
	} else {
		// Do spatial split
		DoSpatialSplit(primsIndexes, vPrims, primsBboxes, spatialSplitPos, spatialSplitAxis,
				spatialLeftChildReferences, spatialRightChildReferences,
				leftPrimsIndexes, rightPrimsIndexes,
				leftPrimsBbox, rightPrimsBbox,
				spatialLeftChildBbox, spatialRightChildBbox);

		leftBbox = &spatialLeftChildBbox;
		rightBbox = &spatialRightChildBbox;
	}

	leftBbox->Expand(MachineEpsilon::E(*leftBbox));
	rightBbox->Expand(MachineEpsilon::E(*rightBbox));

#if !defined(NDEBUG)
	for (u_int i = 0; i < leftPrimsBbox.size(); ++i) {
		assert (leftBbox->Inside(leftPrimsBbox[i]));
	}

	for (u_int i = 0; i < rightPrimsBbox.size(); ++i) {
		assert (rightBbox->Inside(rightPrimsBbox[i]));
	}
#endif

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
	BuildTree(leftPrimsIndexes, vPrims, leftPrimsBbox, *leftBbox,
			currentNode, leftChildIndex, depth + 1);
	BuildTree(rightPrimsIndexes, vPrims, rightPrimsBbox, *rightBbox,
			currentNode, rightChildIndex, depth + 1);
}

void SQBVHAccel::DoObjectSplit(const std::vector<u_int> &primsIndexes, const std::vector<BBox> &primsBboxes,
		const float objectSplitPos, const int objectSplitAxis,
		const u_int objectLeftChildReferences, const u_int objectRightChildReferences,
		std::vector<u_int> &leftPrimsIndexes, std::vector<u_int> &rightPrimsIndexes,
		std::vector<BBox> &leftPrimsBbox, std::vector<BBox> &rightPrimsBbox) {
	// Do object split
	leftPrimsIndexes.reserve(objectLeftChildReferences);
	rightPrimsIndexes.reserve(objectRightChildReferences);
	leftPrimsBbox.reserve(objectLeftChildReferences);
	rightPrimsBbox.reserve(objectRightChildReferences);

	for (u_int i = 0; i < primsIndexes.size(); ++i) {
		const u_int primIndex = primsIndexes[i];
		const float centroid = (primsBboxes[i].pMin[objectSplitAxis] + primsBboxes[i].pMax[objectSplitAxis]) * .5f;

		if (centroid <= objectSplitPos) {
			leftPrimsIndexes.push_back(primIndex);
			leftPrimsBbox.push_back(primsBboxes[i]);
		} else {
			rightPrimsIndexes.push_back(primIndex);
			rightPrimsBbox.push_back(primsBboxes[i]);
		}
	}

	assert (leftPrimsIndexes.size() == objectLeftChildReferences);
	assert (rightPrimsIndexes.size() == objectRightChildReferences);

	++objectSplitCount;
}

void SQBVHAccel::DoSpatialSplit(const std::vector<u_int> &primsIndexes,
		const vector<boost::shared_ptr<Primitive> > &vPrims, const std::vector<BBox> &primsBboxes,
		const float spatialSplitPos, const int spatialSplitAxis,
		const u_int spatialLeftChildReferences, const u_int spatialRightChildReferences,
		std::vector<u_int> &leftPrimsIndexes, std::vector<u_int> &rightPrimsIndexes,
		std::vector<BBox> &leftPrimsBbox, std::vector<BBox> &rightPrimsBbox,
		BBox &spatialLeftChildBbox, BBox &spatialRightChildBbox) {
	// Do spatial split
	leftPrimsIndexes.reserve(spatialLeftChildReferences);
	rightPrimsIndexes.reserve(spatialRightChildReferences);
	leftPrimsBbox.reserve(spatialLeftChildReferences);
	rightPrimsBbox.reserve(spatialRightChildReferences);

	for (u_int i = 0; i < primsIndexes.size(); ++i) {
		u_int primIndex = primsIndexes[i];

		if (primsBboxes[i].pMin[spatialSplitAxis] <= spatialSplitPos) {
			leftPrimsIndexes.push_back(primIndex);

			// Clip triangle with left bounding box
			vector<Point> vertexList = GetPolygonVertexList(vPrims[primIndex].get());
			assert (vertexList.size() > 0);

			vector<Point> clipVertexList = spatialLeftChildBbox.ClipPolygon(vertexList);
			assert (clipVertexList.size() > 0);

			// Compute the bounding box of the clipped triangle
			BBox primBbox;
			for (u_int k = 0; k < clipVertexList.size(); ++k)
				primBbox = Union(primBbox, clipVertexList[k]);
			assert (primBbox.IsValid());

			leftPrimsBbox.push_back(primBbox);
		}

		if (primsBboxes[i].pMax[spatialSplitAxis] > spatialSplitPos) {
			rightPrimsIndexes.push_back(primIndex);

			// Clip triangle with right bounding box
			vector<Point> vertexList = GetPolygonVertexList(vPrims[primIndex].get());
			assert (vertexList.size() > 0);

			vector<Point> clipVertexList = spatialRightChildBbox.ClipPolygon(vertexList);
			assert (clipVertexList.size() > 0);

			// Compute the bounding box of the clipped triangle
			BBox primBbox;
			for (u_int k = 0; k < clipVertexList.size(); ++k)
				primBbox = Union(primBbox, clipVertexList[k]);
			assert (primBbox.IsValid());
			rightPrimsBbox.push_back(primBbox);
		}
	}

	assert (leftPrimsIndexes.size() == spatialLeftChildReferences);
	assert (rightPrimsIndexes.size() == spatialRightChildReferences);

	++spatialSplitCount;
}

vector<Point> SQBVHAccel::GetPolygonVertexList(const Primitive *prim) const {
	vector<Point> vertexList;

	const MeshBaryTriangle *tri = dynamic_cast<const MeshBaryTriangle *>(prim);
	if (tri != NULL) {
		// It is a triangle
		vertexList.push_back(tri->GetP(0));
		vertexList.push_back(tri->GetP(1));
		vertexList.push_back(tri->GetP(2));
		
		return vertexList;
	}

	const AreaLightPrimitive *alp = dynamic_cast<const AreaLightPrimitive *>(prim);
	if (alp != NULL) {
		// It is an area light primitive
		return GetPolygonVertexList(alp->GetPrimitive().get());
	}
	
	return vertexList;
}

float SQBVHAccel::BuildSpatialSplit(const std::vector<u_int> &primsIndexes,
		const vector<boost::shared_ptr<Primitive> > &vPrims,
		const std::vector<BBox> &primsBboxes, const BBox &nodeBbox,
		int &axis, BBox &leftChildBBox, BBox &rightChildBBox,
		u_int &leftChildReferences, u_int &rightChildReferences) {
	// Build the centroids bounding box
	BBox centroidsBbox;
	for (u_int i = 0; i < primsBboxes.size(); ++i)
		centroidsBbox = Union(centroidsBbox, primsBboxes[i].Center());

	// Choose the split axis, taking the axis of maximum extent for the
	// centroids (else weird cases can occur, where the maximum extent axis
	// for the nodeBbox is an axis of 0 extent for the centroids one.).
	axis = centroidsBbox.MaximumExtent();
	
	// Precompute values that are constant with respect to the current
	// primitive considered.
	const float k0 = nodeBbox.pMin[axis];
	const float k1 = (nodeBbox.pMax[axis] - k0) / SPATIAL_SPLIT_BINS;

	// If the bounding box is a point or too small anyway
	if (k1 < .01f)
		return std::numeric_limits<float>::quiet_NaN();

	// Entry and Exit counters as described in SBVH paper
	int entryBins[SPATIAL_SPLIT_BINS];
	int exitBins[SPATIAL_SPLIT_BINS];
	// Bbox of bins
	BBox binsBbox[SPATIAL_SPLIT_BINS];
	BBox binsPrimBbox[SPATIAL_SPLIT_BINS];

	for (int i = 0; i < SPATIAL_SPLIT_BINS; ++i) {
		entryBins[i] = 0;
		exitBins[i] = 0;

		binsBbox[i] = nodeBbox;
		if (i != 0)
			binsBbox[i].pMin[axis] = binsBbox[i - 1].pMax[axis];
		if (i != SPATIAL_SPLIT_BINS - 1)
			binsBbox[i].pMax[axis] = k0 + k1 * (i + 1);
	}

	// Bbox of primitives inside the bins
	for (u_int i = 0; i < primsIndexes.size(); ++i) {
		bool entryFound = false;
		bool exitFound = false;
		for (int j = 0; j < SPATIAL_SPLIT_BINS && (!entryFound || !exitFound); ++j) {
			// Update entry and exit counters
			if (!entryFound) {
				if ((primsBboxes[i].pMin[axis] <= binsBbox[j].pMax[axis]) ||
						(j == SPATIAL_SPLIT_BINS - 1)) {
					entryBins[j] += 1;
					entryFound = true;
				} else
					continue;
			}

			if (!exitFound && ((primsBboxes[i].pMax[axis] <= binsBbox[j].pMax[axis]) ||
					(j == SPATIAL_SPLIT_BINS - 1))) {
				exitBins[j] += 1;
				exitFound = true;
			}

			vector<Point> vertexList = GetPolygonVertexList(vPrims[primsIndexes[i]].get());
			// Safety check
			if (vertexList.size() == 0) {
				// SQBVH is able to work only with triangles, I will clip the
				// bounding box instead of the primitive
				LOG(LUX_INFO, LUX_UNIMPLEMENT) << "A primitive of type " << typeid(*(vPrims[primsIndexes[i]].get())).name() <<
						", used in a SQBVH, isn't a triangle, falling back to bounding box clipping for building";

				BBox binPrimBbox = primsBboxes[i];
				binPrimBbox.pMax[axis] = binsBbox[j].pMax[axis];
				binsPrimBbox[j] = Union(binsPrimBbox[j], binPrimBbox);
			} else {
				// Clip triangle with bin bounding box
				vector<Point> clipVertexList = binsBbox[j].ClipPolygon(vertexList);
				assert (clipVertexList.size() != 0);

				// Compute the bounding box of the clipped triangle
				BBox binPrimBbox;
				for (u_int k = 0; k < clipVertexList.size(); ++k) {
#if !defined(NDEBUG)
					// Safety check
					BBox binBbox = binsBbox[j];
					binBbox.Expand(MachineEpsilon::E(binBbox));
					assert (binBbox.Inside(clipVertexList[k]));
#endif
					binPrimBbox = Union(binPrimBbox, clipVertexList[k]);
				}
				assert (binPrimBbox.IsValid());

				binsPrimBbox[j] = Union(binsPrimBbox[j], binPrimBbox);
			}
		}

		assert (entryFound);
		assert (exitFound);
	}

	// Evaluate where to split
	u_int nbPrimsLeft[SPATIAL_SPLIT_BINS];
	u_int nbPrimsRight[SPATIAL_SPLIT_BINS];
	BBox bboxesLeft[SPATIAL_SPLIT_BINS];
	BBox bboxesRight[SPATIAL_SPLIT_BINS];
	BBox currentBboxLeft, currentBboxRight;
	float areaLeft[SPATIAL_SPLIT_BINS];
	float areaRight[SPATIAL_SPLIT_BINS];
	int currentPrimsLeft = 0;
	int currentPrimsRight = 0;
	for (int i = 0; i < SPATIAL_SPLIT_BINS; ++i) {
		// Left child
		currentPrimsLeft += entryBins[i];
		nbPrimsLeft[i]= currentPrimsLeft;
		// The bounding box can be not valid if the bins is empty
		if (binsPrimBbox[i].IsValid())
			currentBboxLeft = Union(currentBboxLeft, binsPrimBbox[i]);
		bboxesLeft[i] = currentBboxLeft;
		areaLeft[i] = bboxesLeft[i].SurfaceArea();
		
		// Right child
		const int rightIndex = SPATIAL_SPLIT_BINS - 1 - i;
		currentPrimsRight += exitBins[rightIndex];
		nbPrimsRight[rightIndex] = currentPrimsRight;
		
		// The bounding box can be not valid if the bins is empty
		if (binsPrimBbox[rightIndex].IsValid())
			currentBboxRight = Union(currentBboxRight, binsPrimBbox[rightIndex]);
		bboxesRight[rightIndex] = currentBboxRight;
		areaRight[rightIndex] = bboxesRight[rightIndex].SurfaceArea();
	}

	int minBin = -1;
	float minCost = INFINITY;
	// Find the best split axis,
	// there must be at least a bin on the right side
	for (int i = 0; i < SPATIAL_SPLIT_BINS - 1; ++i) {
		if (binsPrimBbox[i].IsValid() && binsPrimBbox[i + 1].IsValid()) {
			float cost = areaLeft[i] * QuadCount(nbPrimsLeft[i]) +
				areaRight[i + 1] * QuadCount(nbPrimsRight[i + 1]);
			if (cost < minCost) {
				minBin = i;
				minCost = cost;
			}
		}
	}

	if (minBin == -1)
		return std::numeric_limits<float>::quiet_NaN();

	leftChildBBox = bboxesLeft[minBin];
	rightChildBBox = bboxesRight[minBin + 1];
	leftChildReferences = nbPrimsLeft[minBin];
	rightChildReferences = nbPrimsRight[minBin + 1];
	const float splitPos = binsBbox[minBin].pMax[axis];

	assert (leftChildBBox.IsValid());
	assert (rightChildBBox.IsValid());
	assert (leftChildBBox.pMax[axis] <= rightChildBBox.pMin[axis] + DEFAULT_EPSILON_STATIC);
	assert (leftChildReferences + rightChildReferences >= primsIndexes.size());

#if !defined(NDEBUG)
	// Safety check
	for (int i = 0; i < SPATIAL_SPLIT_BINS - 1; ++i) {
		u_int leftRef = 0;
		u_int rightRef = 0;
		const float pos = binsBbox[i].pMax[axis];

		for (u_int j = 0; j < primsIndexes.size(); ++j) {
			if (primsBboxes[j].pMin[axis] <= pos) {
				++leftRef;

				assert (primsBboxes[j].Overlaps(bboxesLeft[i]));
			}

			if (primsBboxes[j].pMax[axis] > pos) {
				++rightRef;

				assert (primsBboxes[j].Overlaps(bboxesRight[i + 1]));
			}
		}

		assert (leftRef == nbPrimsLeft[i]);
		assert (rightRef == nbPrimsRight[i + 1]);
	}
#endif

	//--------------------------------------------------------------------------
	// Reference unsplitting
	//--------------------------------------------------------------------------

	/*const float spatialSplitCost = leftChildBBox.SurfaceArea() * QuadCount(spatialLeftChildReferences) +
				rightChildBBox.SurfaceArea() * QuadCount(spatialRightChildReferences);

	for (u_int i = 0; i < primsIndexes.size(); ++i) {
		const bool overlapLeft = (primsBboxes[i].pMin[axis] <= splitPos);
		const bool overlapRight = (primsBboxes[i].pMax[axis] > splitPos);

		if (overlapLeft && overlapRight) {
			// Check if it is worth applying reference unsplitting

			assert (spatialLeftChildReferences > 0);
			assert (spatialRightChildReferences > 0);

			BBox extendedLeftChildBBox = Union(leftChildBBox, primsBboxes[i]);
			const float c1 = extendedLeftChildBBox.SurfaceArea() * QuadCount(spatialLeftChildReferences) +
				rightChildBBox.SurfaceArea() * QuadCount(spatialRightChildReferences - 1);

			BBox extendedRightChildBBox = Union(rightChildBBox, primsBboxes[i]);
			const float c2 = leftChildBBox.SurfaceArea() * QuadCount(spatialLeftChildReferences - 1) +
				extendedRightChildBBox.SurfaceArea() * QuadCount(spatialRightChildReferences);

			if (c1 < spatialSplitCost) {
				// Put the primitive only in left child
				leftChildBBox = extendedLeftChildBBox;
			}

			if (c2 < spatialSplitCost) {
				// Put the primitive only in right child
				rightChildBBox = extendedRightChildBBox;
			}
		}
	}*/

	return splitPos;
}

float SQBVHAccel::BuildObjectSplit(const std::vector<BBox> &primsBboxes, int &axis,
		BBox &leftChildBBox, BBox &rightChildBBox,
		u_int &leftChildReferences, u_int &rightChildReferences) {
	// Build the centroids list and bounding box
	BBox centroidsBbox;
	std::vector<Point> primsCentroids(primsBboxes.size());
	for (u_int i = 0; i < primsBboxes.size(); ++i) {
		const Point center = primsBboxes[i].Center();
		centroidsBbox = Union(centroidsBbox, center);
		primsCentroids[i] = center;
	}

	// Choose the split axis, taking the axis of maximum extent for the
	// centroids (else weird cases can occur, where the maximum extent axis
	// for the nodeBbox is an axis of 0 extent for the centroids one.).
	axis = centroidsBbox.MaximumExtent();

	// Precompute values that are constant with respect to the current
	// primitive considered.
	const float k0 = centroidsBbox.pMin[axis];
	const float k1 = OBJECT_SPLIT_BINS / (centroidsBbox.pMax[axis] - k0);
	
	// If the bbox is a point
	if (isinf(k1))
		return std::numeric_limits<float>::quiet_NaN();

	// Number of primitives in each bin
	int bins[OBJECT_SPLIT_BINS];
	// Bbox of the primitives in the bin
	BBox binsBbox[OBJECT_SPLIT_BINS];

	//--------------
	// Fill in the bins, considering all the primitives when a given
	// threshold is reached, else considering only a portion of the
	// primitives for the binned-SAH process. Also compute the bins bboxes
	// for the primitives. 

	for (int i = 0; i < OBJECT_SPLIT_BINS; ++i)
		bins[i] = 0.f;

	u_int step = (primsBboxes.size() < fullSweepThreshold) ? 1 : skipFactor;

	for (u_int i = 0; i < primsBboxes.size(); i += step) {
		// Binning is relative to the centroids bbox and to the
		// primitives' centroid.
		const int binId = max(0, min(OBJECT_SPLIT_BINS - 1,
				Floor2Int(k1 * (primsCentroids[i][axis] - k0))));
		bins[binId]++;
		binsBbox[binId] = Union(binsBbox[binId], primsBboxes[i]);
	}

	//--------------
	// Evaluate where to split.

	// Cumulative number of primitives in the bins from the first to the
	// ith, and from the last to the ith.
	int nbPrimsLeft[OBJECT_SPLIT_BINS];
	int nbPrimsRight[OBJECT_SPLIT_BINS];
	// The corresponding cumulative bounding boxes.
	BBox bboxesLeft[OBJECT_SPLIT_BINS];
	BBox bboxesRight[OBJECT_SPLIT_BINS];

	// The corresponding SAHs
	float areaLeft[OBJECT_SPLIT_BINS];
	float areaRight[OBJECT_SPLIT_BINS];	

	BBox currentBboxLeft, currentBboxRight;
	int currentNbLeft = 0, currentNbRight = 0;

	for (int i = 0; i < OBJECT_SPLIT_BINS; ++i) {
		//-----
		// Left side
		// Number of prims
		currentNbLeft += bins[i];
		nbPrimsLeft[i] = currentNbLeft;
		// Prims bbox
		currentBboxLeft = Union(currentBboxLeft, binsBbox[i]);
		bboxesLeft[i] = currentBboxLeft;
		// Surface area
		areaLeft[i] = currentBboxLeft.SurfaceArea();
		
		//-----
		// Right side
		// Number of prims
		const int rightIndex = OBJECT_SPLIT_BINS - 1 - i;
		currentNbRight += bins[rightIndex];
		nbPrimsRight[rightIndex] = currentNbRight;
		// Prims bbox
		currentBboxRight = Union(currentBboxRight, binsBbox[rightIndex]);
		bboxesRight[rightIndex] = currentBboxRight;
		// Surface area
		areaRight[rightIndex] = currentBboxRight.SurfaceArea();
	}

	int minBin = -1;
	float minCost = INFINITY;
	// Find the best split axis,
	// there must be at least a bin on the right side
	for (int i = 0; i < OBJECT_SPLIT_BINS - 1; ++i) {
		float cost = areaLeft[i] * QuadCount(nbPrimsLeft[i]) +
			areaRight[i + 1] * QuadCount(nbPrimsRight[i + 1]);
		if (cost < minCost) {
			minBin = i;
			minCost = cost;
		}
	}

	leftChildBBox = bboxesLeft[minBin];
	rightChildBBox = bboxesRight[minBin + 1];
	leftChildReferences = nbPrimsLeft[minBin];
	rightChildReferences = nbPrimsRight[minBin + 1];

	assert ((leftChildReferences == 0) || leftChildBbox.IsValid());
	assert ((rghtChildReferences == 0) || rightChildBbox.IsValid());

	//-----------------
	// Make the partition, in a "quicksort partitioning" way,
	// the pivot being the position of the split plane
	// (no more binId computation)
	// track also the bboxes (primitives and centroids)
	// for the left and right halves.

	// The split plane coordinate is the coordinate of the end of
	// the chosen bin along the split axis
	return centroidsBbox.pMin[axis] + (minBin + 1) *
		(centroidsBbox.pMax[axis] - centroidsBbox.pMin[axis]) / OBJECT_SPLIT_BINS;
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
