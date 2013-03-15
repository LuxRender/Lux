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

#ifndef LUX_SHAPE_H
#define LUX_SHAPE_H
// shape.h*
#include "lux.h"
#include "primitive.h"
#include "error.h"

namespace lux
{

#define SHAPE_LOG(name,severity,code) LOG(severity,code)<<"Shape "<<(name)<<": "

// Lotus - A primitive implementation compatible with the old PBRT Shape class
// Shape Declarations
class Shape : public Primitive {
public:
//	enum ShapeType { LUX_SHAPE, AR_SHAPE, ENV_SHAPE };
	Shape(const Transform &o2w, bool ro, const string &name);
	Shape(const Transform &o2w, bool ro,
		boost::shared_ptr<Material> &material,
		boost::shared_ptr<Volume> &ex,
		boost::shared_ptr<Volume> &in, const string &name);
	virtual ~Shape() { }

	void SetMaterial(boost::shared_ptr<Material> &mat) {
		// Create a temporary to increase shared count
		// The assignment is just a swap
		boost::shared_ptr<Material> m(mat);
		material = mat;
	}
	void SetExterior(boost::shared_ptr<Volume> &vol) {
		// Create a temporary to increase shared count
		// The assignment is just a swap
		boost::shared_ptr<Volume> v(vol);
		exterior = v;
	}
	void SetInterior(boost::shared_ptr<Volume> &vol) {
		// Create a temporary to increase shared count
		// The assignment is just a swap
		boost::shared_ptr<Volume> v(vol);
		interior = v;
	}
	Material *GetMaterial() const { return material.get(); }
	virtual const Volume *GetExterior() const { return exterior.get(); }
	virtual const Volume *GetInterior() const { return interior.get(); }

	virtual BBox WorldBound() const { return ObjectToWorld * ObjectBound(); }
	virtual void Refine(vector<boost::shared_ptr<Primitive> > &refined,
		const PrimitiveRefinementHints& refineHints,
		const boost::shared_ptr<Primitive> &thisPtr) {
		vector<boost::shared_ptr<Shape> > todo;
		Refine(todo); // Use shape refine method
		for (u_int i = 0; i < todo.size(); ++i) {
			boost::shared_ptr<Shape> &shape(todo[i]);
			shape->SetMaterial(material);
			shape->SetExterior(exterior);
			shape->SetInterior(interior);
			if (shape->CanIntersect()) {
				refined.push_back(shape);
			} else {
				// Use primitive refine method
				shape->Refine(refined, refineHints, shape);
			}
		}
	}

	virtual ShapeType GetPrimitiveType() const { return ShapeType(LUX_SHAPE); }
	virtual Vector GetNormal(u_int i) const { return Vector(0.f); }
	virtual Point GetPoint(u_int i) const { return Point(0.f); }
	virtual float GetScale(u_int i) const { return 1.f; }
	virtual bool SetScale(float scale, u_int i) const { return false; }
	virtual bool CanIntersect() const { return true; }
	virtual bool Intersect(const Ray &r, Intersection *isect, bool null_shp_isect = false ) const {
		float thit;
		if (!Intersect(r, &thit, &isect->dg, null_shp_isect))
			return false;
		isect->dg.AdjustNormal(reverseOrientation,
			transformSwapsHandedness);
		isect->Set(ObjectToWorld, this, material.get(), exterior.get(),
			interior.get());
		r.maxt = thit;
		return true;
	}

	virtual void GetShadingGeometry(const Transform &obj2world,
		const DifferentialGeometry &dg,
		DifferentialGeometry *dgShading) const { *dgShading = dg; }

