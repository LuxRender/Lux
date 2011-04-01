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

// bvhaccel.h*

#include "lux.h"
#include "primitive.h"

namespace lux
{

struct BVHAccelTreeNode {
	BBox bbox;
	Primitive* primitive;
	boost::shared_ptr<BVHAccelTreeNode> leftChild;
	boost::shared_ptr<BVHAccelTreeNode> rightSibling;
};

struct BVHAccelArrayNode {
	BBox bbox;
	Primitive* primitive;
	u_int skipIndex;
};

// BVHAccel Declarations
class  BVHAccel : public Aggregate {
public:
	// BVHAccel Public Methods
	BVHAccel(const vector<boost::shared_ptr<Primitive> > &p, u_int treetype,
		int csamples, int icost, int tcost, float ebonus);
	virtual ~BVHAccel();
	virtual BBox WorldBound() const;
	virtual bool CanIntersect() const { return true; }
	virtual bool Intersect(const Ray &ray, Intersection *isect, bool null_shp_isect=false) const;
	virtual bool IntersectP(const Ray &ray, bool null_shp_isect=false) const;

	virtual void GetPrimitives(vector<boost::shared_ptr<Primitive> > &prims);

	static Aggregate *CreateAccelerator(const vector<boost::shared_ptr<Primitive> > &prims, const ParamSet &ps);

private:
	// BVHAccel Private Methods
	boost::shared_ptr<BVHAccelTreeNode> BuildHierarchy(vector<boost::shared_ptr<BVHAccelTreeNode> > &list, u_int begin, u_int end, u_int axis);
	void FindBestSplit(vector<boost::shared_ptr<BVHAccelTreeNode> > &list, u_int begin, u_int end, float *splitValue, u_int *bestAxis);
	u_int BuildArray(boost::shared_ptr<BVHAccelTreeNode> &node, u_int offset);

	// BVHAccel Private Data
	u_int treeType;
	int costSamples, isectCost, traversalCost;
	float emptyBonus;
	u_int nPrims;
	boost::shared_ptr<Primitive> *prims;
	u_int nNodes;
	BVHAccelArrayNode *bvhTree;
};

}//namespace lux

