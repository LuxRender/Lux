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
#include "mc.h"

using namespace lux;

MeshWaldTriangle::MeshWaldTriangle(const Mesh *m, int n)
	: mesh(m), v(&(mesh->triVertexIndex[3 * n]))
{
    // Wald's precomputed values

    // Look for the dominant axis
    const Point &v0 = mesh->p[v[0]];
    const Point &v1 = mesh->p[v[1]];
    const Point &v2 = mesh->p[v[2]];
    Vector e1 = v1 - v0;
    Vector e2 = v2 - v0;

    Vector normal = Normalize(Cross(e1, e2));
	// Dade - check for degenerate triangle
	if (isnan(normal.x) || isnan(normal.y) || isnan(normal.z)) {
		intersectionType = DEGENERATE;
		return;
	}

	// Define the type of intersection to use according the normal
    // of the triangle

    if ((normal.y == 0.0f) && (normal.z == 0.0f))
        intersectionType = ORTHOGONAL_X;
    else if((normal.x == 0.0f) &&  (normal.z == 0.0f))
        intersectionType = ORTHOGONAL_Y;
    else if((normal.x == 0.0f) && (normal.y == 0.0f))
        intersectionType = ORTHOGONAL_Z;
    else if ((fabs(normal.x) > fabs(normal.y)) && (fabs(normal.x) > fabs(normal.z)))
        intersectionType = DOMINANT_X;
    else if (fabs(normal.y) > fabs(normal.z))
        intersectionType = DOMINANT_Y;
    else
        intersectionType = DOMINANT_Z;

    float ax, ay, bx, by, cx, cy;
    switch (intersectionType) {
        case DOMINANT_X: {
            const float invNormal = 1.0f / normal.x;
            nu = normal.y * invNormal;
            nv = normal.z * invNormal;
            nd = v0.x + nu*v0.y + nv * v0.z;
            ax = v0.y;
            ay = v0.z;
            bx = v2.y - ax;
            by = v2.z - ay;
            cx = v1.y - ax;
            cy = v1.z - ay;
            break;
        }
        case DOMINANT_Y: {
            const float invNormal = 1.0f / normal.y;
            nu = normal.z * invNormal;
            nv = normal.x * invNormal;
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
            const float invNormal = 1.0f / normal.z;
            nu = normal.x * invNormal;
            nv = normal.y * invNormal;
            nd = nu * v0.x + nv*v0.y + v0.z;
            ax = v0.x;
            ay = v0.y;
            bx = v2.x - ax;
            by = v2.y - ay;
            cx = v1.x - ax;
            cy = v1.y - ay;
            break;
        }
        case ORTHOGONAL_X:
            nu = 0.0f;
            nv = 0.0f;
            nd = v0.x;
            ax = v0.y;
            ay = v0.z;
            bx = v2.y - ax;
            by = v2.z - ay;
            cx = v1.y - ax;
            cy = v1.z - ay;
            break;
        case ORTHOGONAL_Y:
            nu = 0.0f;
            nv = 0.0f;
            nd = v0.y;
            ax = v0.z;
            ay = v0.x;
            bx = v2.z - ax;
            by = v2.x - ay;
            cx = v1.z - ax;
            cy = v1.x - ay;
            break;
        case ORTHOGONAL_Z:
            nu = 0.0f;
            nv = 0.0f;
            nd = v0.z;
            ax = v0.x;
            ay = v0.y;
            bx = v2.x - ax;
            by = v2.y - ay;
            cx = v1.x - ax;
            cy = v1.y - ay;
            break;
        default:
            BOOST_ASSERT(false);
            // Dade - how can I report internal errors ?
            return;
    }

    float det = bx * cy - by * cx;
    float invDet = 1.0f / det;

    bnu = -by * invDet;
    bnv = bx * invDet;
    bnd = (by * ax - bx * ay) * invDet;
    cnu = cy * invDet;
    cnv = -cx * invDet;
    cnd = (cx * ay - cy * ax) * invDet;

    // Dade - doing some precomputation for filling the _DifferentialGeometry_
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
        CoordinateSystem(Normalize(Cross(e1, e2)), &dpdu, &dpdv);
    } else {
        const float invdet = 1.f / determinant;
        dpdu = ( dv2 * dp1 - dv1 * dp2) * invdet;
        dpdv = (-du2 * dp1 + du1 * dp2) * invdet;
    }

    normalizedNormal = Normal(Normalize(Cross(e1, e2)));
}

BBox MeshWaldTriangle::ObjectBound() const {
    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point &p1 = mesh->p[v[0]];
    const Point &p2 = mesh->p[v[1]];
    const Point &p3 = mesh->p[v[2]];
    return Union(BBox(mesh->WorldToObject(p1), mesh->WorldToObject(p2)),
    		mesh->WorldToObject(p3));
}

