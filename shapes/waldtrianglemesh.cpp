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


#include "waldtrianglemesh.h"
#include "mc.h"
#include "paramset.h"

using namespace lux;

// WaldTriangleMesh Method Definitions
WaldTriangleMesh::WaldTriangleMesh(const Transform &o2w, bool ro,
        int nt, int nv, const int *vi, const Point *P,
        const Normal *N, const Vector *S, const float *uv)
: Shape(o2w, ro) {
    ntris = nt;
    nverts = nv;
    vertexIndex = new int[3 * ntris];
    memcpy(vertexIndex, vi, 3 * ntris * sizeof(int));

	// Copy _uv_, _N_, and _S_ vertex data, if present
    if (uv) {
        uvs = new float[2*nverts];
        memcpy(uvs, uv, 2*nverts*sizeof(float));
    } else
		uvs = NULL;

    p = new Point[nverts];
    if (N) {
        n = new Normal[nverts];
        memcpy(n, N, nverts*sizeof(Normal));
    } else
		n = NULL;

    if (S) {
        s = new Vector[nverts];
        memcpy(s, S, nverts*sizeof(Vector));
    } else
		s = NULL;

    // Transform mesh vertices to world space
    for (int i  = 0; i < nverts; ++i)
        p[i] = ObjectToWorld(P[i]);
}

WaldTriangleMesh::~WaldTriangleMesh() {
    delete[] vertexIndex;
    delete[] p;
    delete[] s;
    delete[] n;
    delete[] uvs;
}

BBox WaldTriangleMesh::ObjectBound() const {
    BBox bobj;
    for (int i = 0; i < nverts; i++)
        bobj = Union(bobj, WorldToObject(p[i]));
    return bobj;
}

BBox WaldTriangleMesh::WorldBound() const {
    BBox worldBounds;
    for (int i = 0; i < nverts; i++)
        worldBounds = Union(worldBounds, p[i]);
    return worldBounds;
}

void
WaldTriangleMesh::Refine(vector<boost::shared_ptr<Shape> > &refined)
const {
    for (int i = 0; i < ntris; ++i) {
        boost::shared_ptr<Shape> o(new WaldTriangle(ObjectToWorld,
                reverseOrientation,
                (WaldTriangleMesh *)this,
                i));
        refined.push_back(o);
    }
}

WaldTriangle::WaldTriangle(const Transform &o2w, bool ro,
        WaldTriangleMesh *m, int n)
: Shape(o2w, ro) {
    mesh = m;
    v = &mesh->vertexIndex[3*n];
    // Update created triangles stats
    // radiance - disabled for threading // static StatsCounter trisMade("Geometry","Triangles created");
    // radiance - disabled for threading // ++trisMade;
    
    // Wald's precomputed values
    
    // Look for the dominant axis
    
    const Point &v0 = mesh->p[v[0]];
    const Point &v1 = mesh->p[v[1]];
    const Point &v2 = mesh->p[v[2]];
    Vector e1 = v1 - v0;
    Vector e2 = v2 - v0;
    Vector normal = Normalize(Cross(e1, e2));
    
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
    
    // NOTE - ratow - Invert generated normal in case it falls on the wrong side.
    // Dade - this computation can be done at scene creation time too
    
    normalizedNormal = Normal(Normalize(Cross(dpdu, dpdv)));
	if(mesh->n) {
		if(Dot(ObjectToWorld(mesh->n[v[0]]+mesh->n[v[1]]+mesh->n[v[2]]), normalizedNormal) < 0)
			normalizedNormal *= -1;
	} else {
		if(Dot(Cross(e1, e2), normalizedNormal) < 0)
			normalizedNormal *= -1;
	}

    // Adjust normal based on orientation and handedness
    if (this->reverseOrientation ^ this->transformSwapsHandedness)
        normalizedNormal *= -1.f;
}

BBox WaldTriangle::ObjectBound() const {
    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point &p1 = mesh->p[v[0]];
    const Point &p2 = mesh->p[v[1]];
    const Point &p3 = mesh->p[v[2]];
    return Union(BBox(WorldToObject(p1), WorldToObject(p2)),
            WorldToObject(p3));
}

BBox WaldTriangle::WorldBound() const {
    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point &p1 = mesh->p[v[0]];
    const Point &p2 = mesh->p[v[1]];
    const Point &p3 = mesh->p[v[2]];
    return Union(BBox(p1, p2), p3);
}

