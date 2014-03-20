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

#include "mesh.h"
#include "geometry/matrix3x3.h"

using namespace luxrays;
using namespace lux;

// checks if quad is degenerate or if any points coincide
bool MeshQuadrilateral::IsDegenerate(const Point &p0, const Point &p1, const Point &p2, const Point &p3) {

	Vector e0 = p1 - p0;
	Vector e1 = p2 - p1;
	Vector e2 = p3 - p2;
	Vector e3 = p0 - p3;

	const float el0 = e0.Length();
	const float el1 = e1.Length();
	const float el2 = e2.Length();
	const float el3 = e3.Length();

	const float lmin = min(min(el0, el1), min(el2, el3));
	const float lmax = max(max(el0, el1), max(el2, el3));

	return lmax == 0.0 || (lmin / lmax) < 1e-30;
}

// checks if a non-degenerate quad is planar
bool MeshQuadrilateral::IsPlanar(const Point &p0, const Point &p1, const Point &p2, const Point &p3) {

	// calculate normal using Newells method
	Vector N(Normalize(Cross(p2 - p0, p3 - p1)));

	// calculate center
	Vector P(0.25 * (p0 + p1 + p2 + p3));

	float D = Dot(N, P);

	const float eps = 1e-3;
	float dist;

	// if planar, the distance from point to plane should be zero
	dist = fabsf(Dot(N, Vector(p0)) - D);
	if (dist > eps)
		return false;

	dist = fabsf(Dot(N, Vector(p1)) - D);
	if (dist > eps)
		return false;

	dist = fabsf(Dot(N, Vector(p2)) - D);
	if (dist > eps)
		return false;

	dist = fabsf(Dot(N, Vector(p3)) - D);
	if (dist > eps)
		return false;

	return true;
}

// checks if a non-degenerate, planar quad is strictly convex
bool MeshQuadrilateral::IsConvex(const Point &p0, const Point &p1, const Point &p2, const Point &p3) {

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
	for (u_int i = 0; i < 4; ++i)
		e[i] = Vector(Dot(e[i], b0), Dot(e[i], b1), 0);

	// in a convex polygon, the x values should alternate between
	// increasing and decreasing exactly twice
	u_int altCount = 0;
	int curd, prevd;

	// since b0 is constructed from the same edge as e0
	// it's x component will always be positive (|e0| is always > 0)
	// this is just a boot-strap, hence i=1..4 below
	curd = 1;
	for (u_int i = 1; i <= 4; ++i) {
		prevd = curd;
		// if x component of edge is zero, we simply ignore it by
		// using the previous direction
		curd = (e[i & 3].x < 1e-6f) ? (e[i & 3].x > -1e-6f ? prevd : -1) : 1;
		altCount += prevd != curd ? 1 : 0;
	}

	if (altCount != 2)
		return false;

	// some concave polygons might pass
	// the above test, verify that the turns
	// all go in the same direction
	int curs, prevs;
	altCount = 0;

	curs = (Cross(e[1], e[0]).z < 0.f) ? -1 : 1;
	for (u_int i = 1; i < 4; i++) {
		prevs = curs;
		curs = (Cross(e[(i + 1) & 3], e[i]).z < 0) ? -1 : 1;
		altCount += prevs != curs ? 1 : 0;
	}

	return altCount == 0;
}

u_int MeshQuadrilateral::MajorAxis(const Vector &v) {
	float absVx = fabsf(v.x);
	float absVy = fabsf(v.y);
	float absVz = fabsf(v.z);

	if (absVx > absVy)
		return (absVx > absVz) ? 0 : 2;
	return (absVy > absVz) ? 1 : 2;
}

