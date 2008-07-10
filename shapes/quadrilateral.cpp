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

// quadrilateral.cpp*
#include "quadrilateral.h"
#include "paramset.h"

using namespace lux;

BBox QuadMesh::ObjectBound() const {
	luxError(LUX_BUG,LUX_SEVERE, "Unimplemented QuadMesh::ObjectBound() method called");
	return BBox();
}


// Quadrilateral Method Definitions
Quadrilateral::Quadrilateral(const lux::Transform &o2w, bool ro, 
	QuadMesh *m, int *indices) : Shape(o2w, ro) {

	idx = indices;
	mesh = m;
}

Quadrilateral::~Quadrilateral() {
}

BBox Quadrilateral::ObjectBound() const {
    // Get quadrilateral vertices in _p0_, _p1_, _p2_, and _p3_
    const Point &p0 = mesh->p[idx[0]];
    const Point &p1 = mesh->p[idx[1]];
    const Point &p2 = mesh->p[idx[2]];
    const Point &p3 = mesh->p[idx[3]];
    return Union(BBox(WorldToObject(p0), WorldToObject(p1)),
            BBox(WorldToObject(p2), WorldToObject(p3)));
}

BBox Quadrilateral::WorldBound() const {
    // Get quadrilateral vertices in _p0_, _p1_, _p2_, and _p3_
    const Point &p0 = mesh->p[idx[0]];
    const Point &p1 = mesh->p[idx[1]];
    const Point &p2 = mesh->p[idx[2]];
    const Point &p3 = mesh->p[idx[3]];
    return Union(BBox(p0, p1), BBox(p2, p3));
}

bool Quadrilateral::Intersect(const Ray &ray, float *tHit,
		DifferentialGeometry *dg) const {

	// Compute intersection for quadrilateral
	// based on "An Efficient Ray-Quadrilateral Intersection Test"
	// by Ares Lagae and Philip Dutré

    // Get quadrilateral vertices in _p00_, _p10_, _p11_, and _p01_
    const Point &p00 = mesh->p[idx[0]];
    const Point &p10 = mesh->p[idx[1]];
    const Point &p11 = mesh->p[idx[2]];
    const Point &p01 = mesh->p[idx[3]];

	// bilinear coordinates
	float u, v;

	// Reject rays using the barycentric coordinates of
	// the intersection point with respect to T.
	Vector e01 = p10 - p00;
	Vector e03 = p01 - p00;
	Vector P = Cross(ray.d, e03);
	float det = Dot(e01, P);
	if (fabsf(det) < 1e-7) 
		return false;
	Vector T = ray.o - p00;
	float alpha = Dot(T, P) / det;	
	if (alpha < 0) 
		return false;
	if (alpha > 1) 
		return false;

	Vector Q = Cross(T, e01);
	float beta = Dot(ray.d, Q) / det;
	if (beta < 0) 
		return false;
	if (beta > 1)
		return false;

	// Reject rays using the barycentric coordinates of
	// the intersection point with respect to T'.
	if ((alpha + beta) > 1.f) {
		Vector e23 = p01 - p11;
		Vector e21 = p10 - p11;
		Vector P2 = Cross(ray.d, e21);
		float det2 = Dot(e23, P2);
		if (fabsf(det2) < 1e-7f) 
			return false;
		Vector T2 = ray.o - p11;
		float alpha2 = Dot(T2, P2) / det2;
		if (alpha2 < 0) 
			return false;
		Vector Q2 = Cross(T2, e23);
		float beta2 = Dot(ray.d, Q2) / det2;
		if (beta2 < 0) 
			return false;
	}

	// Compute the ray parameter of the intersection
	// point.
	float t = Dot(e03, Q) / det;
	if (t < ray.mint || t > ray.maxt) 
		return false;

	// Compute the barycentric coordinates of V11.
	Vector e02 = p11 - p00;
	Normal N = Normal(Cross(e01, e03));

	float a11, b11;
	float absNx = fabsf(N.x);
	float absNy = fabsf(N.y);
	float absNz = fabsf(N.z);

	if (absNx >= absNy && absNx >= absNz) {
		float iNx = 1.f / N.x;
		a11 = (e02.y * e03.z - e02.z * e03.y) * iNx;
		b11 = (e01.y * e02.z - e01.z * e02.y) * iNx;
	} else if (absNy >= absNx && absNy >= absNz) {
		float iNy = 1.f / N.y;
		a11 = (e02.z * e03.x - e02.x * e03.z) * iNy;
		b11 = (e01.z * e02.x - e01.x * e02.z) * iNy;
	} else {
		float iNz = 1.f / N.z;
		a11 = (e02.x * e03.y - e02.y * e03.x) * iNz;
		b11 = (e01.x * e02.y - e01.y * e02.x) * iNz;
	}

	// Compute the bilinear coordinates of the
	// intersection point.
	if (fabsf(a11 - 1.f) < 1e-7f) {
		u = alpha;
		v = fabsf(b11 - 1.f) < 1e-7f ? beta : beta / (u * (b11 - 1.f) + 1.f);
	} else if (fabsf(b11 - 1.f) < 1e-7f) {
		v = beta;
		u = alpha / (v * (a11 - 1.f) + 1.f);
	} else {
		float A = -(b11 - 1.f);
		float B = alpha*(b11 - 1.f) - beta*(a11 - 1.f) - 1.f;
		float C = alpha;

		Quadratic(A, B, C, &u, &v);		
		if ((u < 0) || (u > 1))
			u = v;
		
		v = beta / (u * (b11 - 1.f) + 1.f);
	}

	// TODO - verify differentials
	Vector dpdu = Vector((1.f - v) * (p00 + p10) + v * (p11 + p01));
	Vector dpdv = Vector((1.f - u) * (p00 + p01) + u * (p10 + p11));

    *dg = DifferentialGeometry(ray(t), dpdu, dpdv,
            Vector(0, 0, 0), Vector(0, 0, 0),
            u, v, this);

	*tHit = t;

	return true;
}

bool Quadrilateral::IntersectP(const Ray &ray) const {

	float tHit;
	DifferentialGeometry dg;

	return Intersect(ray, &tHit, &dg);
}

float Quadrilateral::Area() const {

	// assumes convex quadrilateral
    const Point &p0 = mesh->p[idx[0]];
    const Point &p1 = mesh->p[idx[1]];
    const Point &p2 = mesh->p[idx[2]];
    const Point &p3 = mesh->p[idx[3]];

	Vector P = p1 - p0;
	Vector Q = p3 - p1;

	Vector PxQ = Cross(P, Q);

	return 0.5f * PxQ.Length();
}
