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

// trianglemesh.cpp*
#include "shape.h"
#include "paramset.h"

namespace lux
{

// TriangleMesh Declarations
class TriangleMesh : public Shape {
public:
	// TriangleMesh Public Methods
	TriangleMesh(const Transform &o2w, bool ro,
	             int ntris, int nverts, const int *vptr,
				 const Point *P, const Normal *N,
				 const Vector *S, const float *uv);
	~TriangleMesh();
	BBox ObjectBound() const;
	BBox WorldBound() const;
	bool CanIntersect() const { return false; }
	void Refine(vector<boost::shared_ptr<Shape> > &refined) const;
	friend class Triangle;
	template <class T> friend class VertexTexture;
	
	static Shape* CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);
protected:
	// TriangleMesh Data
	int ntris, nverts;
	int *vertexIndex;
	Point *p;
	Normal *n;
	Vector *s;
	float *uvs;
	vector<boost::shared_ptr<Shape> > triPtrs;
};
class Triangle : public Shape {
public:
	// Triangle Public Methods
	Triangle(const Transform &o2w, bool ro,
	         TriangleMesh *m, int n)
			: Shape(o2w, ro) {
		mesh = m;
		v = &mesh->vertexIndex[3*n];
		// Update created triangles stats
		// radiance - disabled for threading // static StatsCounter trisMade("Geometry","Triangles created");
		// radiance - disabled for threading // ++trisMade;
	}
	BBox ObjectBound() const;
	BBox WorldBound() const;
	bool Intersect(const Ray &ray, float *tHit,
	               DifferentialGeometry *dg) const;
	bool IntersectP(const Ray &ray) const;
	void GetUVs(float uv[3][2]) const;
	float Area() const;
	virtual void GetShadingGeometry(const Transform &obj2world,
			const DifferentialGeometry &dg,
			DifferentialGeometry *dgShading) const {
		if (!mesh->n && !mesh->s) {
			*dgShading = dg;
			return;
		}
		// Initialize _Triangle_ shading geometry with _n_ and _s_
		// Compute barycentric coordinates for point
		float b[3];
		// Initialize _A_ and _C_ matrices for barycentrics
		float uv[3][2];
		GetUVs(uv);
		float A[2][2] =
		    { { uv[1][0] - uv[0][0], uv[2][0] - uv[0][0] },
		      { uv[1][1] - uv[0][1], uv[2][1] - uv[0][1] } };
		float C[2] = { dg.u - uv[0][0], dg.v - uv[0][1] };
		if (!SolveLinearSystem2x2(A, C, &b[1])) {
			// Handle degenerate parametric mapping
			b[0] = b[1] = b[2] = 1.f/3.f;
		}
		else
			b[0] = 1.f - b[1] - b[2];
		// Use _n_ and _s_ to compute shading tangents for triangle, _ss_ and _ts_
		Normal ns;
		Vector ss, ts;
		if (mesh->n) ns = Normalize(obj2world(b[0] * mesh->n[v[0]] +
			b[1] * mesh->n[v[1]] + b[2] * mesh->n[v[2]]));
		else   ns = dg.nn;
		if (mesh->s) ss = Normalize(obj2world(b[0] * mesh->s[v[0]] +
			b[1] * mesh->s[v[1]] + b[2] * mesh->s[v[2]]));
		else   ss = Normalize(dg.dpdu);
		ts = Normalize(Cross(ss, ns));
		ss = Cross(ts, ns);
		Vector dndu, dndv;
		if (mesh->n) {
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
				dndu = dndv = Vector(0,0,0);
			else {
				float invdet = 1.f / determinant;
				dndu = ( dv2 * dn1 - dv1 * dn2) * invdet;
				dndv = (-du2 * dn1 + du1 * dn2) * invdet;
			}
		}
		else
			dndu = dndv = Vector(0,0,0);
		*dgShading = DifferentialGeometry(dg.p, ss, ts,
			ObjectToWorld(dndu), ObjectToWorld(dndv), dg.u, dg.v, dg.shape);
		dgShading->dudx = dg.dudx;  dgShading->dvdx = dg.dvdx; // NOBOOK
		dgShading->dudy = dg.dudy;  dgShading->dvdy = dg.dvdy; // NOBOOK
		dgShading->dpdx = dg.dpdx;  dgShading->dpdy = dg.dpdy; // NOBOOK
	}
	Point Sample(float u1, float u2, Normal *Ns) const;
private:
	// Triangle Data
	TriangleMesh* mesh;
	int *v;
};

}//namespace lux
