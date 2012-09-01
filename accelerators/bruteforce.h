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

// bruteforce.cpp*
#include "lux.h"
#include "primitive.h"

namespace lux
{

// BruteForceAccel Declarations
class  BruteForceAccel : public Aggregate {
public:
	// BruteForceAccel Public Methods
	BruteForceAccel(const vector<boost::shared_ptr<Primitive> > &p);
	virtual ~BruteForceAccel();
	virtual BBox WorldBound() const;
	virtual bool CanIntersect() const { return true; }
	virtual bool Intersect(const Ray &ray, Intersection *isect) const;
	virtual bool IntersectP(const Ray &ray) const;
	virtual Transform GetWorldToLocal(float time) const {
		return Transform();
	}

	/**
	 * Add a tesselated approximation of current primitive to list passed as
	 * argument. It can do nothing in case tasselation is not supported.
	 * @param meshList      The vector where the mesh.
	 * @param primitiveList The vector of primitive pointers where to add each a pointer to each primitive tesselated in the corresponding mesh.
	 */
	virtual void Tesselate(vector<luxrays::TriangleMesh *> *meshList,
		vector<const Primitive *> *primitiveList) const;

	virtual void GetPrimitives(vector<boost::shared_ptr<Primitive> > &prims) const;

	static Aggregate *CreateAccelerator(const vector<boost::shared_ptr<Primitive> > &prims, const ParamSet &ps);

private:
	// BruteForceAccel Private Data
	vector<boost::shared_ptr<Primitive> > prims;
	BBox bounds;
};

}//namespace lux

