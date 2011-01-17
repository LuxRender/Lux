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
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

// Code based on the following paper:
// "Direct Ray Tracing of Displacement Mapped Triangles" by Smits, Shirley and Stark
// URL: http://www.cs.utah.edu/~bes/papers/height/paper.html


#include "mesh.h"
#include "mc.h"
#include "texture.h"
#include "spectrumwavelengths.h"
#include <algorithm>

using namespace lux;

MeshMicroDisplacementTriangle::MeshMicroDisplacementTriangle(const Mesh *m, u_int n) :
	mesh(m), v(&(mesh->triVertexIndex[3 * n]))
{
	int *v_ = const_cast<int *>(v);
	if (m->reverseOrientation ^ m->transformSwapsHandedness)
		swap(v_[1], v_[2]);

	const Point &v0 = m->p[v[0]];
	const Point &v1 = m->p[v[1]];
	const Point &v2 = m->p[v[2]];
	const Vector e1(v1 - v0);
	const Vector e2(v2 - v0);

	normalizedNormal = Normalize(Cross(e1, e2));

	// Reorder vertices if geometric normal doesn't match shading normal
	if (m->n) {
		const float cos0 = Dot(normalizedNormal, m->n[v[0]]);
		if (cos0 < 0.f) {
			if (Dot(normalizedNormal, m->n[v[1]]) < 0.f &&
				Dot(normalizedNormal, m->n[v[2]]) < 0.f)
				swap(v_[1], v_[2]);
			else {
				LOG(LUX_WARNING, LUX_CONSISTENCY) <<
					"Inconsistent shading normals";
			}
		} else if (cos0 > 0.f) {
			if (!(Dot(normalizedNormal, m->n[v[1]]) > 0.f &&
				Dot(normalizedNormal, m->n[v[2]]) > 0.f)) {
				LOG(LUX_WARNING, LUX_CONSISTENCY) <<
					"Inconsistent shading normals";
			}
		}
	}

	GetUVs(uvs);
	// Compute deltas for triangle partial derivatives
	const float du1 = uvs[0][0] - uvs[2][0];
	const float du2 = uvs[1][0] - uvs[2][0];
	const float dv1 = uvs[0][1] - uvs[2][1];
	const float dv2 = uvs[1][1] - uvs[2][1];
	const Vector dp1(m->p[v[0]] - m->p[v[2]]), dp2(m->p[v[1]] - m->p[v[2]]);
	const float determinant = du1 * dv2 - dv1 * du2;
	if (determinant == 0.f) {
		// Handle 0 determinant for triangle partial derivative matrix
		CoordinateSystem(normalizedNormal, &dpdu, &dpdv);
	} else {
		const float invdet = 1.f / determinant;
		dpdu = ( dv2 * dp1 - dv1 * dp2) * invdet;
		dpdv = (-du2 * dp1 + du1 * dp2) * invdet;
	}
}

Vector MeshMicroDisplacementTriangle::GetN(u_int i) const
{
	if (mesh->n)
		return Vector(mesh->n[v[i]]);
	else
		return normalizedNormal;
}

BBox MeshMicroDisplacementTriangle::ObjectBound() const
{
	return mesh->WorldToObject(WorldBound());
}

BBox MeshMicroDisplacementTriangle::WorldBound() const
{
	// Get triangle vertices in _p1_, _p2_, and _p3_
	const Point &p1 = mesh->p[v[0]];
	const Point &p2 = mesh->p[v[1]];
	const Point &p3 = mesh->p[v[2]];
	
	const Vector n1(GetN(0));
	const Vector n2(GetN(1));
	const Vector n3(GetN(2));
	
	const float M = mesh->displacementMapOffset + mesh->displacementMapScale;
	const float m = mesh->displacementMapOffset - mesh->displacementMapScale;

	const BBox bb1(p1 + M * n1, p1 + m * n1);
	const BBox bb2(p2 + M * n2, p2 + m * n2);
	const BBox bb3(p3 + M * n3, p3 + m * n3);

	return Union(Union(bb1, bb2), bb3);
}