BBox MeshWaldTriangle::WorldBound() const {
    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point &p1 = mesh->p[v[0]];
    const Point &p2 = mesh->p[v[1]];
    const Point &p3 = mesh->p[v[2]];
    return Union(BBox(p1, p2), p3);
}

bool MeshWaldTriangle::Intersect(const Ray &ray, Intersection *isect) const {
    float uu, vv, t;
    switch (intersectionType) {
        case DOMINANT_X: {
            const float det = ray.d.x + nu * ray.d.y + nv * ray.d.z;
            if(det==0.0f)
                return false;

            const float invDet = 1.0f / det;
            t = (nd - ray.o.x - nu * ray.o.y - nv * ray.o.z) * invDet;

            if (t < ray.mint || t > ray.maxt)
                return false;

            const float hu = ray.o.y + t * ray.d.y;
            const float hv = ray.o.z + t * ray.d.z;
            uu = hu * bnu + hv * bnv + bnd;

            if (uu < 0.0f)
                return false;

            vv = hu * cnu + hv * cnv + cnd;

            if (vv < 0.0f)
                return false;
            if (uu + vv > 1.0f)
                return false;
            break;
        }
        case DOMINANT_Y: {
            const float det = ray.d.y + nu * ray.d.z + nv * ray.d.x;
            if(det==0.0f)
                return false;

            const float invDet = 1.0f / det;
            t = (nd - ray.o.y - nu * ray.o.z - nv * ray.o.x) * invDet;

            if (t < ray.mint || t > ray.maxt)
                return false;

            const float hu = ray.o.z + t * ray.d.z;
            const float hv = ray.o.x + t * ray.d.x;
            uu = hu * bnu + hv * bnv + bnd;

            if (uu < 0.0f)
                return false;

            vv = hu * cnu + hv * cnv + cnd;

            if (vv < 0.0f)
                return false;
            if (uu + vv > 1.0f)
                return false;
            break;
        }
        case DOMINANT_Z: {
            const float det = ray.d.z + nu * ray.d.x + nv * ray.d.y;
            if(det==0.0f)
                return false;

            const float invDet = 1.0f / det;
            t = (nd - ray.o.z - nu * ray.o.x - nv * ray.o.y) * invDet;

            if (t < ray.mint || t > ray.maxt)
                return false;

            const float hu = ray.o.x + t * ray.d.x;
            const float hv = ray.o.y + t * ray.d.y;

            uu = hu * bnu + hv * bnv + bnd;

            if (uu < 0.0f)
                return false;

            vv = hu * cnu + hv * cnv + cnd;

            if (vv < 0.0f)
                return false;
            if (uu + vv > 1.0f)
                return false;
            break;
        }
        case ORTHOGONAL_X: {
            if(ray.d.x == 0.0f)
                return false;

            const float invDet = 1.0f / ray.d.x;
            t = (nd - ray.o.x) * invDet;

            if (t < ray.mint || t > ray.maxt)
                return false;

            const float hu = ray.o.y + t * ray.d.y;
            const float hv = ray.o.z + t * ray.d.z;
            uu = hu * bnu + hv * bnv + bnd;

            if (uu < 0.0f)
                return false;

            vv = hu * cnu + hv * cnv + cnd;

            if (vv < 0.0f)
                return false;
            if (uu + vv > 1.0f)
                return false;
            break;
        }
        case ORTHOGONAL_Y: {
            if(ray.d.y == 0.0f)
                return false;

            const float invDet = 1.0f / ray.d.y;
            t = (nd - ray.o.y) * invDet;

            if (t < ray.mint || t > ray.maxt)
                return false;

            const float hu = ray.o.z + t * ray.d.z;
            const float hv = ray.o.x + t * ray.d.x;
            uu = hu * bnu + hv * bnv + bnd;

            if (uu < 0.0f)
                return false;

            vv = hu * cnu + hv * cnv + cnd;

            if (vv < 0.0f)
                return false;
            if (uu + vv > 1.0f)
                return false;
            break;
        }
        case ORTHOGONAL_Z: {
            if(ray.d.z == 0.0f)
                return false;

            const float invDet = 1.0f / ray.d.z;
            t = (nd - ray.o.z) * invDet;

            if (t < ray.mint || t > ray.maxt)
                return false;

            const float hu = ray.o.x + t * ray.d.x;
            const float hv = ray.o.y + t * ray.d.y;

            uu = hu * bnu + hv * bnv + bnd;

            if (uu < 0.0f)
                return false;

            vv = hu * cnu + hv * cnv + cnd;

            if (vv < 0.0f)
                return false;
            if (uu + vv > 1.0f)
                return false;
            break;
        }
		case DEGENERATE:
			return false;
        default:
            BOOST_ASSERT(false);
            // Dade - how can I report internal errors ?
            return false;
    }

    float uvs[3][2];
    GetUVs(uvs);
    // Interpolate $(u,v)$ triangle parametric coordinates
    const float b0 = 1.0f - uu - vv;
    const float tu = b0 * uvs[0][0] + uu * uvs[1][0] + vv * uvs[2][0];
    const float tv = b0 * uvs[0][1] + uu * uvs[1][1] + vv * uvs[2][1];

	// Dade - using the intepolated normal here in order to fix bug #340
	Normal nn;
	if (mesh->n)
		nn = Normalize(mesh->ObjectToWorld(b0 * mesh->n[v[0]] +
			uu * mesh->n[v[1]] + vv * mesh->n[v[2]]));
	else
		nn = normalizedNormal;

    isect->dg = DifferentialGeometry(ray(t),
            nn,
            dpdu, dpdv,
            Vector(0, 0, 0), Vector(0, 0, 0),
            tu, tv, this);
    isect->dg.AdjustNormal(mesh->reverseOrientation, mesh->transformSwapsHandedness);
    isect->Set(mesh->WorldToObject, this, mesh->GetMaterial().get());
    ray.maxt = t;

    return true;
}

