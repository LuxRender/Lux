/***************************************************************************
 *   Copyright (C) 1998-2011 by authors (see AUTHORS.txt )                 *
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

// Bilinear patch class
// created by Shaun David Ramsey and Kristin Potter copyright (c) 2003
// email ramsey()cs.utah.edu with any quesitons
// modified by Asjørn Heid 2011
/*
This copyright notice is available at:
http://www.opensource.org/licenses/mit-license.php

Copyright (c) 2003 Shaun David Ramsey, Kristin Potter, Charles Hansen

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sel copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.
*/
class BilinearPatch
{
public:
	// Constructors
	BilinearPatch(const Point &p00, const Point &p01, const Point &p10, const Point &p11) : P00(p00), P01(p01), P10(p10), P11(p11) {}
	~BilinearPatch() { }
	// Find the tangent (du)
	Vector TanU(const float v) const;
	// Find the tangent (dv)
	Vector TanV(const float u) const;
	// Find dudv
	Normal N(const float u, const float v) const;
	// Evaluate the surface of the patch at u,v
	Point P(const float u, const float v) const;
	// Find the local closest point to spacept
	bool RayPatchIntersection(const Ray &ray, float *u, float *v, float *t) const;

	// The four points defining the patch
	Point P00, P01, P10, P11;
};
// end Bilinear patch class

MeshMicroDisplacementTriangle::MeshMicroDisplacementTriangle(const Mesh *m, u_int n) :
	mesh(m), v(&(mesh->triVertexIndex[3 * n])), is_Degenerate(false)
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

	if (isnan(normalizedNormal.x) || 
		isnan(normalizedNormal.y) ||
		isnan(normalizedNormal.z))
	{
		is_Degenerate = true;
		return;
	}

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

	// since texture output is clamped, restrict min/max
	const float M = mesh->displacementMapOffset + min(mesh->displacementMapMax, 1.f) * mesh->displacementMapScale;
	const float m = mesh->displacementMapOffset + max(mesh->displacementMapMin, -1.f) * mesh->displacementMapScale;

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

Point MeshMicroDisplacementTriangle::GetDisplacedP(const Point &pbase, const Vector &n, const float u, const float v, const float w) const
{
		const float tu = u * uvs[0][0] + v * uvs[1][0] + w * uvs[2][0];
		const float tv = u * uvs[0][1] + v * uvs[1][1] + w * uvs[2][1];

		const DifferentialGeometry dg(pbase, Normal(n), dpdu, dpdv,
			Normal(0, 0, 0), Normal(0, 0, 0), tu, tv, this);

		const SpectrumWavelengths sw;

		Vector displacement(n);
		displacement *=	(
			Clamp(mesh->displacementMap->Evaluate(sw, dg), -1.f, 1.f) * mesh->displacementMapScale +
			mesh->displacementMapOffset);

		return pbase + displacement;
}

