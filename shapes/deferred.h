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

#include <boost/thread/mutex.hpp>

#include "shape.h"

namespace lux
{

class DeferredLoadShape: public Shape {
public:
	DeferredLoadShape(const Transform &o2w, bool ro, const string &nm,
			const BBox &bbox, const ParamSet &ps);
	virtual ~DeferredLoadShape();

	virtual BBox ObjectBound() const {
		return shapeBBox;
	}

	virtual void GetShadingInformation(const DifferentialGeometry &dgShading, RGBColor *color, float *alpha) const{
		LoadShape();
		return shape->GetShadingInformation(dgShading, color, alpha);
	}

	virtual float Area() const {
		LoadShape();
		return shape->Area();
	}

	virtual float Pdf(const PartialDifferentialGeometry &dg) const {
		LoadShape();
		return shape->Pdf(dg);
	}
	virtual float Pdf(const Point &p, const PartialDifferentialGeometry &dg) const {
		LoadShape();
		return shape->Pdf(p, dg);
	}

	virtual void Tessellate(vector<luxrays::TriangleMesh *> *meshList,
		vector<const Primitive *> *primitiveList) const {
		LoadShape();
		shape->Tessellate(meshList, primitiveList);
	}
	virtual void ExtTessellate(vector<luxrays::ExtTriangleMesh *> *meshList,
		vector<const Primitive *> *primitiveList) const {
		LoadShape();
		shape->ExtTessellate(meshList,primitiveList);
	}
	virtual void GetIntersection(const luxrays::RayHit &rayHit, const u_int index, Intersection *in) const {
		LoadShape();
		shape->GetIntersection(rayHit, index, in);
	}

	virtual bool CanIntersect() const { return true; }
	virtual bool Intersect(const Ray &r, Intersection *isect) const {
		LoadShape();
		return prim->Intersect(r, isect);
	}
	virtual bool IntersectP(const Ray &r) const {
		LoadShape();
		return prim->IntersectP(r);
	}

	virtual void GetShadingGeometry(const Transform &obj2world,
		const DifferentialGeometry &dg,
		DifferentialGeometry *dgShading) const {
		LoadShape();
		return shape->GetShadingGeometry(obj2world, dg, dgShading);
	}

	virtual bool CanSample() const { return true; }
	virtual float Sample(float u1, float u2, float u3,
		DifferentialGeometry *dg) const {
		LoadShape();
		return shape->Sample(u1, u2, u3, dg);
	}
	virtual float Sample(const Point &p, float u1, float u2, float u3,
		DifferentialGeometry *dg) const {
		LoadShape();
		return shape->Sample(p, u1, u2, u3, dg);
	}

	static Shape *CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);

private:
	void LoadShape() const;

	// The shape bounding box express din local coordinate
	BBox shapeBBox;

	mutable boost::mutex loadMutex;

	// A copy of parsing parameters
	mutable ParamSet *params;

	// The deferred loaded shape
	mutable boost::shared_ptr<Shape> shape;
	mutable boost::shared_ptr<Aggregate> accelerator;
	mutable Primitive *prim;
};

}//namespace lux
