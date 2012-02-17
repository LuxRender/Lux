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

MeshWaldTriangle::MeshWaldTriangle(const Mesh *m, u_int n)
	: MeshBaryTriangle(m, n)
{
	// Reorder vertices so that edges lengths will be as close as possible
	const float l0 = DistanceSquared(mesh->p[v[0]], mesh->p[v[1]]);
	const float l1 = DistanceSquared(mesh->p[v[1]], mesh->p[v[2]]);
	const float l2 = DistanceSquared(mesh->p[v[2]], mesh->p[v[0]]);
	const float d0 = fabsf(l0 - l2);
	const float d1 = fabsf(l1 - l0);
	const float d2 = fabsf(l2 - l1);
	int *v_ = const_cast<int *>(v);
	if (d2 < d1 && d2 < d0) {
		swap(v_[0], v_[2]);
		swap(v_[1], v_[2]);
	} else if (d1 < d0) {
		swap(v_[0], v_[1]);
		swap(v_[2], v_[1]);
	}

	// Wald's precomputed values

	// Look for the dominant axis
	const Point &v0 = mesh->p[v[0]];
	const Point &v1 = mesh->p[v[1]];
	const Point &v2 = mesh->p[v[2]];
	Vector e1 = v1 - v0;
	Vector e2 = v2 - v0;

	normalizedNormal = Normal(Normalize(Cross(e1, e2)));
	// Dade - check for degenerate triangle
	if (isnan(normalizedNormal.x) || isnan(normalizedNormal.y) ||
		isnan(normalizedNormal.z)) {
		intersectionType = DEGENERATE;
		return;
	}

	// Define the type of intersection to use according the normal
	// of the triangle

	if ((fabs(normalizedNormal.x) > fabs(normalizedNormal.y)) &&
		(fabs(normalizedNormal.x) > fabs(normalizedNormal.z)))
		intersectionType = DOMINANT_X;
	else if (fabs(normalizedNormal.y) > fabs(normalizedNormal.z))
		intersectionType = DOMINANT_Y;
	else
		intersectionType = DOMINANT_Z;

	float ax, ay, bx, by, cx, cy;
	switch (intersectionType) {
		case DOMINANT_X: {
			const float invNormal = 1.f / normalizedNormal.x;
			nu = normalizedNormal.y * invNormal;
			nv = normalizedNormal.z * invNormal;
			nd = v0.x + nu * v0.y + nv * v0.z;
			ax = v0.y;
			ay = v0.z;
			bx = v2.y - ax;
			by = v2.z - ay;
			cx = v1.y - ax;
			cy = v1.z - ay;
			break;
		}
		case DOMINANT_Y: {
			const float invNormal = 1.f / normalizedNormal.y;
			nu = normalizedNormal.z * invNormal;
			nv = normalizedNormal.x * invNormal;
			nd = nv * v0.x + v0.y + nu * v0.z;
			ax = v0.z;
			ay = v0.x;
			bx = v2.z - ax;
			by = v2.x - ay;
			cx = v1.z - ax;
			cy = v1.x - ay;
			break;
		}
		case DOMINANT_Z: {
			const float invNormal = 1.f / normalizedNormal.z;
			nu = normalizedNormal.x * invNormal;
			nv = normalizedNormal.y * invNormal;
			nd = nu * v0.x + nv * v0.y + v0.z;
			ax = v0.x;
			ay = v0.y;
			bx = v2.x - ax;
			by = v2.y - ay;
			cx = v1.x - ax;
			cy = v1.y - ay;
			break;
		}
		default:
			BOOST_ASSERT(false);
			// Dade - how can I report internal errors ?
			return;
	}

	float det = bx * cy - by * cx;
	float invDet = 1.f / det;

	bnu = -by * invDet;
	bnv = bx * invDet;
	bnd = (by * ax - bx * ay) * invDet;
	cnu = cy * invDet;
	cnv = -cx * invDet;
	cnd = (cx * ay - cy * ax) * invDet;

	// Dade - doing some precomputation for filling the DifferentialGeometry
	// in the intersection method

	// Compute triangle partial derivatives
	float uvs[3][2];
	GetUVs(uvs);
	// Compute deltas for triangle partial derivatives
	const float du1 = uvs[0][0] - uvs[2][0];
	const float du2 = uvs[1][0] - uvs[2][0];
	const float dv1 = uvs[0][1] - uvs[2][1];
	const float dv2 = uvs[1][1] - uvs[2][1];
	const Vector dp1 = v0 - v2, dp2 = v1 - v2;
	const float determinant = du1 * dv2 - dv1 * du2;
	if (determinant == 0.f) {
		// Handle zero determinant for triangle partial derivative matrix
		CoordinateSystem(Vector(normalizedNormal), &dpdu, &dpdv);
	} else {
		const float invdet = 1.f / determinant;
		dpdu = ( dv2 * dp1 - dv1 * dp2) * invdet;
		dpdv = (-du2 * dp1 + du1 * dp2) * invdet;
	}
}

