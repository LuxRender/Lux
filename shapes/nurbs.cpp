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

// nurbs.cpp*
#include "nurbs.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// NURBS Evaluation Functions
static u_int KnotOffset(const float *knot, u_int order, u_int np, float t) {
    u_int firstKnot = order - 1;
    u_int lastKnot = np;

    u_int knotOffset = firstKnot;
    while (t > knot[knotOffset+1])
	++knotOffset;
    assert(knotOffset < lastKnot);
    assert(t >= knot[knotOffset] && t <= knot[knotOffset + 1]);
    return knotOffset;
}

// doesn't handle flat out discontinuities in the curve...

struct Homogeneous3 {
Homogeneous3() { x = y = z = w = 0.f; }
Homogeneous3(float xx, float yy, float zz, float ww) {
    x = xx; y = yy; z = zz; w = ww;
}
float x, y, z, w;
};
static Homogeneous3
NURBSEvaluate(u_int order, const float *knot, const Homogeneous3 *cp, u_int np,
	u_int cpStride, float t, Vector *deriv = NULL) {
//    int nKnots = np + order;
    float alpha;

    u_int knotOffset = KnotOffset(knot, order, np, t);
    knot += knotOffset;

    u_int cpOffset = knotOffset - order + 1;
    assert(cpOffset < np);

    Homogeneous3 *cpWork =
	static_cast<Homogeneous3 *>(alloca(order * sizeof(Homogeneous3)));
    for (u_int i = 0; i < order; ++i)
	cpWork[i] = cp[(cpOffset+i) * cpStride];

    for (u_int i = 0; i < order - 2; ++i)
	for (u_int j = 0; j < order - 1 - i; ++j) {
	    alpha = (knot[1 + j] - t) /
		(knot[1 + j] - knot[j + 2 - order + i]);
	    assert(alpha >= 0.f && alpha <= 1.f);

	    cpWork[j].x = cpWork[j].x * alpha + cpWork[j+1].x * (1.f - alpha);
	    cpWork[j].y = cpWork[j].y * alpha + cpWork[j+1].y * (1.f - alpha);
	    cpWork[j].z = cpWork[j].z * alpha + cpWork[j+1].z * (1.f - alpha);
	    cpWork[j].w = cpWork[j].w * alpha + cpWork[j+1].w * (1.f - alpha);
	}

    alpha = (knot[1] - t) / (knot[1] - knot[0]);
    assert(alpha >= 0.f && alpha <= 1.f);

    Homogeneous3 val(cpWork[0].x * alpha + cpWork[1].x * (1.f - alpha),
		     cpWork[0].y * alpha + cpWork[1].y * (1.f - alpha),
		     cpWork[0].z * alpha + cpWork[1].z * (1.f - alpha),
		     cpWork[0].w * alpha + cpWork[1].w * (1.f - alpha));

    if (deriv) {
		float factor = (order - 1) / (knot[1] - knot[0]);
		Homogeneous3 delta((cpWork[1].x - cpWork[0].x) * factor,
			   (cpWork[1].y - cpWork[0].y) * factor,
			   (cpWork[1].z - cpWork[0].z) * factor,
			   (cpWork[1].w - cpWork[0].w) * factor);

		deriv->x = delta.x / val.w - (val.x * delta.w / (val.w * val.w));
		deriv->y = delta.y / val.w - (val.y * delta.w / (val.w * val.w));
		deriv->z = delta.z / val.w - (val.z * delta.w / (val.w * val.w));
    }

    return val;
}
static Point
NURBSEvaluateSurface(u_int uOrder, const float *uKnot, u_int ucp, float u,
		     u_int vOrder, const float *vKnot, u_int vcp, float v,
		     const Homogeneous3 *cp, Vector *dPdu, Vector *dPdv) {
    Homogeneous3 *iso = static_cast<Homogeneous3 *>(alloca(max(uOrder, vOrder) *
					       sizeof(Homogeneous3)));

    u_int uOffset = KnotOffset(uKnot, uOrder, ucp, u);
    u_int uFirstCp = uOffset - uOrder + 1;
    assert(uFirstCp >= 0 && uFirstCp + uOrder - 1 < ucp);

    for (u_int i = 0; i < uOrder; ++i)
		iso[i] = NURBSEvaluate(vOrder, vKnot, &cp[uFirstCp + i], vcp,
			       ucp, v);

    u_int vOffset = KnotOffset(vKnot, vOrder, vcp, v);
    u_int vFirstCp = vOffset - vOrder + 1;
    assert(vFirstCp >= 0 && vFirstCp + vOrder - 1 < vcp);

    Homogeneous3 P = NURBSEvaluate(uOrder, uKnot, iso - uFirstCp, ucp,
				   1, u, dPdu);

    if (dPdv) {
		for (u_int i = 0; i < vOrder; ++i)
		    iso[i] = NURBSEvaluate(uOrder, uKnot, &cp[(vFirstCp+i)*ucp], ucp,
				   1, u);
		NURBSEvaluate(vOrder, vKnot, iso - vFirstCp, vcp, 1, v, dPdv);
    }
    return Point(P.x/P.w, P.y/P.w, P.z/P.w);;
}
// NURBS Definitions
NURBS::NURBS(const Transform &o2w, bool ro, const string &name, 
		u_int numu, u_int uo, const float *uk,
		float u0, float u1, u_int numv, u_int vo, const float *vk,
		float v0, float v1, const float *p, bool homogeneous)
	: Shape(o2w, ro, name) {
	nu = numu;    uorder = uo;
	umin = u0;    umax = u1;
	nv = numv;    vorder = vo;
	vmin = v0;    vmax = v1;
	isHomogeneous = homogeneous;
	if (isHomogeneous) {
		P = new float[4*nu*nv];
		memcpy(P, p, 4*nu*nv*sizeof(float));
	} else {
		P = new float[3*nu*nv];
		memcpy(P, p, 3*nu*nv*sizeof(float));
	}
	uknot = new float[nu + uorder];
	memcpy(uknot, uk, (nu + uorder) * sizeof(float));
	vknot = new float[nv + vorder];
	memcpy(vknot, vk, (nv + vorder) * sizeof(float));
}
NURBS::~NURBS() {
	delete[] P;
	delete[] uknot;
	delete[] vknot;
}
BBox NURBS::ObjectBound() const {
	if (!isHomogeneous) {
		// Compute object-space bound of non-homogeneous NURBS
		float *pp = P;
		BBox bound = Point(pp[0], pp[1], pp[2]);
		for (u_int i = 0; i < nu*nv; ++i, pp += 3)
			bound = Union(bound, Point(pp[0], pp[1], pp[2]));
		return bound;
	} else {
		// Compute object-space bound of homogeneous NURBS
		float *pp = P;
		BBox bound = Point(pp[0] / pp[3], pp[1] / pp[3], pp[2] / pp[3]);
		for (u_int i = 0; i < nu*nv; ++i, pp += 4)
			bound = Union(bound, Point(pp[0] / pp[3], pp[1] / pp[3], pp[2] / pp[3]));
		return bound;
	}
}
BBox NURBS::WorldBound() const {
	if (!isHomogeneous) {
		// Compute world-space bound of non-homogeneous NURBS
		float *pp = P;
		Point pt(ObjectToWorld * Point(pp[0], pp[1], pp[2]));
		BBox bound = pt;
		for (u_int i = 0; i < nu*nv; ++i, pp += 3) {
			pt = ObjectToWorld * Point(pp[0], pp[1], pp[2]);
			bound = Union(bound, pt);
		}
		return bound;
	} else {
		// Compute world-space bound of homogeneous NURBS
		float *pp = P;
		Point pt(ObjectToWorld * Point(pp[0] / pp[3],
			pp[1] / pp[3], pp[2] / pp[3]));
		BBox bound = pt;
		for (u_int i = 0; i < nu*nv; ++i, pp += 4) {
			pt = ObjectToWorld * Point(pp[0] / pp[3],
				pp[1] / pp[3], pp[2] / pp[3]);
			bound = Union(bound, pt);
		}
		return bound;
	}
}