bool MeshWaldTriangle::IntersectP(const Ray &ray) const {
    float uu, vv, t;
    switch (intersectionType) {
        case DOMINANT_X: {
            const float det = ray.d.x + nu * ray.d.y + nv * ray.d.z;
            if(det==0.0f)
                return false;

            const float invDet = 1.0f / det;
            t = (nd - ray.o.x - nu * ray.o.y - nv * ray.o.z) * invDet;

            if (t < ray.mint || t > ray.maxt)
                return false;

            const float hu = ray.o.y + t * ray.d.y;
            const float hv = ray.o.z + t * ray.d.z;
            uu = hu * bnu + hv * bnv + bnd;

            if (uu < 0.0f)
                return false;

            vv = hu * cnu + hv * cnv + cnd;

            if (vv < 0.0f)
                return false;
            if (uu + vv > 1.0f)
                return false;
            break;
        }
        case DOMINANT_Y: {
            const float det = ray.d.y + nu * ray.d.z + nv * ray.d.x;
            if(det==0.0f)
                return false;

            const float invDet = 1.0f / det;
            t = (nd - ray.o.y - nu * ray.o.z - nv * ray.o.x) * invDet;

            if (t < ray.mint || t > ray.maxt)
                return false;

            const float hu = ray.o.z + t * ray.d.z;
            const float hv = ray.o.x + t * ray.d.x;
            uu = hu * bnu + hv * bnv + bnd;

            if (uu < 0.0f)
                return false;

            vv = hu * cnu + hv * cnv + cnd;

            if (vv < 0.0f)
                return false;
            if (uu + vv > 1.0f)
                return false;
            break;
        }
        case DOMINANT_Z: {
            const float det = ray.d.z + nu * ray.d.x + nv * ray.d.y;
            if(det==0.0f)
                return false;

            const float invDet = 1.0f / det;
            t = (nd - ray.o.z - nu * ray.o.x - nv * ray.o.y) * invDet;

            if (t < ray.mint || t > ray.maxt)
                return false;

            const float hu = ray.o.x + t * ray.d.x;
            const float hv = ray.o.y + t * ray.d.y;

            uu = hu * bnu + hv * bnv + bnd;

            if (uu < 0.0f)
                return false;

            vv = hu * cnu + hv * cnv + cnd;

            if (vv < 0.0f)
                return false;
            if (uu + vv > 1.0f)
                return false;
            break;
        }
        case ORTHOGONAL_X: {
            if(ray.d.x == 0.0f)
                return false;

            const float invDet = 1.0f / ray.d.x;
            t = (nd - ray.o.x) * invDet;

            if (t < ray.mint || t > ray.maxt)
                return false;

            const float hu = ray.o.y + t * ray.d.y;
            const float hv = ray.o.z + t * ray.d.z;
            uu = hu * bnu + hv * bnv + bnd;

            if (uu < 0.0f)
                return false;

            vv = hu * cnu + hv * cnv + cnd;

            if (vv < 0.0f)
                return false;
            if (uu + vv > 1.0f)
                return false;
            break;
        }
        case ORTHOGONAL_Y: {
            if(ray.d.y == 0.0f)
                return false;

            const float invDet = 1.0f / ray.d.y;
            t = (nd - ray.o.y) * invDet;

            if (t < ray.mint || t > ray.maxt)
                return false;

            const float hu = ray.o.z + t * ray.d.z;
            const float hv = ray.o.x + t * ray.d.x;
            uu = hu * bnu + hv * bnv + bnd;

            if (uu < 0.0f)
                return false;

            vv = hu * cnu + hv * cnv + cnd;

            if (vv < 0.0f)
                return false;
            if (uu + vv > 1.0f)
                return false;
            break;
        }
        case ORTHOGONAL_Z: {
            if(ray.d.z == 0.0f)
                return false;

            const float invDet = 1.0f / ray.d.z;
            t = (nd - ray.o.z) * invDet;

            if (t < ray.mint || t > ray.maxt)
                return false;

            const float hu = ray.o.x + t * ray.d.x;
            const float hv = ray.o.y + t * ray.d.y;

            uu = hu * bnu + hv * bnv + bnd;

            if (uu < 0.0f)
                return false;

            vv = hu * cnu + hv * cnv + cnd;

            if (vv < 0.0f)
                return false;
            if (uu + vv > 1.0f)
                return false;
            break;
        }
		case DEGENERATE:
			return false;
        default:
            BOOST_ASSERT(false);
            // Dade - how can I report internal errors ?
            return false;
    }

    return true;
}

