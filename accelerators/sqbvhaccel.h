/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_SQBVHACCEL_H
#define LUX_SQBVHACCEL_H

#include "lux.h"
#include "qbvhaccel.h"

namespace lux
{

class SBVHNode {
public:
	SBVHNode() {
		// All children are empty leaves by default
		for (int i = 0; i < 4; ++i)
			childs[i] = NULL;
	}
	~SBVHNode() {
		for (int i = 0; i < 4; ++i)
			delete childs[i];
	}

	BBox childBboxes[4];
	SBVHNode *childs[4];
	std::vector<u_int> childPrims[4];
};

class SQBVHAccel : public QBVHAccel {
public:
	/**
	   Normal constructor.
	   @param p the vector of shared primitives to put in the QBVH
	   @param mp the maximum number of primitives per leaf
	   @param fst the threshold before switching to full sweep for split
	   @param sf the skip factor during split determination
	*/
	SQBVHAccel(const vector<boost::shared_ptr<Primitive> > &p, u_int mp, u_int fst, u_int sf, float a);
	virtual ~SQBVHAccel() { }

	/**
	   Read configuration parameters and create a new SQBVH accelerator
	   @param prims vector of primitives to store into the SQBVH
	   @param ps configuration parameters
	*/
	static Aggregate *CreateAccelerator(const vector<boost::shared_ptr<Primitive> > &prims, const ParamSet &ps);

private:
	/**
	   Build the tree that will contain the primitives indexed from start
	   to end in the primsIndexes array.
	   @param start
	   @param end
	   @param primsBboxes the bounding boxes for all the primitives
	   @param primsbboxes the centroids of all the primitives
	   @param nodeBbox the bounding box of the node.
	   @param centroidsBbox the bounding box of the centroids of the
	   primitives in the node.
	   @param parentIndex the index of the parent node
	   @param childIndex the index of the node in the parent node
	   (its child number)
	   @param depth the current depth.
	*/
	void BuildTree(const std::vector<u_int> &primsIndexes,
			const BBox *primsBboxes, const Point *primsCentroids,
			const BBox &nodeBbox, const BBox &centroidsBbox,
			const int32_t parentIndex, const int32_t childIndex,
			const int depth);

	float BuildSpatialSplit(u_int start, u_int end, u_int *primsIndexes,
		const BBox *primsBboxes, const Point *primsCentroids,
		const BBox &nodeBbox, const BBox &centroidsBbox,
		int &axis, BBox &leftChildBBox, BBox &rightChildBBox,
		int &spatialLeftChildReferences, int &spatialRightChildReferences);

	vector<vector<u_int> > nodesPrims[4]; // Temporary data for building
	float alpha;
};

} // namespace lux
#endif //LUX_SQBVHACCEL_H
