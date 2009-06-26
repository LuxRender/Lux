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

class Mesh : public Shape {
public:
	enum MeshTriangleType { TRI_WALD, TRI_BARY, TRI_AUTO };
	enum MeshQuadType { QUAD_QUADRILATERAL };
	enum MeshAccelType { ACCEL_KDTREE, ACCEL_BVH, ACCEL_NONE, ACCEL_GRID, ACCEL_BRUTEFORCE, ACCEL_AUTO };
	enum MeshSubdivType { SUBDIV_LOOP };

	Mesh(const Transform &o2w, bool ro,
			MeshAccelType acceltype,
			int nv, const Point *P, const Normal *N, const float *UV,
			MeshTriangleType tritype, int trisCount, const int *tris,
			MeshQuadType quadtype, int nquadsCount, const int *quads,
			MeshSubdivType subdivType, int nsubdivlevels,
			boost::shared_ptr<Texture<float> > displacementMap,
			float displacementMapScale, float displacementMapOffset,
			bool displacementMapNormalSmooth, bool displacementMapSharpBoundary);
	~Mesh();

	BBox ObjectBound() const;
	BBox WorldBound() const;
	bool CanIntersect() const { return false; }
	void Refine(vector<boost::shared_ptr<Primitive> > &refined,
			const PrimitiveRefinementHints &refineHints,
			boost::shared_ptr<Primitive> thisPtr);
	bool CanSample() const { return false; }

	friend class MeshWaldTriangle;
	friend class MeshBaryTriangle;
	friend class MeshQuadrilateral;

	static Shape* CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);

	class BaryMesh {
	public:
		static Shape* CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);
	};

	class WaldMesh {
	public:
		static Shape* CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);
	};

	class LoopMesh {
	public:
		static Shape* CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);
	};

protected:
	// Lotus - refinement data
	MeshAccelType accelType;

	// Dade - vertices data
	int nverts;
	Point *p; // in world space if no subdivision is needed, object space otherwise
	Normal *n; // in object space
	float *uvs;

	// Dade - triangle data
	MeshTriangleType triType;
	int ntris;
	int *triVertexIndex;

	// Dade - quad data
	MeshQuadType quadType;
	int nquads;
	int *quadVertexIndex;

	// Lotus - subdivision data
	bool mustSubdivide;
	int nSubdivLevels;
	MeshSubdivType subdivType;
	// optional displacement map
	boost::shared_ptr<Texture<float> > displacementMap;
	float displacementMapScale;
	float displacementMapOffset;
	bool displacementMapNormalSmooth, displacementMapSharpBoundary;
};

//------------------------------------------------------------------------------
// Triangle shapes
//------------------------------------------------------------------------------

class MeshBaryTriangle : public Primitive {
public:
    // BaryTriangle Public Methods
    MeshBaryTriangle(const Mesh *m, int n)
		: mesh(m), v(&(mesh->triVertexIndex[3 * n]))
	{
    }

    BBox ObjectBound() const;
    BBox WorldBound() const;

    bool CanIntersect() const { return true; }
    virtual bool Intersect(const Ray &ray, Intersection *isect) const;
    virtual bool IntersectP(const Ray &ray) const;

    void GetShadingGeometry(const Transform &obj2world,
            const DifferentialGeometry &dg,
            DifferentialGeometry *dgShading) const;

    bool CanSample() const { return true; }
    float Area() const;
    void Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const;

	virtual bool isDegenerate() const {
		return false; //TODO check degenerate
	}

