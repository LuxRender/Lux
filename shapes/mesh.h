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

#include "shape.h"
#include "paramset.h"

namespace lux
{

enum MeshTriangleType { WALD_TRIANGLE, BARY_TRIANGLE };
enum MeshQuadType { QUADRILATERAL };

class Mesh : public Shape {
public:
	Mesh(const Transform &o2w, bool ro,
			int nv, const Point *P, const Normal *N, const float *UV,
			MeshTriangleType tritype, int trisCount, const int *tris,
			MeshQuadType quadtype, int nquadsCount, const int *quads);
	~Mesh();

	BBox ObjectBound() const;
	BBox WorldBound() const;
	bool CanIntersect() const { return false; }
	void Refine(vector<boost::shared_ptr<Shape> > &refined) const;

	friend class MeshWaldTriangle;
	friend class MeshBaryTriangle;
	friend class MeshQuadrilateral;

	static Shape* CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);

protected:
	// Dade - vertices data
	int nverts;
	Point *p;
	Normal *n;
	float *uvs;

	// Dade - triangle data
	MeshTriangleType triType;
	int ntris;
	int *triVertexIndex;
	vector<boost::shared_ptr<Shape> > triPtrs;

	// Dade - quad data
	MeshQuadType quadType;
	int nquads;
	int *quadVertexIndex;
	vector<boost::shared_ptr<Shape> > quadPtrs;
};

//------------------------------------------------------------------------------
// Triangle shapes
//------------------------------------------------------------------------------

class MeshWaldTriangle : public Shape {
public:
	// WaldTriangle Public Methods
	MeshWaldTriangle(const Transform &o2w, bool ro, Mesh *m, int n);

	BBox ObjectBound() const;
	BBox WorldBound() const;

	bool Intersect(const Ray &ray, float *tHit,
			DifferentialGeometry *dg) const;
	bool IntersectP(const Ray &ray) const;

	float Area() const;
	virtual void GetShadingGeometry(const Transform &obj2world,
			const DifferentialGeometry &dg,
			DifferentialGeometry *dgShading) const;
	Point Sample(float u1, float u2, float u3, Normal *Ns) const;

private:	
	void GetUVs(float uv[3][2]) const {
		if (mesh->uvs) {
			uv[0][0] = mesh->uvs[2 * v[0]];
			uv[0][1] = mesh->uvs[2 * v[0] + 1];
			uv[1][0] = mesh->uvs[2 * v[1]];
			uv[1][1] = mesh->uvs[2 * v[1] + 1];
			uv[2][0] = mesh->uvs[2 * v[2]];
			uv[2][1] = mesh->uvs[2 * v[2] + 1];
		} else {
			uv[0][0] = mesh->p[v[0]].x;
			uv[0][1] = mesh->p[v[0]].y;
			uv[1][0] = mesh->p[v[1]].x;
			uv[1][1] = mesh->p[v[1]].y;
			uv[2][0] = mesh->p[v[2]].x;
			uv[2][1] = mesh->p[v[2]].y;
		}
	}

	// WaldTriangle Data
	Mesh* mesh;
	int *v;

	// Dade - Wald's precomputed values
	enum IntersectionType {
		DOMINANT_X,
		DOMINANT_Y,
		DOMINANT_Z,
		ORTHOGONAL_X,
		ORTHOGONAL_Y,
		ORTHOGONAL_Z,
		DEGENERATE
	};

	IntersectionType intersectionType;
	float nu, nv, nd;
	float bnu, bnv, bnd;
	float cnu, cnv, cnd;

	// Dade - procomputed values for filling the DifferentialGeometry
	Vector dpdu, dpdv;
	Normal normalizedNormal;
};

class MeshBaryTriangle : public Shape {
public:
    // BaryTriangle Public Methods
    MeshBaryTriangle(const Transform &o2w, bool ro,
			Mesh *m, int n) : Shape(o2w, ro) {
        mesh = m;
        v = &mesh->triVertexIndex[3 * n];
    }

    BBox ObjectBound() const;
    BBox WorldBound() const;
    bool Intersect(const Ray &ray, float *tHit,
            DifferentialGeometry *dg) const;
    bool IntersectP(const Ray &ray) const;

