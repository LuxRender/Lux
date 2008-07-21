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

#include "mesh.h"

using namespace lux;

float MeshQuadrilateral::Det2x2(const float a00, const float a01, const float a10, const float a11) {
	return a00*a11 - a01*a10;
}

float MeshQuadrilateral::Det3x3(float A[3][3]) {
	return 
		A[0][0] * Det2x2(A[1][1], A[1][2], A[2][1], A[2][2]) -
		A[0][1] * Det2x2(A[1][0], A[1][2], A[2][0], A[2][2]) +
		A[0][2] * Det2x2(A[1][0], A[1][1], A[2][0], A[2][1]);
}

bool MeshQuadrilateral::Invert3x3(float A[3][3], float InvA[3][3]) {

	float determinant = Det3x3(A);
	if (determinant == 0.f)
		return false;

	float invdet = 1.f / determinant;

	InvA[0][0] = invdet * Det2x2(A[1][1], A[1][2], A[2][1], A[2][2]);
	InvA[0][1] = invdet * Det2x2(A[0][2], A[0][1], A[2][2], A[2][1]);
	InvA[0][2] = invdet * Det2x2(A[0][1], A[0][2], A[1][1], A[1][2]);
	InvA[1][0] = invdet * Det2x2(A[1][2], A[1][0], A[2][2], A[2][0]);
	InvA[1][1] = invdet * Det2x2(A[0][0], A[0][2], A[2][0], A[2][2]);
	InvA[1][2] = invdet * Det2x2(A[0][2], A[0][0], A[1][2], A[1][0]);
	InvA[2][0] = invdet * Det2x2(A[1][0], A[1][1], A[2][0], A[2][1]);
	InvA[2][1] = invdet * Det2x2(A[0][1], A[0][0], A[2][1], A[2][0]);
	InvA[2][2] = invdet * Det2x2(A[0][0], A[0][1], A[1][0], A[1][1]);

	return true;
}

int MeshQuadrilateral::MajorAxis(const Vector &v) {
	float absVx = fabsf(v.x);
	float absVy = fabsf(v.y);
	float absVz = fabsf(v.z);
	
	if (absVx > absVy)
		return (absVx > absVz) ? 0 : 2;
	return (absVy > absVz) ? 1 : 2;
}

//------------------------------------------------------------------------------

MeshQuadrilateral::MeshQuadrilateral(const lux::Transform &o2w, bool ro, 
	Mesh *m, int n) : Shape(o2w, ro) {
	mesh = m;
	idx = &mesh->quadVertexIndex[4 * n];

	// Dade - reorder the vertices if required
	for(int i = 0; i < 4; i++) {
		// Get quadrilateral vertices in _p00_, _p10_, _p11_, and _p01_
		const Point &p00 = mesh->p[idx[0]];
		const Point &p10 = mesh->p[idx[1]];
		const Point &p11 = mesh->p[idx[2]];
		const Point &p01 = mesh->p[idx[3]];

		// Compute the barycentric coordinates of V11.
		const Vector e01 = p10 - p00;
		const Vector e03 = p01 - p00;
		const Vector e02 = p11 - p00;
		const Vector N = Cross(e01, e03);

		float a11 = 0.0f;
		float b11 = 0.0f;
		int Nma = MajorAxis(N);

		switch (Nma) {
			case 0: {
				float iNx = 1.f / N.x;
				a11 = (e02.y * e03.z - e02.z * e03.y) * iNx;
				b11 = (e01.y * e02.z - e01.z * e02.y) * iNx;
				break;
			}
			case 1: {
				float iNy = 1.f / N.y;
				a11 = (e02.z * e03.x - e02.x * e03.z) * iNy;
				b11 = (e01.z * e02.x - e01.x * e02.z) * iNy;
				break;
			}
			case 2: {
				float iNz = 1.f / N.z;
				a11 = (e02.x * e03.y - e02.y * e03.x) * iNz;
				b11 = (e01.x * e02.y - e01.y * e02.x) * iNz;
				break;
			}
			default:
				BOOST_ASSERT(false);
				// Dade - how can I report internal errors ?
				break;
		}

		if ((a11 > 1.0f) || (b11 > 1.0f)) {
			// Dade - we need to reorder the vertices

			// Dade - this code has as side effect to reorder the indices
			// in mesh->quadVertexIndex. It is not a very clean behavior but
			// it is simple and fast

			const int tmp = idx[0];
			idx[0] = idx[1];
			idx[1] = idx[2];
			idx[2] = idx[3];
			idx[3] = tmp;
		} else
			break;
	}
}