void NURBS::Refine(vector<boost::shared_ptr<Shape> > &refined) const {
	// Compute NURBS dicing rates
	u_int diceu = 30, dicev = 30;
	float *ueval = new float[diceu];
	float *veval = new float[dicev];
	Point *evalPs = new Point[diceu*dicev];
	Normal *evalNs = new Normal[diceu*dicev];
	for (u_int i = 0; i < diceu; ++i)
		ueval[i] = Lerp((float)i / (float)(diceu-1), umin, umax);
	for (u_int i = 0; i < dicev; ++i)
		veval[i] = Lerp((float)i / (float)(dicev-1), vmin, vmax);
	// Evaluate NURBS over grid of points
	memset(evalPs, 0, diceu*dicev*sizeof(Point));
	memset(evalNs, 0, diceu*dicev*sizeof(Point));
	float *uvs = new float[2*diceu*dicev];
	// Turn NURBS into triangles
	Homogeneous3 *Pw = (Homogeneous3 *)P;
	if (!isHomogeneous) {
		Pw = static_cast<Homogeneous3 *>(alloca(nu*nv*sizeof(Homogeneous3)));
		for (u_int i = 0; i < nu*nv; ++i) {
			Pw[i].x = P[3*i];
			Pw[i].y = P[3*i+1];
			Pw[i].z = P[3*i+2];
			Pw[i].w = 1.f;
		}
	}
	for (u_int v = 0; v < dicev; ++v) {
		for (u_int u = 0; u < diceu; ++u) {
			uvs[2*(v*diceu+u)]   = ueval[u];
			uvs[2*(v*diceu+u)+1] = veval[v];
	
			Vector dPdu, dPdv;
			Point pt = NURBSEvaluateSurface(uorder, uknot, nu, ueval[u],
				vorder, vknot, nv, veval[v], Pw, &dPdu, &dPdv);
			evalPs[v*diceu + u].x = pt.x;
			evalPs[v*diceu + u].y = pt.y;
			evalPs[v*diceu + u].z = pt.z;
			evalNs[v*diceu + u] = Normal(Normalize(Cross(dPdu, dPdv)));
		}
	}
	// Generate points-polygons mesh
	u_int nTris = 2*(diceu-1)*(dicev-1);
	int *vertices = new int[3 * nTris];
	int *vertp = vertices;
	// Compute the vertex offset numbers for the triangles
	for (u_int v = 0; v < dicev-1; ++v) {
		for (u_int u = 0; u < diceu-1; ++u) {
	#define VN(u,v) static_cast<int>((v)*diceu+(u))
			*vertp++ = VN(u,   v);
			*vertp++ = VN(u+1, v);
			*vertp++ = VN(u+1, v+1);
	
			*vertp++ = VN(u,   v);
			*vertp++ = VN(u+1, v+1);
			*vertp++ = VN(u,   v+1);
	#undef VN
		}
	}
	u_int nVerts = diceu*dicev;
	ParamSet paramSet;
	paramSet.AddInt("indices", vertices, 3*nTris);
	paramSet.AddPoint("P", evalPs, nVerts);
	paramSet.AddFloat("uv", uvs, 2 * nVerts);
	paramSet.AddNormal("N", evalNs, nVerts);
	refined.push_back(MakeShape("trianglemesh", ObjectToWorld,
			reverseOrientation, paramSet));
	// Cleanup from NURBS refinement
	delete[] uvs;
	delete[] ueval;
	delete[] veval;
	delete[] evalPs;
	delete[] evalNs;
	delete[] vertices;
}
Shape* NURBS::CreateShape(const Transform &o2w,
		bool reverseOrientation, const ParamSet &params) {
	string name = params.FindOneString("name", "'nurbs'");
	int nu = params.FindOneInt("nu", -1);
	int uorder = params.FindOneInt("uorder", -1);
	u_int nuknots, nvknots;
	const float *uknots = params.FindFloat("uknots", &nuknots);
	BOOST_ASSERT(nu != -1 && uorder != -1 && uknots != NULL);
	BOOST_ASSERT(nuknots == static_cast<u_int>(nu) + uorder);
	float u0 = params.FindOneFloat("u0", uknots[uorder-1]);
	float u1 = params.FindOneFloat("u1", uknots[nu]);

	int nv = params.FindOneInt("nv", -1);
	int vorder = params.FindOneInt("vorder", -1);
	const float *vknots = params.FindFloat("vknots", &nvknots);
	BOOST_ASSERT(nv != -1 && vorder != -1 && vknots != NULL);
	BOOST_ASSERT(nvknots == static_cast<u_int>(nv) + vorder);
	float v0 = params.FindOneFloat("v0", vknots[vorder-1]);
	float v1 = params.FindOneFloat("v1", vknots[nv]);

	bool isHomogeneous = false;
	u_int npts;
	const float *P = reinterpret_cast<const float *>(params.FindPoint("P", &npts));
	if (!P) {
		P = params.FindFloat("Pw", &npts);
		npts /= 4;
		if (!P) return NULL;
		isHomogeneous = true;
	}
	BOOST_ASSERT(P);
	BOOST_ASSERT(npts == static_cast<u_int>(nu*nv));
	return new NURBS(o2w, reverseOrientation, name, nu, uorder, uknots, u0, u1,
		nv, vorder, vknots, v0, v1, const_cast<float *>(P),
		isHomogeneous);
}

static DynamicLoader::RegisterShape<NURBS> r("nurbs");