static bool intersectTri(const Ray &ray, const Point &p1,
	const Vector &e1, const Vector &e2, float *b0, float *b1, float *b2,
	float *t)
{
	const Vector s1(Cross(ray.d, e2));
	const float divisor = Dot(s1, e1);
	if (divisor == 0.f)
		return false;
	const double invDivisor = 1.0 / divisor;
	// Compute first barycentric coordinate
	const Vector d(ray.o - p1);
	*b1 = static_cast<float>(Dot(d, s1) * invDivisor);
	if (*b1 < 0.f)
		return false;
	// Compute second barycentric coordinate
	const Vector s2(Cross(d, e1));
	*b2 = static_cast<float>(Dot(ray.d, s2) * invDivisor);
	if (*b2 < 0.f)
		return false;
	*b0 = 1.f - *b1 - *b2;
	if (*b0 < 0.f)
		return false;
	// Compute _t_ to intersection point
	*t = static_cast<float>(Dot(e2, s2) * invDivisor);

	return true;
}


static u_int MajorAxis(const Vector &v) 
{
	const float absVx = fabsf(v.x);
	const float absVy = fabsf(v.y);
	const float absVz = fabsf(v.z);

	if (absVx > absVy)
		return (absVx > absVz) ? 0 : 2;
	return (absVy > absVz) ? 1 : 2;
}