float MeshWaldTriangle::Area() const {
    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point &p1 = mesh->p[v[0]];
    const Point &p2 = mesh->p[v[1]];
    const Point &p3 = mesh->p[v[2]];

    return 0.5f * Cross(p2 - p1, p3 - p1).Length();
}

void MeshWaldTriangle::Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const {
    float b1, b2;
	UniformSampleTriangle(u1, u2, &b1, &b2);
	// Get triangle vertices in _p1_, _p2_, and _p3_
	const Point &p1 = mesh->p[v[0]];
	const Point &p2 = mesh->p[v[1]];
	const Point &p3 = mesh->p[v[2]];
	float b3 = 1.f - b1 - b2;
	dg->p = b1 * p1 + b2 * p2 + b3 * p3;

	if(mesh->reverseOrientation ^ mesh->transformSwapsHandedness)
		dg->nn = -normalizedNormal;
	else
		dg->nn = normalizedNormal;
	CoordinateSystem(Vector(dg->nn), &dg->dpdu, &dg->dpdv);

    float uv[3][2];
    GetUVs(uv);
    dg->u = b1 * uv[0][0] + b2 * uv[1][0] + b3 * uv[2][0];
    dg->v = b1 * uv[0][1] + b2 * uv[1][1] + b3 * uv[2][1];
}

void MeshWaldTriangle::GetShadingGeometry(const Transform &obj2world,
		const DifferentialGeometry &dg,
		DifferentialGeometry *dgShading) const
{
	if (!mesh->n) {
		*dgShading = dg;
		return;
	}

	// Use _n_ and _s_ to compute shading tangents for triangle, _ss_ and _ts_
	Normal ns = dg.nn;
	Vector ss = Normalize(dg.dpdu);
	Vector ts = Normalize(Cross(ss, ns));
	ss = Cross(ts, ns);

	Vector dndu, dndv;
	// Compute \dndu and \dndv for triangle shading geometry
	float uvs[3][2];
	GetUVs(uvs);

	// Compute deltas for triangle partial derivatives of normal
	float du1 = uvs[0][0] - uvs[2][0];
	float du2 = uvs[1][0] - uvs[2][0];
	float dv1 = uvs[0][1] - uvs[2][1];
	float dv2 = uvs[1][1] - uvs[2][1];
	Vector dn1 = Vector(mesh->n[v[0]] - mesh->n[v[2]]);
	Vector dn2 = Vector(mesh->n[v[1]] - mesh->n[v[2]]);
	float determinant = du1 * dv2 - dv1 * du2;

	if (determinant == 0)
		dndu = dndv = Vector(0, 0, 0);
	else {
		float invdet = 1.f / determinant;
		dndu = ( dv2 * dn1 - dv1 * dn2) * invdet;
		dndv = (-du2 * dn1 + du1 * dn2) * invdet;

		dndu = mesh->ObjectToWorld(dndu);
		dndv = mesh->ObjectToWorld(dndu);
	}

	*dgShading = DifferentialGeometry(
			dg.p,
			ns,
			ss, ts,
			dndu, dndv,
			dg.u, dg.v, this);
	dgShading->reverseOrientation = mesh->reverseOrientation;
	dgShading->transformSwapsHandedness = mesh->transformSwapsHandedness;

	dgShading->dudx = dg.dudx;  dgShading->dvdx = dg.dvdx;
	dgShading->dudy = dg.dudy;  dgShading->dvdy = dg.dvdy;
	dgShading->dpdx = dg.dpdx;  dgShading->dpdy = dg.dpdy;
}

bool MeshWaldTriangle::isDegenerate() const {
	return intersectionType == DEGENERATE;
}