static bool intersectPlane(const Ray &ray, const Point &p, const Vector &n, float *t)
{
	const float num = Dot(n, ray.d);

	if (fabsf(num) < MachineEpsilon::E(fabsf(num)))
		return false;

	*t = Dot(n, p - ray.o) / num;

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
	// since texture output is clamped, restrict min/max
	const float M = mesh->displacementMapOffset + min(mesh->displacementMapMax, 1.f) * mesh->displacementMapScale;
	const float m = mesh->displacementMapOffset + max(mesh->displacementMapMin, -1.f) * mesh->displacementMapScale;

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

		float t, u, v;

		const BilinearPatch bip(p00, p01, p10, p11);
		
		// currently ignores potential second hit
		// ideally marching should be restarted with second hit if
		// first hit misses
		if (bip.RayPatchIntersection(ray, &u, &v, &t)) {
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

		float t, u, v;

		const BilinearPatch bip(p00, p01, p10, p11);

		if (bip.RayPatchIntersection(ray, &u, &v, &t)) {
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

		float t, u, v;

		const BilinearPatch bip(p00, p01, p10, p11);

		if (bip.RayPatchIntersection(ray, &u, &v, &t)) {
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

		if (enterSide < 0) {
			// ray enters through one of the caps, and possibly exits through one side
			// check entry cell to figure out correct entry side

			float tt;
			float ttmin = 1e30;


			// TODO - improve
			const Vector pn12(Cross(pb - pa, normalizedNormal));

			if (intersectPlane(ray, pa, pn12, &tt)) {
				if (tt < ttmin) {
					ttmin = tt;
					enterSide = 1;
				}
			}

			const Vector pn23(Cross(pc - pb, normalizedNormal));

			if (intersectPlane(ray, pb, pn23, &tt)) {
				if (tt < ttmin) {
					ttmin = tt;
					enterSide = 2;
				}
			}

			const Vector pn31(Cross(pa - pc, normalizedNormal));

			if (intersectPlane(ray, pc, pn31, &tt)) {
				if (tt < ttmin) {
					ttmin = tt;
					enterSide = 3;
				}
			}

			// if enterSide < 0 then we dont hit any walls
		}
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
			if (t >= ray.mint && t <= ray.maxt) {
				// interpolate microtriangle even if it is very small
				// otherwise selfshadowing will occur
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
		if (i == ei && j == ej && k == ek)
			return false;

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

float MeshMicroDisplacementTriangle::Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const
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
	return Pdf(*dg);
}

void MeshMicroDisplacementTriangle::GetShadingGeometry(const Transform &obj2world,
	const DifferentialGeometry &dg, DifferentialGeometry *dgShading) const
{
	if (!mesh->displacementMapNormalSmooth || !mesh->n) {
		*dgShading = dg;
		return;
	}

	const Point p(dg.iData.baryTriangle.coords[0] * mesh->p[v[0]] +
		dg.iData.baryTriangle.coords[1] * mesh->p[v[1]] +
		dg.iData.baryTriangle.coords[2] * mesh->p[v[2]]);
	// Use _n_ to compute shading tangents for triangle, _ss_ and _ts_
	const Normal ns = Normalize(dg.iData.baryTriangle.coords[0] * mesh->n[v[0]] +
		dg.iData.baryTriangle.coords[1] * mesh->n[v[1]] + dg.iData.baryTriangle.coords[2] * mesh->n[v[2]]);

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


// Bilinear patch implementation, see copyright information above
// x,y,z position of a point at params u and v
Point BilinearPatch::P(const float u, const float v) const
{
	return 
		((1.f - u) * (1.f - v)) * P00 +
		((1.f - u) *        v)  * P01 + 
		(       u  * (1.f - v)) * P10 +
		(       u  *        v)  * P11;
}

// Find tangent (du)
Vector BilinearPatch::TanU(const float v) const
{
  return (1.f - v) * (P10 - P00) + v * (P11 - P01);
}

// Find tanget (dv)
Vector BilinearPatch::TanV(const float u) const
{
  return (1.f - u) * (P01 - P00) + u * (P11 - P10);
}


// Find the normal of the patch
Normal BilinearPatch::N(const float u, const float v) const
{
  return Normalize(Normal(Cross(TanU(v), TanV(u))));
}
  
//choose between the best denominator to avoid singularities
//and to get the most accurate root possible
static float getu(const float v, const float M1, const float M2, 
				  const float J1, const float J2, 
				  const float K1, const float K2, 
				  const float R1, const float R2)
{

	const float denom = v * (M1-M2) + J1 - J2;
	const float d2 = v * M1 + J1;
	if(fabsf(denom) > fabsf(d2)) // which denominator is bigger
		return (v * (K2 - K1) + R2 - R1) / denom;
	return -(v * K1 + R1) / d2;
}

// compute t with the best accuracy by using the component
// of the direction that is largest
static float computet(const Ray &ray, const Point &srfpos)
{
	const float dx = fabsf(ray.d.x);
	const float dy = fabsf(ray.d.y);
	const float dz = fabsf(ray.d.z);
	// if x is bigger than y and z
	if(dx >= dy && dx >= dz)
		return (srfpos.x - ray.o.x) / ray.d.x;
	// if y is bigger than x and z
	else if(dy >= dz)
		return (srfpos.y - ray.o.y) / ray.d.y;
	// otherwise x isn't bigger than both and y isn't bigger than both
	else
		return (srfpos.z - ray.o.z) / ray.d.z;    
}

// returns roots in u[2] between min and max
static u_int QuadraticRoot(float a, float b, float c, 
		   float min, float max, float *u)
{
	float t0, t1;
	if (!Quadratic(a, b, c, &t0, &t1))
		return 0;

	u_int sol = 0;
	if (t0 > min && t0 < max)
		u[sol++] = t0;
	if (t1 > min && t1 < max)
		u[sol++] = t1;
	return sol;
}

//             RayPatchIntersection
// intersect rays of the form p = o + t d where t is the parameter
// to solve for. 
// return true to this function
// for invalid intersections - simply return false uv values can be 
// anything
#define ray_epsilon 1e-9
bool BilinearPatch::RayPatchIntersection(const Ray &ray, float *u, float *v, float *t) const
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Equation of the patch:
	// P(u, v) = (1-u)(1-v)P00 + (1-u)vP01 + u(1-v)P10 + uvP11
	// Equation of the ray:
	// R(t) = r + tq
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Variables for substitition
	// a = P11 - P10 - P01 + P00
	// b = P10 - P00
	// c = P01 - P00
	// d = P00  (d is shown below in the #ifdef raypatch area)
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~  

	// Find a w.r.t. x, y, z
	const float ax = P11.x - P10.x - P01.x + P00.x;
	const float ay = P11.y - P10.y - P01.y + P00.y;
	const float az = P11.z - P10.z - P01.z + P00.z;

	// Find A1 and A2
	const float A1 = ax * ray.d.z - az * ray.d.x;
	const float A2 = ay * ray.d.z - az * ray.d.y;

	// Find b w.r.t. x, y, z
	const float bx = P10.x - P00.x;
	const float by = P10.y - P00.y;
	const float bz = P10.z - P00.z;

	// Find B1 and B2
	const float B1 = bx * ray.d.z - bz * ray.d.x;
	const float B2 = by * ray.d.z - bz * ray.d.y;

	// Find c w.r.t. x, y, z
	const float cx = P01.x - P00.x;
	const float cy = P01.y - P00.y;
	const float cz = P01.z - P00.z;

	// Find C1 and C2
	const float C1 = cx * ray.d.z - cz * ray.d.x;
	const float C2 = cy * ray.d.z - cz * ray.d.y;

	// Find d w.r.t. x, y, z - subtracting r just after  
	const float dx = P00.x - ray.o.x;
	const float dy = P00.y - ray.o.y;
	const float dz = P00.z - ray.o.z;
  
	// Find D1 and D2
	const float D1 = dx * ray.d.z - dz * ray.d.x;
	const float D2 = dy * ray.d.z - dz * ray.d.y;


	const float A = A2*C1 - A1*C2;
	const float B = A2*D1 - A1*D2 + B2*C1 -B1*C2;
	const float C = B2*D1 - B1*D2;

	float vsol[2]; // the two roots from quadraticroot
	u_int num_sol = QuadraticRoot(A, B, C, -ray_epsilon, 1.f + ray_epsilon, vsol);

	switch(num_sol)
	{
		case 0: {
			return false; // no solutions found
		}
		case 1: {
			*v =  vsol[0];
			*u = getu(vsol[0], A2, A1, B2, B1, C2, C1, D2, D1);
			const Point pos1 = P(*u, *v);
			*t = computet(ray, pos1);
			if (*u < 1.f + ray_epsilon && *u > -ray_epsilon && *t > 0.f)//vars okay?
				return true;
			else
				return false; // no other soln - so ret false
		}
		case 2: { // two solutions found
			*v = vsol[0];
			*u = getu(vsol[0], A2, A1, B2, B1, C2, C1, D2, D1);
			const Point pos1 = P(*u, *v);
			*t = computet(ray, pos1);
			if(*u < 1.f + ray_epsilon && *u > -ray_epsilon && *t > 0.f) {
				const float tu = getu(vsol[1], A2, A1, B2, B1, C2, C1, D2, D1);

				if (tu < 1.f + ray_epsilon && tu > ray_epsilon) {
					const Point pos2 = P(tu, vsol[1]);
					const float t2 = computet(ray, pos2);
					if (t2 < 0.f || *t < t2) // t2 is bad or t1 is better
						return true;
					// other wise both t2 > 0 and t2 < t1
					*v = vsol[1];
					*u = tu;
					*t = t2;
					return true;
				}
				return true; // u2 is bad but u1 vars are still okay
			}
			else // doesn't fit in the root - try other one
			{
				*v = vsol[1];
				*u = getu(vsol[1], A2, A1, B2, B1, C2, C1, D2, D1);
				const Point pos1 = P(*u, *v);
				*t = computet(ray, pos1);
				if (*u < 1.f + ray_epsilon && *u > -ray_epsilon && *t > 0.f)
					return true;
				else
					return false;
			}
			break;
		}
	}
 
  BOOST_ASSERT(false);
  return false; 
}