	void GetUVs(float uv[3][2]) const {
		if (mesh->uvs) {
			uv[0][0] = mesh->uvs[2*v[0]];
			uv[0][1] = mesh->uvs[2*v[0]+1];
			uv[1][0] = mesh->uvs[2*v[1]];
			uv[1][1] = mesh->uvs[2*v[1]+1];
			uv[2][0] = mesh->uvs[2*v[2]];
			uv[2][1] = mesh->uvs[2*v[2]+1];
		} else {
			uv[0][0] = .5f;//mesh->p[v[0]].x;
			uv[0][1] = .5f;//mesh->p[v[0]].y;
			uv[1][0] = .5f;//mesh->p[v[1]].x;
			uv[1][1] = .5f;//mesh->p[v[1]].y;
			uv[2][0] = .5f;//mesh->p[v[2]].x;
			uv[2][1] = .5f;//mesh->p[v[2]].y;
		}
	}
	const Point &GetP(u_int i) const { return mesh->p[v[i]]; }

	// BaryTriangle Data
	const Mesh *mesh;
	const int *v;
};

class MeshWaldTriangle : public MeshBaryTriangle {
public:
	// WaldTriangle Public Methods
	MeshWaldTriangle(const Mesh *m, int n);

	bool Intersect(const Ray &ray, Intersection *isect) const;
	bool IntersectP(const Ray &ray) const;

	void Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const;
	bool isDegenerate() const;
private:
	// WaldTriangle Data

	// Dade - Wald's precomputed values
	enum IntersectionType {
		DOMINANT_X,
		DOMINANT_Y,
		DOMINANT_Z,
		DEGENERATE
	};
	IntersectionType intersectionType;
	float nu, nv, nd;
	float bnu, bnv, bnd;
	float cnu, cnv, cnd;

	// Dade - precomputed values for filling the DifferentialGeometry
	Vector dpdu, dpdv;
	Normal normalizedNormal;
};

//------------------------------------------------------------------------------
// Quad shapes
//------------------------------------------------------------------------------

// Quadrilateral Declarations
// assumes points form a strictly convex, planar quad
class MeshQuadrilateral : public Primitive {
public:
	// Quadrilateral Public Methods
	MeshQuadrilateral(const Mesh *m, int n);

	BBox ObjectBound() const;
	BBox WorldBound() const;

	bool CanIntersect() const { return true; }
	bool Intersect(const Ray &ray, Intersection *isect) const;
	bool IntersectP(const Ray &ray) const;

	void GetShadingGeometry(const Transform &obj2world,
            const DifferentialGeometry &dg,
            DifferentialGeometry *dgShading) const;

	bool CanSample() const { return true; }
	float Area() const;
	void Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const {
		const Point &p0 = mesh->p[idx[0]];
		const Point &p1 = mesh->p[idx[1]];
		const Point &p2 = mesh->p[idx[2]];
	    const Point &p3 = mesh->p[idx[3]];

	    float b0 = (1.f-u1)*(1.f-u2);
	    float b1 = u1*(1.f-u2);
	    float b2 = u1*u2;
	    float b3 = (1.f-u1)*u2;

		dg->p = b0*p0 + b1*p1 +b2*p2 + b3*p3;

		Vector e0 = p1 - p0;
		Vector e1 = p2 - p0;

		dg->nn = Normalize(Normal(Cross(e0, e1)));
		if (mesh->reverseOrientation ^ mesh->transformSwapsHandedness)
			dg->nn = -dg->nn;
		CoordinateSystem(Vector(dg->nn), &dg->dpdu, &dg->dpdv);

		float uv[4][2];
		GetUVs(uv);
		dg->u = b0*uv[0][0] + b1*uv[1][0] + b2*uv[2][0] + b3*uv[3][0];
		dg->v = b0*uv[0][1] + b1*uv[1][1] + b2*uv[2][1] + b3*uv[3][1];
	}

	bool isDegenerate() const {
		return false; //TODO check degenerate
	}

	static bool IsPlanar(const Point &p0, const Point &p1, const Point &p2, const Point &p3);
	static bool IsDegenerate(const Point &p0, const Point &p1, const Point &p2, const Point &p3);
	static bool IsConvex(const Point &p0, const Point &p1, const Point &p2, const Point &p3);

private:
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
	const Mesh *mesh;
	const int *idx;
};

}//namespace lux