void MeshQuadrilateral::ComputeV11BarycentricCoords(const Vector &e01,
	const Vector &e02, const Vector &e03, float *a11, float *b11) {
		const Vector N = Cross(e01, e03);

	u_int Nma = MajorAxis(N);

	switch (Nma) {
		case 0: {
			float iNx = 1.f / N.x;
			*a11 = (e02.y * e03.z - e02.z * e03.y) * iNx;
			*b11 = (e01.y * e02.z - e01.z * e02.y) * iNx;
			break;
		}
		case 1: {
			float iNy = 1.f / N.y;
			*a11 = (e02.z * e03.x - e02.x * e03.z) * iNy;
			*b11 = (e01.z * e02.x - e01.x * e02.z) * iNy;
			break;
		}
		case 2: {
			float iNz = 1.f / N.z;
			*a11 = (e02.x * e03.y - e02.y * e03.x) * iNz;
			*b11 = (e01.x * e02.y - e01.y * e02.x) * iNz;
			break;
		}
		default:
			BOOST_ASSERT(false);
			// since we don't allow for degenerate quads the normal
			// should always be well defined and we should never get here
			break;
	}
}

//------------------------------------------------------------------------------
MeshQuadrilateral::MeshQuadrilateral(const lux::Mesh *m, u_int n)
	: mesh(m), idx(&(mesh->quadVertexIndex[4 * n]))
{
	// LordCrc - check for problematic quads
	const Point &p0 = Inverse(mesh->ObjectToWorld) * mesh->p[idx[0]];
	const Point &p1 = Inverse(mesh->ObjectToWorld) * mesh->p[idx[1]];
	const Point &p2 = Inverse(mesh->ObjectToWorld) * mesh->p[idx[2]];
	const Point &p3 = Inverse(mesh->ObjectToWorld) * mesh->p[idx[3]];

	// assume convex and planar check is performed before
	if (IsDegenerate(p0, p1, p2, p3)) {
		LOG(LUX_DEBUG, LUX_CONSISTENCY)<< "Degenerate quadrilateral detected";
		idx = NULL;
	}

	if (!idx)
		return;

	// Dade - reorder the vertices if required
	for(u_int i = 0; i < 4; i++) {
		// Get quadrilateral vertices in _p00_, _p10_, _p11_ and _p01_
		const Point &p00 = mesh->p[idx[0]];
		const Point &p10 = mesh->p[idx[1]];
		const Point &p11 = mesh->p[idx[2]];
		const Point &p01 = mesh->p[idx[3]];

		// Compute the barycentric coordinates of V11.
		const Vector e01 = p10 - p00;
		const Vector e03 = p01 - p00;
		const Vector e02 = p11 - p00;
		//const Vector N = Cross(e01, e03);

		float a11 = 0.0f;
		float b11 = 0.0f;

		ComputeV11BarycentricCoords(e01, e02, e03, &a11, &b11);

		if ((a11 > 1.0f) || (b11 > 1.0f)) {
			// Dade - we need to reorder the vertices

			// Dade - this code has as side effect to reorder the indices
			// in mesh->quadVertexIndex. It is not a very clean behavior but
			// it is simple and fast

			int* nonConstIdx = const_cast<int*>(idx);
			const int tmp = nonConstIdx[0];
			nonConstIdx[0] = nonConstIdx[1];
			nonConstIdx[1] = nonConstIdx[2];
			nonConstIdx[2] = nonConstIdx[3];
			nonConstIdx[3] = tmp;
		} else
			break;
	}
}

BBox MeshQuadrilateral::ObjectBound() const {

	if (!idx)
		return BBox();

	// Get quadrilateral vertices in _p0_, _p1_, _p2_, and _p3_
	const Point &p0 = mesh->p[idx[0]];
	const Point &p1 = mesh->p[idx[1]];
	const Point &p2 = mesh->p[idx[2]];
	const Point &p3 = mesh->p[idx[3]];

	return Union(BBox(Inverse(mesh->ObjectToWorld) * p0,
		Inverse(mesh->ObjectToWorld) * p1),
		BBox(Inverse(mesh->ObjectToWorld) * p2,
		Inverse(mesh->ObjectToWorld) * p3));
}

BBox MeshQuadrilateral::WorldBound() const {

	if (!idx)
		return BBox();

	// Get quadrilateral vertices in _p0_, _p1_, _p2_, and _p3_
	const Point &p0 = mesh->p[idx[0]];
	const Point &p1 = mesh->p[idx[1]];
	const Point &p2 = mesh->p[idx[2]];
	const Point &p3 = mesh->p[idx[3]];

	return Union(BBox(p0, p1), BBox(p2, p3));
}