BBox MeshQuadrilateral::ObjectBound() const {
    // Get quadrilateral vertices in _p0_, _p1_, _p2_, and _p3_
    const Point &p0 = mesh->p[idx[0]];
    const Point &p1 = mesh->p[idx[1]];
    const Point &p2 = mesh->p[idx[2]];
    const Point &p3 = mesh->p[idx[3]];

	return Union(BBox(WorldToObject(p0), WorldToObject(p1)),
            BBox(WorldToObject(p2), WorldToObject(p3)));
}

BBox MeshQuadrilateral::WorldBound() const {
    // Get quadrilateral vertices in _p0_, _p1_, _p2_, and _p3_
    const Point &p0 = mesh->p[idx[0]];
    const Point &p1 = mesh->p[idx[1]];
    const Point &p2 = mesh->p[idx[2]];
    const Point &p3 = mesh->p[idx[3]];

    return Union(BBox(p0, p1), BBox(p2, p3));
}

bool MeshQuadrilateral::Intersect(const Ray &ray, float *tHit,
		DifferentialGeometry *dg) const {
	// Compute intersection for quadrilateral
	// based on "An Efficient Ray-Quadrilateral Intersection Test"
	// by Ares Lagae and Philip Dutr�
	// http://www.cs.kuleuven.be/~graphics/CGRG.PUBLICATIONS/LagaeDutre2005AnEfficientRayQuadrilateralIntersectionTest/
	// http://jgt.akpeters.com/papers/LagaeDutre05/erqit.cpp.html

    // Get quadrilateral vertices in _p00_, _p10_, _p11_, and _p01_
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
	if (fabsf(det) < 1e-7) 
		return false;

	float invdet = 1.f / det;

	const Vector T = ray.o - p00;
	float alpha = Dot(T, P) * invdet;	
	if (alpha < 0)// || alpha > 1) 
		return false;

	const Vector Q = Cross(T, e01);
	float beta = Dot(ray.d, Q) * invdet;
	if (beta < 0)// || beta > 1)
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
		float invdet2 = (det2 < 0) ? -1 : 1;
		const Vector T2 = ray.o - p11;
		float alpha2 = Dot(T2, P2) * invdet2;
		if (alpha2 < 0) 
			return false;
		const Vector Q2 = Cross(T2, e23);
		float beta2 = Dot(ray.d, Q2) * invdet2;
		if (beta2 < 0) 
			return false;
	}

	// Compute the ray parameter of the intersection
	// point.
	float t = Dot(e03, Q) * invdet;
	if (t < ray.mint || t > ray.maxt) 
		return false;

	// Compute the barycentric coordinates of V11.
	const Vector e02 = p11 - p00;
	const Vector N = Cross(e01, e03);

	float a11, b11;
	int Nma = MajorAxis(N);

	switch (Nma) {
		case 0: {
			float iNx = 1.f / N.x;
			a11 = (e02.y * e03.z - e02.z * e03.y) * iNx - 1.f;
			b11 = (e01.y * e02.z - e01.z * e02.y) * iNx - 1.f;
			break;
		}
		case 1: {
			float iNy = 1.f / N.y;
			a11 = (e02.z * e03.x - e02.x * e03.z) * iNy - 1.f;
			b11 = (e01.z * e02.x - e01.x * e02.z) * iNy - 1.f;
			break;
		}
		case 2: {
			float iNz = 1.f / N.z;
			a11 = (e02.x * e03.y - e02.y * e03.x) * iNz - 1.f;
			b11 = (e01.x * e02.y - e01.y * e02.x) * iNz - 1.f;
			break;
		}
		default:
            BOOST_ASSERT(false);
            // Dade - how can I report internal errors ?
            return false;
	}

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
		if ((u < 0) || (u > 1))
			u = v;
		
		v = beta / (u * b11 + 1.f);
	}

	// compute partial differentials
	Vector dpdu, dpdv;
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

	// Dade - using the intepolated normal here in order to fix bug #340
	Normal nn;
	if (mesh->n)
		nn = Normalize(ObjectToWorld(
				((1.0f - u) * (1.0f - v)) * mesh->n[idx[0]] +
				(u * (1.0f - v)) * mesh->n[idx[1]] +
				(u * v) * mesh->n[idx[2]] +
				((1.0f - u) * v) * mesh->n[idx[3]]));
	else
		nn = Normal(Normalize(N));

	// Adjust normal based on orientation and handedness
    if (reverseOrientation ^ transformSwapsHandedness)
        nn *= -1.f;

    *dg = DifferentialGeometry(ray(t),
			nn,
			dpdu, dpdv,
            Vector(0, 0, 0), Vector(0, 0, 0),
            u, v, this);

	*tHit = t;

	return true;
}

bool MeshQuadrilateral::IntersectP(const Ray &ray) const {

	float tHit;
	DifferentialGeometry dg;

	return Intersect(ray, &tHit, &dg);
}

float MeshQuadrilateral::Area() const {

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
		DifferentialGeometry *dgShading) const {
	// TODO
	*dgShading = dg;
}
