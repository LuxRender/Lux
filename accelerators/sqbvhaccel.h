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

// The number of bins for spatial split
#define SPATIAL_SPLIT_BINS 64

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
	*/
	void BuildTree(const std::vector<u_int> &primsIndexes,
			const vector<boost::shared_ptr<Primitive> > &vPrims,
			const std::vector<BBox> &primsBboxes, const BBox &nodeBbox,
			const int32_t parentIndex, const int32_t childIndex,
			const int depth);

	float BuildSpatialSplit(const std::vector<u_int> &primsIndexes,
		const vector<boost::shared_ptr<Primitive> > &vPrims,
		const std::vector<BBox> &primsBboxes, const BBox &nodeBbox,
		int &axis, BBox &leftChildBBox, BBox &rightChildBBox,
		u_int &leftChildReferences, u_int &rightChildReferences);
	float BuildObjectSplit(const std::vector<BBox> &primsBboxes, int &axis,
		BBox &leftChildBBox, BBox &rightChildBBox,
		u_int &leftChildReferences, u_int &rightChildReferences);

	void DoObjectSplit(const std::vector<u_int> &primsIndexes, const std::vector<BBox> &primsBboxes,
		const float objectSplitPos, const int objectSplitAxis,
		const u_int objectLeftChildReferences, const u_int objectRightChildReferences,
		std::vector<u_int> &leftPrimsIndexes, std::vector<u_int> &rightPrimsIndexes,
		std::vector<BBox> &leftPrimsBbox, std::vector<BBox> &rightPrimsBbox);
	void DoSpatialSplit(const std::vector<u_int> &primsIndexes,
		const vector<boost::shared_ptr<Primitive> > &vPrims, const std::vector<BBox> &primsBboxes,
		const float spatialSplitPos, const int spatialSplitAxis,
		const u_int spatialLeftChildReferences, const u_int spatialRightChildReferences,
		std::vector<u_int> &leftPrimsIndexes, std::vector<u_int> &rightPrimsIndexes,
		std::vector<BBox> &leftPrimsBbox, std::vector<BBox> &rightPrimsBbox,
		BBox &spatialLeftChildBbox, BBox &spatialRightChildBbox);

	bool DoesSupportPolygonVertexList(const Primitive *prim) const;
	vector<Point> GetPolygonVertexList(const Primitive *prim) const;
	
	vector<vector<u_int> > nodesPrims[4]; // Temporary data for building
	float alpha;

	// Some statistics about the quality of the built accelerator
	u_int objectSplitCount, spatialSplitCount;
};

} // namespace lux
#endif //LUX_SQBVHACCEL_H