bool MeshQuadrilateral::Intersect(const Ray &ray, Intersection *isect) const {
	// Compute intersection for quadrilateral
	// based on "An Efficient Ray-Quadrilateral Intersection Test"
	// by Ares Lagae and Philip Dutr�
	// http://www.cs.kuleuven.be/~graphics/CGRG.PUBLICATIONS/LagaeDutre2005AnEfficientRayQuadrilateralIntersectionTest/
	// http://jgt.akpeters.com/papers/LagaeDutre05/erqit.cpp.html

	if (!idx)
		return false;

	// Get quadrilateral vertices in _p00_, _p10_, _p11_ and _p01_
	const Point &p00 = mesh->p[idx[0]];
	const Point &p10 = mesh->p[idx[1]];
	const Point &p11 = mesh->p[idx[2]];
	const Point &p01 = mesh->p[idx[3]];

	// Reject rays using the barycentric coordinates of
	// the intersection point with respect to T.
	const Vector e01 = p10 - p00;
	const Vector e03 = p01 - p00;
	const Vector P = Cross(ray.d, e03);
	float det = Dot(e01, P);
	if (fabsf(det) < 1e-7f)
		return false;

	float invdet = 1.f / det;

	const Vector T = ray.o - p00;
	float alpha = Dot(T, P) * invdet;
	if (alpha < 0.f)// || alpha > 1)
		return false;

	const Vector Q = Cross(T, e01);
	float beta = Dot(ray.d, Q) * invdet;
	if (beta < 0.f)// || beta > 1)
		return false;

	// Reject rays using the barycentric coordinates of
	// the intersection point with respect to T'.
	if ((alpha + beta) > 1.f) {
		const Vector e23 = p01 - p11;
		const Vector e21 = p10 - p11;
		const Vector P2 = Cross(ray.d, e21);
		float det2 = Dot(e23, P2);
		if (fabsf(det2) < 1e-7f)
			return false;
		//float invdet2 = 1.f / det2;
		// since we only reject if alpha or beta < 0
		// we just need the sign info from det2
		float invdet2 = (det2 < 0) ? -1.f : 1.f;
		const Vector T2 = ray.o - p11;
		float alpha2 = Dot(T2, P2) * invdet2;
		if (alpha2 < 0.f)
			return false;
		const Vector Q2 = Cross(T2, e23);
		float beta2 = Dot(ray.d, Q2) * invdet2;
		if (beta2 < 0.f)
			return false;
	}

	// Compute the ray parameter of the intersection
	// point.
	float t = Dot(e03, Q) * invdet;
	if (t < ray.mint || t > ray.maxt)
		return false;

	// Compute the barycentric coordinates of V11.
	const Vector e02 = p11 - p00;

	float a11 = 0.f;
	float b11 = 0.f;

	ComputeV11BarycentricCoords(e01, e02, e03, &a11, &b11);

	// save a lot of redundant computations
	a11 = a11 - 1;
	b11 = b11 - 1;

	// Compute the bilinear coordinates of the
	// intersection point.
	float u = 0.0f, v = 0.0f;
	if (fabsf(a11) < 1e-7f) {
		u = alpha;
		v = fabsf(b11) < 1e-7f ? beta : beta / (u * b11 + 1.f);
	} else if (fabsf(b11) < 1e-7f) {
		v = beta;
		u = alpha / (v * a11 + 1.f);
	} else {
		float A = -b11;
		float B = alpha*b11 - beta*a11 - 1.f;
		float C = alpha;

		Quadratic(A, B, C, &u, &v);
		if ((u < 0.f) || (u > 1.f))
			u = v;

		v = beta / (u * b11 + 1.f);
	}

	// compute partial differentials
	// see bugtracker ID 324 for derivation
	Vector dpdu, dpdv;
	float uv[4][2];
	float A[3][3], InvA[3][3];

	GetUVs(uv);

	A[0][0] = uv[1][0] - uv[0][0];
	A[0][1] = uv[1][1] - uv[0][1];
	A[0][2] = uv[1][0] * uv[1][1] - uv[0][0] * uv[0][1];
	A[1][0] = uv[2][0] - uv[0][0];
	A[1][1] = uv[2][1] - uv[0][1];
	A[1][2] = uv[2][0] * uv[2][1] - uv[0][0] * uv[0][1];
	A[2][0] = uv[3][0] - uv[0][0];
	A[2][1] = uv[3][1] - uv[0][1];
	A[2][2] = uv[3][0] * uv[3][1] - uv[0][0] * uv[0][1];

	// invert matrix
	if (!Invert3x3(A, InvA)) {
		// Handle zero determinant for quadrilateral partial derivative matrix
		Vector N = Cross(e01, e02);
		CoordinateSystem(Normalize(N), &dpdu, &dpdv);
	} else {
		dpdu = Vector(
			InvA[0][0] * e01.x + InvA[0][1] * e02.x + InvA[0][2] * e03.x,
			InvA[0][0] * e01.y + InvA[0][1] * e02.y + InvA[0][2] * e03.y,
			InvA[0][0] * e01.z + InvA[0][1] * e02.z + InvA[0][2] * e03.z);
		dpdv = Vector(
			InvA[1][0] * e01.x + InvA[1][1] * e02.x + InvA[1][2] * e03.x,
			InvA[1][0] * e01.y + InvA[1][1] * e02.y + InvA[1][2] * e03.y,
			InvA[1][0] * e01.z + InvA[1][1] * e02.z + InvA[1][2] * e03.z);
	}

	Vector N = Cross(e01, e02);
	Normal nn(Normal(Normalize(N)));

	if (isect) {
		isect->dg = DifferentialGeometry(ray(t),
			nn,
			dpdu, dpdv,
			Normal(0, 0, 0), Normal(0, 0, 0),
			u, v, this);
		isect->dg.AdjustNormal(mesh->reverseOrientation, mesh->transformSwapsHandedness);
		isect->Set(mesh->ObjectToWorld, this, mesh->GetMaterial(),
			mesh->GetExterior(), mesh->GetInterior());
		ray.maxt = t;
	}

	return true;
}