    float Area() const;
    virtual void GetShadingGeometry(const Transform &obj2world,
            const DifferentialGeometry &dg,
            DifferentialGeometry *dgShading) const;
    Point Sample(float u1, float u2, Normal *Ns) const;

private:
	void GetUVs(float uv[3][2]) const {
		if (mesh->uvs) {
			uv[0][0] = mesh->uvs[2*v[0]];
			uv[0][1] = mesh->uvs[2*v[0]+1];
			uv[1][0] = mesh->uvs[2*v[1]];
			uv[1][1] = mesh->uvs[2*v[1]+1];
			uv[2][0] = mesh->uvs[2*v[2]];
			uv[2][1] = mesh->uvs[2*v[2]+1];
		} else {
			uv[0][0] = mesh->p[v[0]].x;
			uv[0][1] = mesh->p[v[0]].y;
			uv[1][0] = mesh->p[v[1]].x;
			uv[1][1] = mesh->p[v[1]].y;
			uv[2][0] = mesh->p[v[2]].x;
			uv[2][1] = mesh->p[v[2]].y;
		}
	}

    // BaryTriangle Data
    Mesh *mesh;
    int *v;
};

//------------------------------------------------------------------------------
// Quad shapes
//------------------------------------------------------------------------------

// Quadrilateral Declarations
// assumes points form a strictly convex, planar quad
class MeshQuadrilateral : public Shape {
public:
	// Quadrilateral Public Methods
	MeshQuadrilateral(const lux::Transform &o2w, bool ro, Mesh *m, int n);

	BBox ObjectBound() const;
	BBox WorldBound() const;
	bool Intersect(const Ray &ray, float *tHit,
	               DifferentialGeometry *dg) const;
	bool IntersectP(const Ray &ray) const;

	float Area() const;
	virtual void GetShadingGeometry(const Transform &obj2world,
            const DifferentialGeometry &dg,
            DifferentialGeometry *dgShading) const;

	Point Sample(float u1, float u2, Normal *Ns) const {
		Point p;

		const Point &p0 = mesh->p[idx[0]];
		const Point &p1 = mesh->p[idx[1]];
		const Point &p2 = mesh->p[idx[2]];
	    const Point &p3 = mesh->p[idx[3]];

		p = (1.f-u1)*(1.f-u2)*p0 + u1*(1.f-u2)*p1
			+u1*u2*p2 + (1.f-u1)*u2*p3;

		Vector e0 = p1 - p0;
		Vector e1 = p2 - p0;

		*Ns = Normalize(Normal(Cross(e0, e1)));
		if (reverseOrientation) *Ns *= -1.f;
		return p;
	}

	static bool IsPlanar(const Point &p0, const Point &p1, const Point &p2, const Point &p3);
	static bool IsDegenerate(const Point &p0, const Point &p1, const Point &p2, const Point &p3);
	static bool IsConvex(const Point &p0, const Point &p1, const Point &p2, const Point &p3);

private:
	static float Det2x2(const float a00, const float a01, const float a10, const float a11);
	static float Det3x3(float A[3][3]);
	static bool Invert3x3(float A[3][3], float InvA[3][3]);
	static int MajorAxis(const Vector &v);

	static void ComputeV11BarycentricCoords(const Vector &e01, const Vector &e02, const Vector &e03, float *a11, float *b11);

	void GetUVs(float uv[4][2]) const {		
		if (mesh->uvs) {
			uv[0][0] = mesh->uvs[2 * idx[0]];
			uv[0][1] = mesh->uvs[2 * idx[0] + 1];
			uv[1][0] = mesh->uvs[2 * idx[1]];
			uv[1][1] = mesh->uvs[2 * idx[1] + 1];
			uv[2][0] = mesh->uvs[2 * idx[2]];
			uv[2][1] = mesh->uvs[2 * idx[2] + 1];
			uv[3][0] = mesh->uvs[2 * idx[3]];
			uv[3][1] = mesh->uvs[2 * idx[3] + 1];
		} else {
			uv[0][0] = mesh->p[idx[0]].x;
			uv[0][1] = mesh->p[idx[0]].y;
			uv[1][0] = mesh->p[idx[1]].x;
			uv[1][1] = mesh->p[idx[1]].y;
			uv[2][0] = mesh->p[idx[2]].x;
			uv[2][1] = mesh->p[idx[2]].y;
			uv[3][0] = mesh->p[idx[3]].x;
			uv[3][1] = mesh->p[idx[3]].y;
		}
	}

	// Quadrilateral Private Data
	Mesh *mesh;
	int *idx;
};

}//namespace lux
