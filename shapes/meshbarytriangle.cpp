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

#include "mesh.h"
#include "mc.h"

using namespace lux;

MeshBaryTriangle::MeshBaryTriangle(const Mesh *m, u_int n) :
	mesh(m), v(&(mesh->triVertexIndex[3 * n])), is_Degenerate(false)
{
	int *v_ = const_cast<int *>(v);
	if (m->reverseOrientation ^ m->transformSwapsHandedness)
		swap(v_[1], v_[2]);

	const Point &v0 = m->p[v[0]];
	const Point &v1 = m->p[v[1]];
	const Point &v2 = m->p[v[2]];
	Vector e1 = v1 - v0;
	Vector e2 = v2 - v0;

	Normal normalizedNormal(Normalize(Cross(e1, e2)));

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
				m->inconsistentShadingTris++;
			}
		} else if (cos0 > 0.f) {
			if (!(Dot(normalizedNormal, m->n[v[1]]) > 0.f &&
				Dot(normalizedNormal, m->n[v[2]]) > 0.f)) {
				m->inconsistentShadingTris++;
			}
		}
	}
}

BBox MeshBaryTriangle::ObjectBound() const
{
	// Get triangle vertices in _p1_, _p2_, and _p3_
	const Point &p1 = mesh->p[v[0]];
	const Point &p2 = mesh->p[v[1]];
	const Point &p3 = mesh->p[v[2]];
	return Union(BBox(mesh->WorldToObject(p1), mesh->WorldToObject(p2)),
		mesh->WorldToObject(p3));
}

BBox MeshBaryTriangle::WorldBound() const
{
	// Get triangle vertices in _p1_, _p2_, and _p3_
	const Point &p1 = mesh->p[v[0]];
	const Point &p2 = mesh->p[v[1]];
	const Point &p3 = mesh->p[v[2]];
	return Union(BBox(p1, p2), p3);
}

bool MeshBaryTriangle::Intersect(const Ray &ray, Intersection* isect) const
{
	Vector e1, e2, s1;
	// Compute $\VEC{s}_1$
	// Get triangle vertices in _p1_, _p2_, and _p3_
	const Point &p1 = mesh->p[v[0]];
	const Point &p2 = mesh->p[v[1]];
	const Point &p3 = mesh->p[v[2]];
	e1 = p2 - p1;
	e2 = p3 - p1;
	s1 = Cross(ray.d, e2);
	const float divisor = Dot(s1, e1);
	if (divisor == 0.f)
		return false;
	const float invDivisor = 1.f / divisor;
	// Compute first barycentric coordinate
	const Vector d = ray.o - p1;
	const float b1 = Dot(d, s1) * invDivisor;
	if (b1 < 0.f)
		return false;
	// Compute second barycentric coordinate
	const Vector s2 = Cross(d, e1);
	const float b2 = Dot(ray.d, s2) * invDivisor;
	if (b2 < 0.f)
		return false;
	const float b0 = 1.f - b1 - b2;
	if (b0 < 0.f)
		return false;
	// Compute _t_ to intersection point
	const float t = Dot(e2, s2) * invDivisor;
	if (t < ray.mint || t > ray.maxt)
		return false;

	// Fill in _DifferentialGeometry_ from triangle hit
	// Compute triangle partial derivatives
	Vector dpdu, dpdv;
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
		CoordinateSystem(Normalize(Cross(e1, e2)), &dpdu, &dpdv);
	} else {
		const float invdet = 1.f / determinant;
		dpdu = ( dv2 * dp1 - dv1 * dp2) * invdet;
		dpdv = (-du2 * dp1 + du1 * dp2) * invdet;
	}

	// Interpolate $(u,v)$ triangle parametric coordinates
	const float tu = b0 * uvs[0][0] + b1 * uvs[1][0] + b2 * uvs[2][0];
	const float tv = b0 * uvs[0][1] + b1 * uvs[1][1] + b2 * uvs[2][1];

	const Normal nn = Normal(Normalize(Cross(e1, e2)));
	const Point pp(p1 + b1 * e1 + b2 * e2);

	isect->dg = DifferentialGeometry(pp, nn, dpdu, dpdv,
		Normal(0, 0, 0), Normal(0, 0, 0), tu, tv, this);

	isect->Set(mesh->WorldToObject, this, mesh->GetMaterial(),
		mesh->GetExterior(), mesh->GetInterior());
	isect->dg.iData.baryTriangle.coords[0] = b0;
	isect->dg.iData.baryTriangle.coords[1] = b1;
	isect->dg.iData.baryTriangle.coords[2] = b2;
	ray.maxt = t;

	return true;
}

