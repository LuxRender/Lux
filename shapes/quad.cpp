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

// quad.cpp*
#include "quad.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Quad Method Definitions
Quad::Quad(const lux::Transform &o2w, bool ro, int nq, int nv, 
		const int *vi, const Point *P) : Shape(o2w, ro) {
	
	mesh = new QuadMesh(o2w, ro, nq, nv, vi, P);
	quad = new Quadrilateral(o2w, ro, mesh, &mesh->idx[0]);

}

Quad::~Quad() {
	delete quad;
	delete mesh;
}

BBox Quad::ObjectBound() const {
	return quad->ObjectBound();
}

BBox Quad::WorldBound() const {
	return quad->WorldBound();
}

bool Quad::Intersect(const Ray &ray, float *tHit,
		DifferentialGeometry *dg) const {

	return quad->Intersect(ray, tHit, dg);
}

bool Quad::IntersectP(const Ray &ray) const {

	return quad->IntersectP(ray);
}

float Quad::Area() const {

	return quad->Area();
}

// checks if quad is degenerate or if any points coincide
bool IsDegenerate(const Point &p0, const Point &p1, const Point &p2, const Point &p3) {

	Vector e0 = p1 - p0;
	Vector e1 = p2 - p1;
	Vector e2 = p3 - p2;
	Vector e3 = p0 - p3;

	return (e0.Length() < 1e-6f || e1.Length() < 1e-6f ||
		e2.Length() < 1e-6f || e3.Length() < 1e-6f);
}

// checks if a non-degenerate quad is planar
// most likely susceptible to numerical issues for large quads
bool IsPlanar(const Point &p0, const Point &p1, const Point &p2, const Point &p3) {

	// basis vectors for projection
	Vector e0 = Normalize(p1 - p0);
	Vector e1 = Normalize(p3 - p0);
	
	if (1.f - fabsf(Dot(e0, e1)) < 1e-6) {
		// if collinear, use p2
		e1 = Normalize(p2 - p0);
	}

	Vector n = Normalize(Cross(e0, e1));

	// project p3 onto the normal created by e0 and e1
	float proj = fabsf(Dot(Vector(p3), n));

	// if planar, the projection length should be zero
	return proj < 1e-6f;
}

// checks if a non-degenerate, planar quad is strictly convex
bool IsConvex(const Point &p0, const Point &p1, const Point &p2, const Point &p3) {

	// basis vectors for plane
	Vector b0 = Normalize(p1 - p0);
	Vector b1 = p3 - p0;
	// orthogonalize using Gram-Schmitdt
	b1 = Normalize(b1 - b0 * Dot(b1, b0));
	
	if (1.f - fabsf(Dot(b0, b1)) < 1e-6) {
		// if collinear, use p2
		b1 = p2 - p0;
		// orthogonalize using Gram-Schmitdt
		b1 = Normalize(b1 - b0 * Dot(b1, b0));
	}

	// compute polygon edges
	Vector e[4];

	e[0] = p1 - p0;
	e[1] = p2 - p1;
	e[2] = p3 - p2;
	e[3] = p0 - p3;	

	// project edges onto the plane
	for (int i = 0; i < 4; i++)
		e[i] = Vector(Dot(e[i], b0), Dot(e[i], b1), 0);

	// in a convex polygon, the x values should alternate between 
	// increasing and decreasing exactly twice
	int altCount = 0;
	int curd, prevd;

	// since b0 is constructed from the same edge as e0
	// it's x component will always be positive (|e0| is always > 0)
	// this is just a boot-strap, hence i=1..4 below
	curd = 1;
	for (int i = 1; i <= 4; i++) {
		prevd = curd;
		// if x component of edge is zero, we simply ignore it by 
		// using the previous direction
		curd = (e[i & 3].x < 1e-6) ? (e[i & 3].x > -1e-6f ? prevd : -1) : 1;
		altCount += prevd != curd ? 1 : 0;
	}

	if (altCount != 2)
		return false;

	// some concave polygons might pass
	// the above test, verify that the turns
	// all go in the same direction	
	int curs, prevs;
	altCount = 0;

	curs = (Cross(e[1], e[0]).z < 0) ? -1 : 1;
	for (int i = 1; i < 4; i++) {
		prevs = curs;
		curs = (Cross(e[(i + 1) & 3], e[i]).z < 0) ? -1 : 1;
		altCount += prevs != curs ? 1 : 0;
	}

	return altCount == 0;
}

Shape* Quad::CreateShape(const Transform &o2w,
		bool reverseOrientation, const ParamSet &params) {

	int np = 0;
	const Point *P = params.FindPoint("P", &np);

	if (P == NULL) {
		float hw = params.FindOneFloat("width", 1.f) / 2.f;
		float hh = params.FindOneFloat("height", 1.f) / 2.f;

		Point p[4];

		p[0] = Point(-hw, -hh, 0.f);
		p[1] = Point( hw, -hh, 0.f);
		p[2] = Point( hw,  hh, 0.f);
		p[3] = Point(-hw,  hh, 0.f);

		P = p;
	}
	else if (np != 4) {
		std::stringstream ss;
		ss<<"Number of vertices for quad must be 4";
		luxError(LUX_CONSISTENCY,LUX_ERROR,ss.str().c_str());
		return NULL;
	}

	const Point &p0 = P[0];
	const Point &p1 = P[1];
	const Point &p2 = P[2];
	const Point &p3 = P[3];

	if (IsDegenerate(p0, p1, p2, p3)) {
		std::stringstream ss;
		ss<<"Degenerate quad detected";
		luxError(LUX_CONSISTENCY,LUX_ERROR,ss.str().c_str());
		return NULL;
	}

	if (!IsPlanar(p0, p1, p2, p3)) {
		std::stringstream ss;
		ss<<"Non-planar quad detected";
		luxError(LUX_CONSISTENCY,LUX_ERROR,ss.str().c_str());
		return NULL;
	}

	if (!IsConvex(p0, p1, p2, p3)) {
		std::stringstream ss;
		ss<<"Non-convex quad detected";
		luxError(LUX_CONSISTENCY,LUX_ERROR,ss.str().c_str());
		return NULL;
	}

	int vi[4];
	for (int i = 0; i < 4; i++)
		vi[i] = i;

	return new Quad(o2w, reverseOrientation, 1, 4, vi, P);
}

static DynamicLoader::RegisterShape<Quad> r("quad");
