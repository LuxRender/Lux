/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

#ifndef LUX_SHAPE_H
#define LUX_SHAPE_H
// shape.h*
#include "lux.h"
#include "primitive.h"
#include "error.h"

namespace lux
{

// Lotus - A primitive implementation compatible with the old PBRT Shape class
// Shape Declarations
class Shape : public Primitive {
public:
	Shape(const Transform &o2w, bool ro);
	Shape(const Transform &o2w, bool ro,
			boost::shared_ptr<Material> material);
	virtual ~Shape() { }

	void SetMaterial(boost::shared_ptr<Material> mat) { this->material = mat; }
	boost::shared_ptr<Material> GetMaterial() const { return material; }

	virtual BBox WorldBound() const { return ObjectToWorld(ObjectBound()); }
	virtual void Refine(vector<boost::shared_ptr<Primitive> > &refined,
			const PrimitiveRefinementHints& refineHints,
			boost::shared_ptr<Primitive> thisPtr)
	{
		vector<boost::shared_ptr<Shape> > todo;
		Refine(todo); // Use shape refine method
		for(u_int i=0; i<todo.size(); i++) {
			boost::shared_ptr<Shape> shape = todo[i];
			shape->SetMaterial(this->material);
			if (shape->CanIntersect()) {
				refined.push_back(shape);
			}
			else {
				// Use primitive refine method
				shape->Refine(refined, refineHints, shape);
			}
		}
	}

	virtual bool CanIntersect() const { return true; }
	virtual bool Intersect(const Ray &r, Intersection *isect) const {
		float thit;
		if (!Intersect(r, &thit, &isect->dg))
			return false;
		isect->dg.AdjustNormal(reverseOrientation, transformSwapsHandedness);
		isect->Set(WorldToObject, this, material.get());
		r.maxt = thit;
		return true;
	}

	virtual void GetShadingGeometry(const Transform &obj2world,
			const DifferentialGeometry &dg,
			DifferentialGeometry *dgShading) const
	{
		*dgShading = dg;
	}

	virtual bool CanSample() const { return true; }
	virtual void Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const {
		dg->p = Sample(u1, u2, u3, &dg->nn);
		CoordinateSystem(Vector(dg->nn), &dg->dpdu, &dg->dpdv);
		//TODO fill in uv coordinates
		dg->u = dg->v = .5f;
	}
	virtual void Sample(const Point &p, float u1, float u2, float u3, DifferentialGeometry *dg) const {
		dg->p = Sample(p, u1, u2, u3, &dg->nn);
		CoordinateSystem(Vector(dg->nn), &dg->dpdu, &dg->dpdv);
		//TODO fill in uv coordinates
		dg->u = dg->v = .5f;
	}

	// Old PBRT Shape interface methods
	virtual BBox ObjectBound() const {
		luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Shape::ObjectBound method called!");
		return BBox();
	}
	virtual void Refine(vector<boost::shared_ptr<Shape> > &refined) const {
		luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Shape::Refine() method called");
	}
	virtual bool Intersect(const Ray &ray, float *t_hitp,
			DifferentialGeometry *dg) const
	{
		luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Shape::Intersect() method called");
		return false;
	}
	virtual Point Sample(float u1, float u2, float u3, Normal *Ns) const {
		luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Shape::Sample() method called");
		return Point();
	}
	virtual Point Sample(const Point &p, float u1, float u2, float u3, Normal *Ns) const {
		return Sample(u1, u2, u3, Ns);
	}
	// Shape data
	const Transform ObjectToWorld, WorldToObject;
	const bool reverseOrientation, transformSwapsHandedness;
protected:
	boost::shared_ptr<Material> material;
};

class PrimitiveSet : public Primitive {
public:
	// PrimitiveSet Public Methods
	PrimitiveSet(boost::shared_ptr<Aggregate> a);
	PrimitiveSet(const vector<boost::shared_ptr<Primitive> > &p);
	virtual ~PrimitiveSet() { }

	virtual BBox WorldBound() const { return worldbound; }
	virtual bool CanIntersect() const {
		for (u_int i = 0; i < primitives.size(); ++i)
			if (!primitives[i]->CanIntersect()) return false;
		return true;
	}
	virtual bool Intersect(const Ray &r, Intersection *in) const;
	virtual bool IntersectP(const Ray &r) const;

	virtual bool CanSample() const {
		for (u_int i = 0; i < primitives.size(); ++i)
			if (!primitives[i]->CanSample()) return false;
		return true;
	}
	virtual void Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const {
		u_int sn;
		if( primitives.size() <= 16) {
			for (sn = 0; sn < primitives.size()-1; ++sn)
				if (u3 < areaCDF[sn]) break;
		}
		else {
			sn = Clamp(
				(u_int)(std::upper_bound(areaCDF.begin(), areaCDF.end(), u3) - areaCDF.begin()),
				(u_int)(0), (u_int)(primitives.size() - 1));
		}
		primitives[sn]->Sample(u1, u2, u3, dg);
	}
	virtual float Area() const { return area; }
private:
	void initAreas();

	// PrimitiveSet Private Data
	float area;
	vector<float> areaCDF;
	vector<boost::shared_ptr<Primitive> > primitives;
	BBox worldbound;
	boost::shared_ptr<Primitive> accelerator;
};

}//namespace lux

#endif // LUX_SHAPE_H