	virtual bool CanSample() const { return true; }
	virtual float Sample(float u1, float u2, float u3,
		DifferentialGeometry *dg) const {
		dg->p = Sample(u1, u2, u3, &dg->nn);
		CoordinateSystem(Vector(dg->nn), &dg->dpdu, &dg->dpdv);
		dg->dndu = dg->dndv = Normal(0, 0, 0);
		//TODO fill in uv coordinates
		dg->u = dg->v = .5f;
		dg->handle = this;
		return Pdf(*dg);
	}
	virtual float Sample(const Point &p, float u1, float u2, float u3,
		DifferentialGeometry *dg) const {
		dg->p = Sample(p, u1, u2, u3, &dg->nn);
		CoordinateSystem(Vector(dg->nn), &dg->dpdu, &dg->dpdv);
		dg->dndu = dg->dndv = Normal(0, 0, 0);
		//TODO fill in uv coordinates
		dg->u = dg->v = .5f;
		dg->handle = this;
		return Pdf(*dg);
	}

	// Old PBRT Shape interface methods
	virtual BBox ObjectBound() const {
		LOG( LUX_SEVERE,LUX_BUG)<<"Unimplemented Shape::ObjectBound method called!";
		return BBox();
	}
	virtual void Refine(vector<boost::shared_ptr<Shape> > &refined) const {
		LOG(LUX_SEVERE,LUX_BUG)<<"Unimplemented Shape::Refine() method called";
	}
	virtual bool Intersect(const Ray &ray, float *t_hitp,
		DifferentialGeometry *dg, bool null_shp_isect = false ) const {
		LOG(LUX_SEVERE,LUX_BUG)<<"Unimplemented Shape::Intersect() method called";
		return false;
	}
	virtual Point Sample(float u1, float u2, float u3, Normal *Ns) const {
		LOG(LUX_SEVERE,LUX_BUG)<<"Unimplemented Shape::Sample() method called";
		return Point();
	}
	virtual Point Sample(const Point &p, float u1, float u2, float u3,
		Normal *Ns) const { return Sample(u1, u2, u3, Ns); }
	virtual Transform GetLocalToWorld(float time) const {
		return ObjectToWorld;
	}
	// Shape data
	const Transform ObjectToWorld;
protected:
	boost::shared_ptr<Material> material;
	boost::shared_ptr<Volume> exterior, interior;
	const string name;
public: // Last to get better data alignment
	const bool reverseOrientation, transformSwapsHandedness;
};

class PrimitiveSet : public Primitive {
public:
	// PrimitiveSet Public Methods
	PrimitiveSet(boost::shared_ptr<Aggregate> &a);
	PrimitiveSet(const vector<boost::shared_ptr<Primitive> > &p);
	virtual ~PrimitiveSet() { }

	virtual BBox WorldBound() const { return worldbound; }
	virtual ShapeType GetPrimitiveType() const { return ShapeType(LUX_SHAPE); }
	virtual bool CanIntersect() const {
		for (u_int i = 0; i < primitives.size(); ++i)
			if (!primitives[i]->CanIntersect()) return false;
		return true;
	}
	virtual bool Intersect(const Ray &r, Intersection *in, bool null_shp_isect = false ) const;
	virtual bool IntersectP(const Ray &r, bool null_shp_isect = false ) const;

	virtual bool CanSample() const {
		for (u_int i = 0; i < primitives.size(); ++i)
			if (!primitives[i]->CanSample()) return false;
		return true;
	}
	virtual float Sample(float u1, float u2, float u3,
		DifferentialGeometry *dg) const {
		size_t sn;
		if (primitives.size() <= 16) {
			for (sn = 0; sn < primitives.size() - 1; ++sn)
				if (u3 < areaCDF[sn]) break;
		} else {
			sn = Clamp<size_t>(static_cast<size_t>(std::upper_bound(areaCDF.begin(),
				areaCDF.end(), u3) - areaCDF.begin()),
				0U, primitives.size() - 1U);
		}
		const float pdf = primitives[sn]->Sample(u1, u2, u3, dg);
		return (sn == 0 ? areaCDF[sn] : areaCDF[sn] - areaCDF[sn - 1]) *
			pdf;
	}
	virtual float Area() const { return area; }
	virtual Transform GetLocalToWorld(float time) const {
		return Transform();
	}
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
