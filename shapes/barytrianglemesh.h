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

// barytrianglemesh.cpp*
#include "shape.h"
#include "paramset.h"
#include "mc.h"

namespace lux {

// BaryTriangleMesh Declarations
class BaryTriangleMesh : public Shape {
public:
    // BaryTriangleMesh Public Methods
    BaryTriangleMesh(const Transform &o2w, bool ro,
            int ntris, int nverts, const int *vptr,
            const Point *P, const Normal *N,
            const Vector *S, const float *uv);
    ~BaryTriangleMesh();
    BBox ObjectBound() const;
    BBox WorldBound() const;
    bool CanIntersect() const { return false; }
    void Refine(vector<boost::shared_ptr<Primitive> > &refined,
    		const PrimitiveRefinementHints& refineHints,
    		boost::shared_ptr<Primitive> thisPtr);
    bool CanSample() const { return false; }
    friend class BaryTriangle;
    template <class T> friend class VertexTexture;

    static Shape* CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);
protected:
    // BaryTriangleMesh Data
    int ntris, nverts;
    int *vertexIndex;
    Point *p;
    Normal *n;
    Vector *s;
    float *uvs;
    vector<boost::shared_ptr<Shape> > triPtrs;
};

class BaryTriangle : public Primitive {
public:
    // BaryTriangle Public Methods
    BaryTriangle(BaryTriangleMesh *m, int n) {
        mesh = m;
        v = &mesh->vertexIndex[3*n];
        // Update created triangles stats
        // radiance - disabled for threading // static StatsCounter trisMade("Geometry","Triangles created");
        // radiance - disabled for threading // ++trisMade;
    }
    BBox ObjectBound() const;
    BBox WorldBound() const;

    bool CanIntersect() const { return true; }
    bool Intersect(const Ray &ray, Intersection* isect) const;
    bool IntersectP(const Ray &ray) const;

    void GetShadingGeometry(const Transform &obj2world,
            const DifferentialGeometry &dg,
            DifferentialGeometry *dgShading) const;

    bool CanSample() const { return true; }
    float Area() const;
    Point Sample(float u1, float u2, float u3, Normal *Ns) const;

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
    BaryTriangleMesh* mesh;
    int *v;
};

}//namespace lux
