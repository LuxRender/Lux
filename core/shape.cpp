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

// shape.cpp*
#include "shape.h"
#include "primitive.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Shape Method Definitions
Shape::Shape(const Transform &o2w, bool ro)
	: ObjectToWorld(o2w), WorldToObject(o2w.GetInverse()),
	reverseOrientation(ro),
	transformSwapsHandedness(o2w.SwapsHandedness()) {
	// Update shape creation statistics
	// radiance - disabled for threading // static StatsCounter nShapesMade("Geometry","Total shapes created");
	// radiance - disabled for threading // ++nShapesMade;
}
// ShapeSet Method Definitions
ShapeSet::ShapeSet(const vector<boost::shared_ptr<Shape> > &s,
	const Transform &o2w, bool ro)
	: Shape(o2w, ro) {
	shapes = s;
	area = 0;
	vector<float> areas;
	for (u_int i = 0; i < shapes.size(); ++i) {
		float a = shapes[i]->Area();
		area += a;
		areas.push_back(a);
	}
	float prevCDF = 0;
	for (u_int i = 0; i < shapes.size(); ++i) {
		areaCDF.push_back(prevCDF + areas[i] / area);
		prevCDF = areaCDF[i];
	}

	// NOTE - ratow - Use accelerator for complex lights.
	if(shapes.size() <= 16) {
		accelerator = NULL;
		worldbound = WorldBound();
		// NOTE - ratow - Correctly expands bounds when pMin is not negative or pMax is not positive.
		worldbound.pMin -= (worldbound.pMax-worldbound.pMin)*0.01f;
		worldbound.pMax += (worldbound.pMax-worldbound.pMin)*0.01f;
	} else {
		vector<Primitive*> primitives;
		for (u_int i = 0; i < shapes.size(); ++i) {
			boost::shared_ptr<Material> emptyMtl;
			Primitive* prim (new GeometricPrimitive(shapes[i], emptyMtl, NULL));
			primitives.push_back(prim);
		}
		accelerator = MakeAccelerator("kdtree", primitives, ParamSet());
		if (!accelerator)
			luxError(LUX_BUG,LUX_SEVERE,"Unable to find \"kdtree\" accelerator");
	}
}
bool ShapeSet::Intersect(const Ray &ray, float *t_hitp,
		DifferentialGeometry *dg) const {
	Ray bray = ray;
	if(accelerator) {
		Intersection isect;
		if (!accelerator->Intersect(bray, &isect))
			return false;

		*t_hitp = bray.maxt;
		*dg = isect.dg;
		return true;
	} else if(worldbound.IntersectP(bray)) {
		// NOTE - ratow - Testing each shape for intersections again because the _ShapeSet_ can be non-planar.
		// _t_hitp_ and _dg_ are now set according to the nearest intersection.
		bool anyHit = false;
		for (u_int i = 0; i < shapes.size(); ++i) {
			if (shapes[i]->Intersect(bray, t_hitp, dg)) {
				bray.maxt = *t_hitp;
				anyHit = true;
			}
		}
		return anyHit;
	} else {
		return false;
	}
}
// DifferentialGeometry Method Definitions
DifferentialGeometry::DifferentialGeometry(const Point &P,
		const Vector &DPDU, const Vector &DPDV,
		const Vector &DNDU, const Vector &DNDV,
		float uu, float vv, const Shape *sh)
	: p(P), dpdu(DPDU), dpdv(DPDV), dndu((Normal)DNDU), dndv((Normal)DNDV) {
	// Initialize _DifferentialGeometry_ from parameters
	nn = Normal(Normalize(Cross(dpdu, dpdv)));
	u = uu;
	v = vv;
	shape = sh;
	dudx = dvdx = dudy = dvdy = 0;
	// Adjust normal based on orientation and handedness
	if (shape->reverseOrientation ^ shape->transformSwapsHandedness)
		nn *= -1.f;
}
void DifferentialGeometry::ComputeDifferentials(
		const RayDifferential &ray) const {
	if (ray.hasDifferentials) {
		// Estimate screen-space change in \pt and $(u,v)$
		// Compute auxiliary intersection points with plane
		float d = -Dot(nn, Vector(p.x, p.y, p.z));
		Vector rxv(ray.rx.o.x, ray.rx.o.y, ray.rx.o.z);
		float tx = -(Dot(nn, rxv) + d) / Dot(nn, ray.rx.d);
		Point px = ray.rx.o + tx * ray.rx.d;
		Vector ryv(ray.ry.o.x, ray.ry.o.y, ray.ry.o.z);
		float ty = -(Dot(nn, ryv) + d) / Dot(nn, ray.ry.d);
		Point py = ray.ry.o + ty * ray.ry.d;
		dpdx = px - p;
		dpdy = py - p;
		// Compute $(u,v)$ offsets at auxiliary points
		// Initialize _A_, _Bx_, and _By_ matrices for offset computation
		float A[2][2], Bx[2], By[2], x[2];
		int axes[2];
		if (fabsf(nn.x) > fabsf(nn.y) && fabsf(nn.x) > fabsf(nn.z)) {
			axes[0] = 1; axes[1] = 2;
		}
		else if (fabsf(nn.y) > fabsf(nn.z)) {
			axes[0] = 0; axes[1] = 2;
		}
		else {
			axes[0] = 0; axes[1] = 1;
		}
		// Initialize matrices for chosen projection plane
		A[0][0] = dpdu[axes[0]];
		A[0][1] = dpdv[axes[0]];
		A[1][0] = dpdu[axes[1]];
		A[1][1] = dpdv[axes[1]];
		Bx[0] = px[axes[0]] - p[axes[0]];
		Bx[1] = px[axes[1]] - p[axes[1]];
		By[0] = py[axes[0]] - p[axes[0]];
		By[1] = py[axes[1]] - p[axes[1]];
		if (SolveLinearSystem2x2(A, Bx, x)) {
			dudx = x[0]; dvdx = x[1];
		}
		else  {
			dudx = 1.; dvdx = 0.;
		}
		if (SolveLinearSystem2x2(A, By, x)) {
			dudy = x[0]; dvdy = x[1];
		}
		else {
			dudy = 0.; dvdy = 1.;
		}
	}
	else {
		dudx = dvdx = 0.;
		dudy = dvdy = 0.;
		dpdx = dpdy = Vector(0,0,0);
	}
}