bool MeshQuadrilateral::IntersectP(const Ray &ray) const {
	return Intersect(ray, NULL);
}

float MeshQuadrilateral::Area() const {

	if (!idx)
		return 0.f;

	// assumes convex quadrilateral
	const Point &p0 = mesh->p[idx[0]];
	const Point &p1 = mesh->p[idx[1]];
	const Point &p3 = mesh->p[idx[3]];

	Vector P = p1 - p0;
	Vector Q = p3 - p1;

	Vector PxQ = Cross(P, Q);

	return 0.5f * PxQ.Length();
}

void MeshQuadrilateral::GetShadingGeometry(const Transform &obj2world,
	const DifferentialGeometry &dg,
	DifferentialGeometry *dgShading) const
{
	if (!mesh->n) {
		*dgShading = dg;
		if (!mesh->uvs) {
			// Lotus - the length of dpdu/dpdv can be important for bumpmapping
			const BBox bounds = MeshQuadrilateral::WorldBound();
			int maxExtent = bounds.MaximumExtent();
			float maxSize = bounds.pMax[maxExtent] - bounds.pMin[maxExtent];
			dgShading->dpdu *= (maxSize * .1f) / dgShading->dpdu.Length();
			dgShading->dpdv *= (maxSize * .1f) / dgShading->dpdv.Length();
		}
		return;
	}

	// Use _n_ and _s_ to compute shading tangents for triangle, _ss_ and _ts_
	Normal ns(Normalize(mesh->ObjectToWorld * (
		((1.0f - dg.u) * (1.0f - dg.v)) * mesh->n[idx[0]] +
		(dg.u * (1.0f - dg.v)) * mesh->n[idx[1]] +
		(dg.u * dg.v) * mesh->n[idx[2]] +
		((1.0f - dg.u) * dg.v) * mesh->n[idx[3]])));
	float lenDpDu = dg.dpdu.Length();
	float lenDpDv = dg.dpdv.Length();
	Vector ts = Normalize(Cross(dg.dpdu, ns));
	Vector ss = Cross(ts, ns);
	// Lotus - the length of dpdu/dpdv can be important for bumpmapping
	if (mesh->uvs) {
		ss *= lenDpDu;
		ts *= lenDpDv;
	} else {
		const BBox bounds = MeshQuadrilateral::WorldBound();
		int maxExtent = bounds.MaximumExtent();
		float maxSize = bounds.pMax[maxExtent] - bounds.pMin[maxExtent];
		ss *= maxSize * .1f;
		ts *= maxSize * .1f;
	}

	// compute partial differentials
	// see bugtracker ID 324 for derivation
	Normal dndu, dndv;
	float uv[4][2];
	float A[3][3], InvA[3][3];

	GetUVs(uv);

	A[0][0] = uv[1][0] - uv[0][0];
	A[0][1] = uv[1][1] - uv[0][1];
	A[0][2] = uv[1][0]*uv[1][1] - uv[0][0]*uv[0][1];
	A[1][0] = uv[2][0] - uv[0][0];
	A[1][1] = uv[2][1] - uv[0][1];
	A[1][2] = uv[2][0]*uv[2][1] - uv[0][0]*uv[0][1];
	A[2][0] = uv[3][0] - uv[0][0];
	A[2][1] = uv[3][1] - uv[0][1];
	A[2][2] = uv[3][0]*uv[3][1] - uv[0][0]*uv[0][1];

	// invert matrix
	if (!Invert3x3(A, InvA)) {
        // Handle zero determinant for quadrilateral partial derivative matrix
		dndu = dndv = Normal(0, 0, 0);
	} else {
		const Normal &n00 = mesh->n[idx[0]];
		const Normal &n10 = mesh->n[idx[1]];
		const Normal &n11 = mesh->n[idx[2]];
		const Normal &n01 = mesh->n[idx[3]];

		const Normal dn01 = n10 - n00;
		const Normal dn02 = n11 - n00;
		const Normal dn03 = n01 - n00;

		dndu = Normal(
			InvA[0][0] * dn01.x + InvA[0][1] * dn02.x + InvA[0][2] * dn03.x,
			InvA[0][0] * dn01.y + InvA[0][1] * dn02.y + InvA[0][2] * dn03.y,
			InvA[0][0] * dn01.z + InvA[0][1] * dn02.z + InvA[0][2] * dn03.z);
		dndv = Normal(
			InvA[1][0] * dn01.x + InvA[1][1] * dn02.x + InvA[1][2] * dn03.x,
			InvA[1][0] * dn01.y + InvA[1][1] * dn02.y + InvA[1][2] * dn03.y,
			InvA[1][0] * dn01.z + InvA[1][1] * dn02.z + InvA[1][2] * dn03.z);

		dndu *= obj2world;
		dndv *= obj2world;
	}

	*dgShading = DifferentialGeometry(dg.p, ns, ss, ts, dndu, dndv,
		dg.u, dg.v, this);
}