bool MeshWaldTriangle::Intersect(const Ray &ray, Intersection *isect, bool null_shp_isect) const
{
	//look if shape is a null type
	if (null_shp_isect && mesh->support) return false;

	float o0, o1, o2, d0, d1, d2;
	switch (intersectionType) {
		case DOMINANT_X: {
			o0 = ray.o.x;
			o1 = ray.o.y;
			o2 = ray.o.z;
			d0 = ray.d.x;
			d1 = ray.d.y;
			d2 = ray.d.z;
			break;
		}
		case DOMINANT_Y: {
			o0 = ray.o.y;
			o1 = ray.o.z;
			o2 = ray.o.x;
			d0 = ray.d.y;
			d1 = ray.d.z;
			d2 = ray.d.x;
			break;
		}
		case DOMINANT_Z: {
			o0 = ray.o.z;
			o1 = ray.o.x;
			o2 = ray.o.y;
			d0 = ray.d.z;
			d1 = ray.d.x;
			d2 = ray.d.y;
			break;
		}
		default:
			return false;
	}
	const float det = d0 + nu * d1 + nv * d2;
	if (det == 0.f)
		return false;

	const float t = nd - o0 - nu * o1 - nv * o2;
	if (det > 0.f) {
		if (t <= det * ray.mint || t >= det * ray.maxt)
			return false;
	} else {
		if (t >= det * ray.mint || t <= det * ray.maxt)
			return false;
	}

	const float hu = det * o1 + t * d1;
	const float hv = det * o2 + t * d2;
	const float uu = (hu * bnu + hv * bnv) / det + bnd;
	if (uu < 0.f)
		return false;

	const float vv = (hu * cnu + hv * cnv) / det + cnd;
	if (vv < 0.f)
                return false;

	const float b0 = 1.f - uu - vv;
	if (b0 < 0.f)
		return false;

	const float tt = t / det;

	float uvs[3][2];
	GetUVs(uvs);
	// Interpolate $(u,v)$ triangle parametric coordinates

	float tu_ = b0 * uvs[0][0] + uu * uvs[1][0] + vv * uvs[2][0];
	float tv_ = b0 * uvs[0][1] + uu * uvs[1][1] + vv * uvs[2][1];

	const Point pp(b0 * mesh->p[v[0]] + uu * mesh->p[v[1]] + vv * mesh->p[v[2]]);

	if (mesh->proj_text){
		Point end = pp;
		Vector wh = Normalize(end-mesh->cam);

		tu_ = SphericalPhi(wh) ;
		tv_ = SphericalTheta(wh) ;
	}
	const float tu = tu_;
	const float tv = tv_;

	isect->dg = DifferentialGeometry(pp, normalizedNormal, dpdu, dpdv,
		Normal(0, 0, 0), Normal(0, 0, 0), tu, tv, this);
	isect->Set(mesh->WorldToObject, this, mesh->GetMaterial(),
		mesh->GetExterior(), mesh->GetInterior());
	isect->dg.iData.baryTriangle.coords[0] = b0;
	isect->dg.iData.baryTriangle.coords[1] = uu;
	isect->dg.iData.baryTriangle.coords[2] = vv;
	ray.maxt = tt;

	return true;
}

bool MeshWaldTriangle::IntersectP(const Ray &ray, bool null_shp_isect) const
{
	float o0, o1, o2, d0, d1, d2;
	switch (intersectionType) {
		case DOMINANT_X: {
			o0 = ray.o.x;
			o1 = ray.o.y;
			o2 = ray.o.z;
			d0 = ray.d.x;
			d1 = ray.d.y;
			d2 = ray.d.z;
			break;
		}
		case DOMINANT_Y: {
			o0 = ray.o.y;
			o1 = ray.o.z;
			o2 = ray.o.x;
			d0 = ray.d.y;
			d1 = ray.d.z;
			d2 = ray.d.x;
			break;
		}
		case DOMINANT_Z: {
			o0 = ray.o.z;
			o1 = ray.o.x;
			o2 = ray.o.y;
			d0 = ray.d.z;
			d1 = ray.d.x;
			d2 = ray.d.y;
			break;
		}
		default:
			return false;
	}
	const float det = d0 + nu * d1 + nv * d2;
	if (det == 0.f)
		return false;

	const float t = nd - o0 - nu * o1 - nv * o2;
	if (det > 0.f) {
		if (t <= det * ray.mint || t >= det * ray.maxt)
			return false;
	} else {
		if (t >= det * ray.mint || t <= det * ray.maxt)
			return false;
	}

	const float hu = det * o1 + t * d1;
	const float hv = det * o2 + t * d2;
	const float uu = (hu * bnu + hv * bnv) / det + bnd;
	if (uu < 0.f)
		return false;

	const float vv = (hu * cnu + hv * cnv) / det + cnd;
	if (vv < 0.f || uu + vv > 1.f)
                return false;

	return true;
}

float MeshWaldTriangle::Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const
{
	float b1, b2;
	UniformSampleTriangle(u1, u2, &b1, &b2);
	// Get triangle vertices in _p1_, _p2_, and _p3_
	const Point &p1 = mesh->p[v[0]];
	const Point &p2 = mesh->p[v[1]];
	const Point &p3 = mesh->p[v[2]];
	const float b3 = 1.f - b1 - b2;
	dg->p = b1 * p1 + b2 * p2 + b3 * p3;

	dg->nn = normalizedNormal;
	dg->dpdu = dpdu;
	dg->dpdv = dpdv;
	dg->dndu = dg->dndv = Normal(0, 0, 0);

	float uv[3][2];
	GetUVs(uv);
	dg->u = b1 * uv[0][0] + b2 * uv[1][0] + b3 * uv[2][0];
	dg->v = b1 * uv[0][1] + b2 * uv[1][1] + b3 * uv[2][1];

	dg->handle = this;

	dg->iData.baryTriangle.coords[0] = b1;
	dg->iData.baryTriangle.coords[1] = b2;
	dg->iData.baryTriangle.coords[2] = b3;
	return Pdf(*dg);
}

bool MeshWaldTriangle::isDegenerate() const {
	return intersectionType == DEGENERATE;
}