bool WaldTriangle::Intersect(const Ray &ray, float *tHit,
        DifferentialGeometry *dg) const {
    // Dade - debugging code
    //std::stringstream ss;
    //ss<<"ray.mint = "<<ray.mint<<" ray.maxt = "<<ray.maxt;
    //luxError(LUX_NOERROR,LUX_INFO,ss.str().c_str());
    
    float uu, vv, t;
    switch (intersectionType) {
        case DOMINANT_X: {
            const float det = ray.d.x + nu * ray.d.y + nv * ray.d.z;
            if(det==0.0f)
                return false;
            
            const float invDet = 1.0f / det;
            t = (nd - ray.o.x - nu * ray.o.y - nv * ray.o.z) * invDet;
            
            // Dade - debugging code
            //std::stringstream ss;
            //ss<<"t = "<<t<<" ray.mint = "<<ray.mint<<" ray.maxt = "<<ray.maxt;
            //luxError(LUX_NOERROR,LUX_INFO,ss.str().c_str());
            
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
        default:
            BOOST_ASSERT(false);
            // Dade - how can I report internal errors ?
            return false;
    }
    
    // radiance - disabled for threading // triangleHits.Add(1, 0); //NOBOOK
    float uvs[3][2];
    GetUVs(uvs);
    // Interpolate $(u,v)$ triangle parametric coordinates
    const float b0 = 1.0f - uu - vv;
    const float tu = b0*uvs[0][0] + uu*uvs[1][0] + vv*uvs[2][0];
    const float tv = b0*uvs[0][1] + uu*uvs[1][1] + vv*uvs[2][1];
    *dg = DifferentialGeometry(ray(t),
            normalizedNormal,
            dpdu, dpdv,
            Vector(0, 0, 0), Vector(0, 0, 0),
            tu, tv, this);

    *tHit = t;
    return true;
}

bool WaldTriangle::IntersectP(const Ray &ray) const {
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
        default:
            BOOST_ASSERT(false);
            // Dade - how can I report internal errors ?
            return false;
    }
    
    return true;
}

float WaldTriangle::Area() const {
    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point &p1 = mesh->p[v[0]];
    const Point &p2 = mesh->p[v[1]];
    const Point &p3 = mesh->p[v[2]];
    return 0.5f * Cross(p2-p1, p3-p1).Length();
}

Point WaldTriangle::Sample(float u1, float u2,
        Normal *Ns) const {
    float b1, b2;
    UniformSampleTriangle(u1, u2, &b1, &b2);
    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point &p1 = mesh->p[v[0]];
    const Point &p2 = mesh->p[v[1]];
    const Point &p3 = mesh->p[v[2]];
    Point p = b1 * p1 + b2 * p2 + (1.f - b1 - b2) * p3;

    *Ns = normalizedNormal;

    return p;
}

Shape* WaldTriangleMesh::CreateShape(const Transform &o2w,
        bool reverseOrientation, const ParamSet &params) {
    int nvi, npi, nuvi, nsi, nni;
    const int *vi = params.FindInt("indices", &nvi);
    const Point *P = params.FindPoint("P", &npi);
    const float *uvs = params.FindFloat("uv", &nuvi);

	if (!uvs) uvs = params.FindFloat("st", &nuvi);
    // NOTE - lordcrc - Bugfix, pbrt tracker id 0000085: check for correct number of uvs
    if (uvs && nuvi != npi * 2) {
        luxError(LUX_CONSISTENCY, LUX_ERROR, "Number of \"uv\"s for triangle mesh must match \"P\"s");
        uvs = NULL;
    }
    if (!vi || !P) return NULL;

	const Vector *S = params.FindVector("S", &nsi);
    if (S && nsi != npi) {
        luxError(LUX_CONSISTENCY, LUX_ERROR, "Number of \"S\"s for triangle mesh must match \"P\"s");
        S = NULL;
    }

	const Normal *N = params.FindNormal("N", &nni);
    if (N && nni != npi) {
        luxError(LUX_CONSISTENCY, LUX_ERROR, "Number of \"N\"s for triangle mesh must match \"P\"s");
        N = NULL;
    }
    if (uvs && N) {
        // if there are normals, check for bad uv's that
        // give degenerate mappings; discard them if so
        const int *vp = vi;
        for (int i = 0; i < nvi; i += 3, vp += 3) {
            float area = .5f * Cross(P[vp[0]]-P[vp[1]], P[vp[2]]-P[vp[1]]).Length();
            if (area < 1e-7) continue; // ignore degenerate tris.
            // NOTE - radiance - disabled check for degenerate uv coordinates.
            // discards all uvs in case of unimportant small errors from exporters.
            /*if ((uvs[2*vp[0]] == uvs[2*vp[1]] &&
             * uvs[2*vp[0]+1] == uvs[2*vp[1]+1]) ||
             * (uvs[2*vp[1]] == uvs[2*vp[2]] &&
             * uvs[2*vp[1]+1] == uvs[2*vp[2]+1]) ||
             * (uvs[2*vp[2]] == uvs[2*vp[0]] &&
             * uvs[2*vp[2]+1] == uvs[2*vp[0]+1])) {
             * Warning("Degenerate uv coordinates in triangle mesh.  Discarding all uvs.");
             * printf("vp = %i\n", *vp);
             * uvs = NULL;
             * break;
             * }*/
        }
    }
    for (int i = 0; i < nvi; ++i)
        if (vi[i] >= npi) {
            //Error("waldtrianglemesh has out of-bounds vertex index %d (%d \"P\" values were given",
            //	vi[i], npi);
            std::stringstream ss;
            ss<<"waldtrianglemesh has out of-bounds vertex index "<<vi[i]<<" ("<<npi<<"  \"P\" values were given";
            luxError(LUX_CONSISTENCY, LUX_ERROR, ss.str().c_str());
            return NULL;
        }

    return new WaldTriangleMesh(o2w, reverseOrientation, nvi/3, npi, vi, P,
            N, S, uvs);
}