static void ComputeV11BarycentricCoords(const Vector &e01,
	const Vector &e02, const Vector &e03, float *a11, float *b11) 
{	
	const Vector N(Cross(e01, e03));

	switch (MajorAxis(N)) {
		case 0: {
			const float iNx = 1.f / N.x;
			*a11 = (e02.y * e03.z - e02.z * e03.y) * iNx;
			*b11 = (e01.y * e02.z - e01.z * e02.y) * iNx;
			break;
		}
		case 1: {
			const float iNy = 1.f / N.y;
			*a11 = (e02.z * e03.x - e02.x * e03.z) * iNy;
			*b11 = (e01.z * e02.x - e01.x * e02.z) * iNy;
			break;
		}
		case 2: {
			const float iNz = 1.f / N.z;
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

static bool intersectQuad(const Ray &ray, const Point &p00, const Point &p10,
	const Point &p01, const Point &p11, float *u, float *v, float *t)
{
	// Reject rays using the barycentric coordinates of
	// the intersection point with respect to T.
	const Vector e01(p10 - p00);
	const Vector e03(p01 - p00);
	const Vector P(Cross(ray.d, e03));
	const float det = Dot(e01, P);
	if (fabsf(det) < 1e-7f)
		return false;

	const float invdet = 1.f / det;

	const Vector T(ray.o - p00);
	const float alpha = Dot(T, P) * invdet;
	if (alpha < 0.f)// || alpha > 1)
		return false;

	const Vector Q(Cross(T, e01));
	const float beta = Dot(ray.d, Q) * invdet;
	if (beta < 0.f)// || beta > 1)
		return false;

	// Reject rays using the barycentric coordinates of
	// the intersection point with respect to T'.
	if ((alpha + beta) > 1.f) {
		const Vector e23(p01 - p11);
		const Vector e21(p10 - p11);
		const Vector P2(Cross(ray.d, e21));
		const float det2 = Dot(e23, P2);
		if (fabsf(det2) < 1e-7f)
			return false;
		// since we only reject if alpha or beta < 0
		// we just need the sign info from det2
		const Vector T2(ray.o - p11);
		const float alpha2 = Dot(T2, P2);
		if (det2 < 0.f) {
			if (alpha2 > 0.f)
				return false;
		} else if (alpha2 < 0.f)
			return false;
		const Vector Q2 = Cross(T2, e23);
		float beta2 = Dot(ray.d, Q2);
		if (det2 < 0.f) {
			if (beta2 > 0.f)
				return false;
		} else if (beta2 < 0.f)
			return false;
	}

	// Compute the ray parameter of the intersection
	// point.
	*t = Dot(e03, Q) * invdet;
	if (!(*t > ray.mint && *t < ray.maxt))
		return false;

	// Compute the barycentric coordinates of V11.
	const Vector e02(p11 - p00);

	float a11, b11;

	ComputeV11BarycentricCoords(e01, e02, e03, &a11, &b11);

	// save a lot of redundant computations
	a11 -= 1.f;
	b11 -= 1.f;

	// Compute the bilinear coordinates of the
	// intersection point.
	if (fabsf(a11) < 1e-7f) {
		*u = alpha;
		*v = fabsf(b11) < 1e-7f ? beta : beta / (alpha * b11 + 1.f);
	} else if (fabsf(b11) < 1e-7f) {
		*v = beta;
		*u = alpha / (beta * a11 + 1.f);
	} else {
		const float A = -b11;
		const float B = alpha * b11 - beta * a11 - 1.f;
		const float C = alpha;

		Quadratic(A, B, C, u, v);
		if ((*u < 0.f) || (*u > 1.f))
			*u = *v;

		*v = beta / (*u * b11 + 1.f);
	}

	return true;
}

Point MeshMicroDisplacementTriangle::GetDisplacedP(const Point &pbase, const Vector &n, const float u, const float v, const float w) const
{
		const float tu = u * uvs[0][0] + v * uvs[1][0] + w * uvs[2][0];
		const float tv = u * uvs[0][1] + v * uvs[1][1] + w * uvs[2][1];

		const DifferentialGeometry dg(pbase, Normal(n), dpdu, dpdv,
			Normal(0, 0, 0), Normal(0, 0, 0), tu, tv, this);

		SpectrumWavelengths sw;

		Vector displacement(n);
		displacement *=	(
			Clamp(mesh->displacementMap->Evaluate(sw, dg), -1.f, 1.f) * mesh->displacementMapScale +
			mesh->displacementMapOffset);

		return pbase + displacement;
}

static bool intersectPlane(const Ray &ray, const Point &p1, const Point &p2, const Point &p3, float *t)
{
	const Vector e1(p2 - p1);
	const Vector e2(p3 - p1);
	const Vector n(Cross(e1, e2));

	const float num = Dot(n, ray.d);

	if (fabsf(num) < MachineEpsilon::E(fabsf(num)))
		return false;

	*t = Dot(n, p1 - ray.o) / num;

	return true;
}

enum LastChange { iplus, jminus, kplus, iminus, jplus, kminus };

bool MeshMicroDisplacementTriangle::Intersect(const Ray &ray, Intersection* isect) const
{
	// Compute $\VEC{s}_1$
	// Get triangle vertices in _p1_, _p2_, and _p3_
	const Point &p1 = mesh->p[v[0]];
	const Point &p2 = mesh->p[v[1]];
	const Point &p3 = mesh->p[v[2]];

	const Vector n1(GetN(0));
	const Vector n2(GetN(1));
	const Vector n3(GetN(2));

	const int N = mesh->nSubdivLevels;
	const float delta = 1.f / N;

	int i = -1, j = -1, k = -1; // indicies of current cell
	int ei = -1, ej = -1, ek = -1; // indicies of end cell
	int enterSide = -1; // which side the ray enters the volume
	int exitSide = -1; // which side the ray exits the volume

	// find start and end hitpoints in volume	
	float tmin = 1e30f;
	float tmax = -1e30f;

	// upper and lower displacment
	const float M = mesh->displacementMapOffset + mesh->displacementMapScale;
	const float m = mesh->displacementMapOffset - mesh->displacementMapScale;

	// determine initial i,j,k for travesal
	// check all faces of volume

	// top face
	{
		const Point pa(p1 + M * n1);
		const Point pb(p2 + M * n2);
		const Point pc(p3 + M * n3);

		const Vector e1(pb - pa);
		const Vector e2(pc - pa);

		float t, b0, b1, b2;

		if (intersectTri(ray, pa, e1, e2, &b0, &b1, &b2, &t)) {
			if (t < tmin) {
				tmin = t;
				i = min(Floor2Int(b0 * N), N - 1);
				j = min(Floor2Int(b1 * N), N - 1);
				k = min(Floor2Int(b2 * N), N - 1);
			}
			if (t > tmax) {
				tmax = t;
				ei = min(Floor2Int(b0 * N), N - 1);
				ej = min(Floor2Int(b1 * N), N - 1);
				ek = min(Floor2Int(b2 * N), N - 1);
			}
		}
	}

	// bottom face
	{
		const Point pa(p1 + m * n1);
		const Point pb(p2 + m * n2);
		const Point pc(p3 + m * n3);

		const Vector e1(pb - pa);
		const Vector e2(pc - pa);

		float t, b0, b1, b2;

		if (intersectTri(ray, pa, e1, e2, &b0, &b1, &b2, &t)) {
			if (t < tmin) {
				tmin = t;
				i = min(Floor2Int(b0 * N), N - 1);
				j = min(Floor2Int(b1 * N), N - 1);
				k = min(Floor2Int(b2 * N), N - 1);
			}
			if (t > tmax) {
				tmax = t;
				ei = min(Floor2Int(b0 * N), N - 1);
				ej = min(Floor2Int(b1 * N), N - 1);
				ek = min(Floor2Int(b2 * N), N - 1);
			}
		}
	}

	// first side
	{
		const Point p00(p1 + m * n1);
		const Point p10(p2 + m * n2);
		const Point p01(p1 + M * n1);
		const Point p11(p2 + M * n2);

		float t, u ,v;

		// TODO - cheaper ray-quad test (possibly ray-plane?)
		if (intersectQuad(ray, p00, p10, p01, p11, &u, &v, &t)) {
			// always hits a lower triangle, so i+j+k == N-1
			if (t < tmin) {
				tmin = t;
				j = min(Floor2Int(u * N), N - 1);
				k = 0;
				i = (N - 1) - j;
				enterSide = 1;
			}
			if (t > tmax) {
				tmax = t;
				ej = min(Floor2Int(u * N), N - 1);
				ek = 0;
				ei = (N - 1) - ej;
				exitSide = 1;
			}
		}
	}


	// second side
	{
		const Point p00(p2 + m * n2);
		const Point p10(p3 + m * n3);
		const Point p01(p2 + M * n2);
		const Point p11(p3 + M * n3);

		float t, u ,v;

		if (intersectQuad(ray, p00, p10, p01, p11, &u, &v, &t)) {
			// always hits a lower triangle, so i+j+k == N-1
			if (t < tmin) {
				tmin = t;
				i = 0;
				k = min(Floor2Int(u * N), N - 1);
				j = (N - 1) - k;
				enterSide = 2;
			}
			if (t > tmax) {
				tmax = t;
				ei = 0;
				ek = min(Floor2Int(u * N), N - 1);
				ej = (N - 1) - ek;
				exitSide = 2;
			}
		}
	}

	// third side
	{
		const Point p00(p3 + m * n3);
		const Point p10(p1 + m * n1);
		const Point p01(p3 + M * n3);
		const Point p11(p1 + M * n1);

		float t, u ,v;

		if (intersectQuad(ray, p00, p10, p01, p11, &u, &v, &t)) {
			// always hits a lower triangle, so i+j+k == N-1
			if (t < tmin) {
				tmin = t;
				i = min(Floor2Int(u * N), N - 1);
				j = 0;
				k = (N - 1) - i;
				enterSide = 3;
			}
			if (t > tmax) {
				tmax = t;
				ei = min(Floor2Int(u * N), N - 1);
				ej = 0;
				ek = (N - 1) - ei;
				exitSide = 3;
			}
		}
	}

	// no hit
	if (i < 0 || j < 0 || k < 0)
		return false;


	// initialize microtriangle vertices a,b,c
	// order doesnt matter since we wont traverse
	float ua, ub, uc; // first barycentric coordinate
	float va, vb, vc; // second barycentric coordinate
	float wa, wb, wc; // third barycentric coordinate
	if (i+j+k == N-1) {
		// lower triangle
		ua = (i+1) * delta;
		va = j * delta;
		wa = k * delta;

		ub = i * delta;
		vb = (j+1) * delta;
		wb = k * delta;

		uc = i * delta;
		vc = j * delta;
		wc = (k+1) * delta;
	} else {
		// upper triangle
		ua = (i+1) * delta;
		va = j * delta;
		wa = (k+1) * delta;

		ub = (i+1) * delta;
		vb = (j+1) * delta;
		wb = k * delta;

		uc = i * delta;
		vc = (j+1) * delta;
		wc = (k+1) * delta;
	}

	// interpolated normal, only normal of c is actively used
	Vector na = Normalize(n1 * ua + n2 * va + n3 * wa);
	Vector nb = Normalize(n1 * ub + n2 * vb + n3 * wb);
	Vector nc = Normalize(n1 * uc + n2 * vc + n3 * wc);

	Point a, b, c; // vertices of generated microtriangle
	{
		// point in macrotriangle
		const Point pa = p1 * ua + p2 * va + p3 * wa;
		const Point pb = p1 * ub + p2 * vb + p3 * wb;
		const Point pc = p1 * uc + p2 * vc + p3 * wc;

		a = GetDisplacedP(pa, na, ua, va, wa);
		b = GetDisplacedP(pb, nb, ub, vb, wb);
		c = GetDisplacedP(pc, nc, uc, vc, wc);
	}

	if (enterSide < 0) {
		// ray enters through one of the caps, and exits through one side
		// check entry cell to figure out correct entry side

		float tt;
		float ttmin = 1e30;

		const Point p11(a + m * na);
		const Point p12(b + m * nb);
		const Point p13(a + M * na);

		if (intersectPlane(ray, p11, p12, p13, &tt)) {
			if (tt < ttmin) {
				ttmin = tt;
				enterSide = 1;
			}
		}

		const Point p21(b + m * nb);
		const Point p22(c + m * nc);
		const Point p23(b + M * nb);

		if (intersectPlane(ray, p21, p22, p23, &tt)) {
			if (tt < ttmin) {
				ttmin = tt;
				enterSide = 2;
			}
		}

		const Point p31(c + m * nc);
		const Point p32(a + m * na);
		const Point p33(c + M * nc);

		if (intersectPlane(ray, p31, p32, p33, &tt)) {
			if (tt < ttmin) {
				ttmin = tt;
				enterSide = 3;
			}
		}

		// if enterSide < 0 then we dont hit any walls
	}

	// Determine "change" variable based on which sides
	// the ray hits. If the ray doesn't hit any the start
	// cell will be the end cell as well.
	LastChange change = iminus;

	if (enterSide > 0) {
		// reorder vertices so that ab is the edge the ray entered through
		switch (enterSide) {
			case 1:
				if (i + j + k == N - 1)
					// lower triangle
					change = kplus;
				else
					change = iminus;
				break;
			case 2:
				swap(a, b);
				swap(na, nb);
				swap(ua, ub);
				swap(va, vb);
				swap(wa, wb);

				swap(b, c);
				swap(nb, nc);
				swap(ub, uc);
				swap(vb, vc);
				swap(wb, wc);

				if (i + j + k == N - 1)
					// lower triangle
					change = iplus;
				else
					change = jminus;
				break;
			case 3:
				swap(b, c);
				swap(nb, nc);
				swap(ub, uc);
				swap(vb, vc);
				swap(wb, wc);

				swap(a, b);
				swap(na, nb);
				swap(ua, ub);
				swap(va, vb);
				swap(wa, wb);

				if (i + j + k == N - 1)
					// lower triangle
					change = jplus;
				else
					change = kminus;
				break;
			default:
				// oh bugger
				BOOST_ASSERT(false);
				return false;
		}
	} else
		enterSide = -2;

	// traverse
	for (int iter = 0; iter <= 2 * N; ++iter) {
		const Vector e1(b - a);
		const Vector e2(c - a);

		float t;
		float b0, b1, b2;

		if (intersectTri(ray, a, e1, e2, &b0, &b1, &b2, &t)) {
			if ((t >= ray.mint) && (t <= ray.maxt)) {
				// interpolate position in microtriangle
				const Point pp(a * b0 + b * b1 + c * b2);

				// recover barycentric coordinates in macrotriangle
				const float v = va * b0 + vb * b1 + vc * b2;
				const float w = wa * b0 + wb * b1 + wc * b2;
				b0 = 1.f - v - w;
				b1 = v;
				b2 = w;

				Normal nn(Normalize(Cross(e1, e2)));
				Vector ts(Normalize(Cross(nn, dpdu)));
				Vector ss(Cross(ts, nn));
				// Lotus - the length of dpdu/dpdv can be important for bumpmapping
				ss *= dpdu.Length();
				if (Dot(dpdv, ts) < 0.f)
					ts *= -dpdv.Length();
				else
					ts *= dpdv.Length();

				// Interpolate $(u,v)$ triangle parametric coordinates
				const float tu = b0 * uvs[0][0] + b1 * uvs[1][0] + b2 * uvs[2][0];
				const float tv = b0 * uvs[0][1] + b1 * uvs[1][1] + b2 * uvs[2][1];

				isect->dg = DifferentialGeometry(pp, nn, ss, ts,
					Normal(0, 0, 0), Normal(0, 0, 0), tu, tv, this);

				isect->Set(mesh->WorldToObject, this, mesh->GetMaterial(),
					mesh->GetExterior(), mesh->GetInterior());
				isect->dg.iData.baryTriangle.coords[0] = b0;
				isect->dg.iData.baryTriangle.coords[1] = b1;
				isect->dg.iData.baryTriangle.coords[2] = b2;
				ray.maxt = t;

				return true;
			}
		}

		// check if we reached end cell
		if (i == ei && j == ej && k == ek) {
			return false;
		}

		const bool rightOfC = Dot(Cross(nc, (ray.o - c)), ray.d) > 0.f;
		
		if (rightOfC) {
			a = c;
			va = vc;
			wa = wc;
		} else {
			b = c;
			vb = vc;
			wb = wc;
		}

		// 5 = -1 mod 6
		change = (LastChange)((change + (rightOfC ? 1 : 5)) % 6);
	
		switch (change) {
			case iminus:
				if (--i < 0)
					return false;
				vc = (j + 1) * delta;
				wc = (k + 1) * delta;
				break;
			case iplus:
				if (++i >= N)
					return false;
				vc = j * delta;
				wc = k * delta;
				break;
			case jminus:
				if (--j < 0)
					return false;
				vc = j * delta;
				wc = (k + 1) * delta;
				break;
			case jplus:
				if (++j >= N)
					return false;
				vc = (j + 1) * delta;
				wc = k * delta;
				break;
			case kminus:
				if (--k < 0)
					return false;
				vc = (j + 1) * delta;
				wc = k * delta;
				break;
			case kplus:
				if (++k >= N)
					return false;
				vc = j * delta;
				wc = (k + 1) * delta;
				break;
			default:
				BOOST_ASSERT(false);
				// something has gone horribly wrong
				return false;
		}
		const float uc = (1.f - vc - wc);

		// point in macrotriangle
		const Point pc(p1 * uc + p2 * vc + p3 * wc);
		// interpolated normal
		nc = Normalize(n1 * uc + n2 * vc + n3 * wc);

		c = GetDisplacedP(pc, nc, uc, vc, wc);
	}

	// something went wrong
	return false;
}

bool MeshMicroDisplacementTriangle::IntersectP(const Ray &ray) const
{
	// TODO - optimized implementation

	Intersection isect;
	return Intersect(ray, &isect);
}

float MeshMicroDisplacementTriangle::Area() const
{
	// Get triangle vertices in _p1_, _p2_, and _p3_
	const Point &p1 = mesh->p[v[0]];
	const Point &p2 = mesh->p[v[1]];
	const Point &p3 = mesh->p[v[2]];
	return 0.5f * Cross(p2-p1, p3-p1).Length();
}

void MeshMicroDisplacementTriangle::Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const
{
	// TODO - compute proper derivatives

	float b1, b2;
	UniformSampleTriangle(u1, u2, &b1, &b2);
	// Get triangle vertices in _p1_, _p2_, and _p3_
	const Point &p1 = mesh->p[v[0]];
	const Point &p2 = mesh->p[v[1]];
	const Point &p3 = mesh->p[v[2]];
	float b3 = 1.f - b1 - b2;
	Point pp = b1 * p1 + b2 * p2 + b3 * p3;
	dg->nn = Normalize(Normal(Cross(p2-p1, p3-p1)));

	float uvs[3][2];
	GetUVs(uvs);
	// Compute deltas for triangle partial derivatives
	const float du1 = uvs[0][0] - uvs[2][0];
	const float du2 = uvs[1][0] - uvs[2][0];
	const float dv1 = uvs[0][1] - uvs[2][1];
	const float dv2 = uvs[1][1] - uvs[2][1];
	const Vector dp1 = p1 - p3, dp2 = p2 - p3;
	const float determinant = du1 * dv2 - dv1 * du2;
	if (determinant == 0.f) {
		// Handle 0 determinant for triangle partial derivative matrix
		CoordinateSystem(Vector(dg->nn), &dg->dpdu, &dg->dpdv);
	} else {
		const float invdet = 1.f / determinant;
		dg->dpdu = ( dv2 * dp1 - dv1 * dp2) * invdet;
		dg->dpdv = (-du2 * dp1 + du1 * dp2) * invdet;
	}

	// Interpolate $(u,v)$ triangle parametric coordinates
	dg->u = b1 * uvs[0][0] + b2 * uvs[1][0] + b3 * uvs[2][0];
	dg->v = b1 * uvs[0][1] + b2 * uvs[1][1] + b3 * uvs[2][1];

	// displace sampled point
	dg->p = GetDisplacedP(pp, Vector(dg->nn), dg->u, dg->v, 1.f - dg->u - dg->v);

	dg->iData.baryTriangle.coords[0] = b1;
	dg->iData.baryTriangle.coords[1] = b2;
	dg->iData.baryTriangle.coords[2] = b3;
}

void MeshMicroDisplacementTriangle::GetShadingGeometry(const Transform &obj2world,
	const DifferentialGeometry &dg, DifferentialGeometry *dgShading) const
{
	if (!mesh->displacementMapNormalSmooth) {
		*dgShading = dg;
		return;
	}

	const Point p(dg.iData.baryTriangle.coords[0] * mesh->p[v[0]] +
		dg.iData.baryTriangle.coords[1] * mesh->p[v[1]] +
		dg.iData.baryTriangle.coords[2] * mesh->p[v[2]]);
	// Use _n_ to compute shading tangents for triangle, _ss_ and _ts_
	const Normal ns(Normalize(dg.iData.baryTriangle.coords[0] * GetN(0) +
		dg.iData.baryTriangle.coords[1] * GetN(1) +
		dg.iData.baryTriangle.coords[2] * GetN(2)));

	Vector ts(Normalize(Cross(ns, dpdu)));
	Vector ss(Cross(ts, ns));
	// Lotus - the length of dpdu/dpdv can be important for bumpmapping
	ss *= dg.dpdu.Length();
	if (Dot(dpdv, ts) < 0.f)
		ts *= -dg.dpdv.Length();
	else
		ts *= dg.dpdv.Length();

	Normal dndu, dndv;
	// Compute \dndu and \dndv for triangle shading geometry

	// Compute deltas for triangle partial derivatives of normal
	const float du1 = uvs[0][0] - uvs[2][0];
	const float du2 = uvs[1][0] - uvs[2][0];
	const float dv1 = uvs[0][1] - uvs[2][1];
	const float dv2 = uvs[1][1] - uvs[2][1];
	const Normal dn1 = mesh->n[v[0]] - mesh->n[v[2]];
	const Normal dn2 = mesh->n[v[1]] - mesh->n[v[2]];
	const float determinant = du1 * dv2 - dv1 * du2;

	if (determinant == 0.f)
		dndu = dndv = Normal(0, 0, 0);
	else {
		const float invdet = 1.f / determinant;
		dndu = ( dv2 * dn1 - dv1 * dn2) * invdet;
		dndv = (-du2 * dn1 + du1 * dn2) * invdet;
	}

	*dgShading = DifferentialGeometry(p, ns, ss, ts,
		dndu, dndv, dg.u, dg.v, this);
	float dddu, dddv;
	SpectrumWavelengths sw;
	mesh->displacementMap->GetDuv(sw, *dgShading, 0.001f, &dddu, &dddv);
	dgShading->p = dg.p;
	dgShading->dpdu = ss + dddu * Vector(ns);
	dgShading->dpdv = ts + dddv * Vector(ns);
	dgShading->nn = Normal(Normalize(Cross(dgShading->dpdu, dgShading->dpdv)));
	// The above transform keeps the normal in the original normal
	// hemisphere. If they are opposed, it means UVN was indirect and
	// the normal needs to be reversed
	if (Dot(ns, dgShading->nn) < 0.f)
		dgShading->nn = -dgShading->nn;
}
