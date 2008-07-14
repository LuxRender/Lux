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
#include "geometry.h"
#include "error.h"

namespace lux
{

// DifferentialGeometry Declarations
class  DifferentialGeometry {
	public:

	DifferentialGeometry() { u = v = 0.; shape = NULL; }
	// DifferentialGeometry Public Methods
	DifferentialGeometry(
			const Point &P,
			const Vector &DPDU,	const Vector &DPDV,
			const Vector &DNDU, const Vector &DNDV,
			float uu, float vv,
			const Shape *sh);
	DifferentialGeometry(
			const Point &P, const Normal &NN,
			const Vector &DPDU,	const Vector &DPDV, 
			const Vector &DNDU, const Vector &DNDV, 
			float uu, float vv,
			const Shape *sh);
	void ComputeDifferentials(const RayDifferential &r) const;
	// DifferentialGeometry Public Data
	Point p;
	Normal nn;
	Vector dpdu, dpdv;
	Normal dndu, dndv;
	mutable Vector dpdx, dpdy;
	float u, v;
	const Shape *shape;
	mutable float dudx, dvdx, dudy, dvdy;

	// Dade - shape specific data, useful to "transport" informatin between
	// shape intersection method and GetShadingGeometry()
	union {
		float triangleBaryCoords[3];
	};
};

// Shape Declarations
class  Shape {
public:
	// Shape Interface
	Shape(const Transform &o2w, bool ro);
	virtual ~Shape() { }
	virtual BBox ObjectBound() const = 0;
	virtual BBox WorldBound() const {
		return ObjectToWorld(ObjectBound());
	}
	virtual bool CanIntersect() const { return true; }
	virtual void
		Refine(vector<boost::shared_ptr<Shape> > &refined) const {
		//Severe("Unimplemented Shape::Refine() method called");
		luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Shape::Refine() method called");
	}
	virtual bool Intersect(const Ray &ray, float *tHit,
			DifferentialGeometry *dg) const {
		luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Shape::Intersect() method called");
		return false;
	}
	virtual bool IntersectP(const Ray &ray) const {
		luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Shape::IntersectP() method called");
		return false;
	}
	virtual void GetShadingGeometry(const Transform &obj2world,
			const DifferentialGeometry &dg,
			DifferentialGeometry *dgShading) const {
		*dgShading = dg;
	}
	virtual float Area() const {
		luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Shape::Area() method called");
		return 0.;
	}
	virtual Point Sample(float u1, float u2, Normal *Ns) const {
		luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Shape::Sample method called");
		return Point();
	}
	virtual float Pdf(const Point &Pshape) const {
		return 1.f / Area();
	}
	virtual Point Sample(const Point &P,
			float u1, float u2, Normal *Ns) const {
		return Sample(u1, u2, Ns);
	}
	virtual float Pdf(const Point &p, const Vector &wi) const {
		// Intersect sample ray with area light geometry
		DifferentialGeometry dgLight;
		Ray ray(p, wi);
		float thit;
		if (!Intersect(ray, &thit, &dgLight)) return 0.;
		// Convert light sample weight to solid angle measure
		float pdf = DistanceSquared(p, ray(thit)) /
			(AbsDot(dgLight.nn, -wi) * Area());
		if (AbsDot(dgLight.nn, -wi) == 0.f) pdf = INFINITY;
		return pdf;
	}
	// Shape Public Data
	const Transform ObjectToWorld, WorldToObject;
	const bool reverseOrientation, transformSwapsHandedness;
};

class ShapeSet : public Shape {
public:
	// ShapeSet Public Methods
	Point Sample(float u1, float u2, Normal *Ns) const {
		float ls = lux::random::floatValue();
		u_int sn;
		for (sn = 0; sn < shapes.size()-1; ++sn)
			if (ls < areaCDF[sn]) break;
		return shapes[sn]->Sample(u1, u2, Ns);
	}
	ShapeSet(const vector<boost::shared_ptr<Shape> > &s,
		const Transform &o2w, bool ro);
	BBox ObjectBound() const {
		BBox ob;
		for (u_int i = 0; i < shapes.size(); ++i)
			ob = Union(ob, shapes[i]->ObjectBound());
		return ob;
	}
	bool CanIntersect() const {
		for (u_int i = 0; i < shapes.size(); ++i)
			if (!shapes[i]->CanIntersect()) return false;
		return true;
	}
	bool Intersect(const Ray &ray, float *t_hitp,
			DifferentialGeometry *dg) const;
	void Refine(vector<boost::shared_ptr<Shape> > &refined) const {
		for (u_int i = 0; i < shapes.size(); ++i) {
			if (shapes[i]->CanIntersect())
				refined.push_back(shapes[i]);
			else shapes[i]->Refine(refined);
		}

	}
	float Area() const { return area; }
private:
	// ShapeSet Private Data
	float area;
	vector<float> areaCDF;
	vector<boost::shared_ptr<Shape> > shapes;
	BBox worldbound;
	Primitive *accelerator;
};

}//namespace lux

#endif // LUX_SHAPE_H