void MeshQuadrilateral::GetShadingInformation(const DifferentialGeometry &dgShading,
		RGBColor *color, float *alpha) const {
	if (mesh->cols) {
		const RGBColor *c0 = (const RGBColor *)(&mesh->cols[idx[0] * 3]);
		const RGBColor *c1 = (const RGBColor *)(&mesh->cols[idx[1] * 3]);
		const RGBColor *c2 = (const RGBColor *)(&mesh->cols[idx[2] * 3]);
		const RGBColor *c3 = (const RGBColor *)(&mesh->cols[idx[3] * 3]);

		*color = ((1.0f - dgShading.u) * (1.0f - dgShading.v)) * (*c0) +
				(dgShading.u * (1.0f - dgShading.v)) * (*c1) +
				(dgShading.u * dgShading.v) * (*c2) +
				((1.0f - dgShading.u) * dgShading.v) * (*c3);
	} else
		*color = RGBColor(1.f);

	if (mesh->alphas) {
		const float alpha0 = mesh->alphas[idx[0]];
		const float alpha1 = mesh->alphas[idx[1]];
		const float alpha2 = mesh->alphas[idx[2]];
		const float alpha3 = mesh->alphas[idx[3]];

		*alpha = ((1.0f - dgShading.u) * (1.0f - dgShading.v)) * alpha0 +
				(dgShading.u * (1.0f - dgShading.v)) * alpha1 +
				(dgShading.u * dgShading.v) * alpha2 +
				((1.0f - dgShading.u) * dgShading.v) * alpha3;
	} else
		*alpha = 1.f;
}