bool MeshBaryTriangle::IntersectP(const Ray &ray) const
{
	// Compute $\VEC{s}_1$
	// Get triangle vertices in _p1_, _p2_, and _p3_
	const Point &p1 = mesh->p[v[0]];
	const Point &p2 = mesh->p[v[1]];
	const Point &p3 = mesh->p[v[2]];
	Vector e1 = p2 - p1;
	Vector e2 = p3 - p1;
	Vector s1 = Cross(ray.d, e2);
	const float divisor = Dot(s1, e1);
	if (divisor == 0.f)
		return false;
	const float invDivisor = 1.f / divisor;
	// Compute first barycentric coordinate
	const Vector d = ray.o - p1;
	const float b1 = Dot(d, s1) * invDivisor;
	if (b1 < 0.f)
		return false;
	// Compute second barycentric coordinate
	const Vector s2 = Cross(d, e1);
	const float b2 = Dot(ray.d, s2) * invDivisor;
	if (b2 < 0.f)
		return false;
	if (b1 + b2 > 1.f)
		return false;
	// Compute _t_ to intersection point
	float t = Dot(e2, s2) * invDivisor;
	if (t < ray.mint || t > ray.maxt)
		return false;

	return true;
}

float MeshBaryTriangle::Area() const
{
	// Get triangle vertices in _p1_, _p2_, and _p3_
	const Point &p1 = mesh->p[v[0]];
	const Point &p2 = mesh->p[v[1]];
	const Point &p3 = mesh->p[v[2]];
	return 0.5f * Cross(p2-p1, p3-p1).Length();
}

void MeshBaryTriangle::Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const
{
	float b1, b2;
	UniformSampleTriangle(u1, u2, &b1, &b2);
	// Get triangle vertices in _p1_, _p2_, and _p3_
	const Point &p1 = mesh->p[v[0]];
	const Point &p2 = mesh->p[v[1]];
	const Point &p3 = mesh->p[v[2]];
	float b3 = 1.f - b1 - b2;
	dg->p = b1 * p1 + b2 * p2 + b3 * p3;
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
	dg->dndu = dg->dndv = Normal(0, 0, 0);

	// Interpolate $(u,v)$ triangle parametric coordinates
	dg->u = b1 * uvs[0][0] + b2 * uvs[1][0] + b3 * uvs[2][0];
	dg->v = b1 * uvs[0][1] + b2 * uvs[1][1] + b3 * uvs[2][1];

	dg->handle = this;

	dg->iData.baryTriangle.coords[0] = b1;
	dg->iData.baryTriangle.coords[1] = b2;
	dg->iData.baryTriangle.coords[2] = b3;
}

void MeshBaryTriangle::GetShadingGeometry(const Transform &obj2world,
	const DifferentialGeometry &dg, DifferentialGeometry *dgShading) const
{
	if (!mesh->n) {
		*dgShading = dg;
		return;
	}

	// Use _n_ to compute shading tangents for triangle, _ss_ and _ts_
	const Normal ns = Normalize(dg.iData.baryTriangle.coords[0] * mesh->n[v[0]] +
		dg.iData.baryTriangle.coords[1] * mesh->n[v[1]] + dg.iData.baryTriangle.coords[2] * mesh->n[v[2]]);

	Vector ts(Normalize(Cross(ns, dg.dpdu)));
	Vector ss(Cross(ts, ns));
	// Lotus - the length of dpdu/dpdv can be important for bumpmapping
	ss *= dg.dpdu.Length();
	if (Dot(dg.dpdv, ts) < 0.f)
		ts *= -dg.dpdv.Length();
	else
		ts *= dg.dpdv.Length();

	Normal dndu, dndv;
	// Compute \dndu and \dndv for triangle shading geometry
	float uvs[3][2];
	GetUVs(uvs);

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

	*dgShading = DifferentialGeometry(dg.p, ns, ss, ts,
		dndu, dndv, dg.u, dg.v, this);
}
