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

// loopsubdiv.h*
#include "shape.h"
#include "texture.h"
#include "error.h"
#include <set>
#include <map>
using std::set;
using std::map;
// LoopSubdiv Macros
#define NEXT(i) (((i)+1)%3)
#define PREV(i) (((i)+2)%3)

namespace lux
{

// LoopSubdiv Local Structures
struct SDFace;
struct SDVertex {
	// SDVertex Constructor
	SDVertex(Point pt = Point(0,0,0), float uu = 0.0f, float vv = 0.0f)
		: P(pt), u(uu), v(vv), startFace(NULL), child(NULL),
		regular(false), boundary(false) {
	}

	// SDVertex Methods
	u_int valence() const;
	void oneRing(Point *P) const;

	Point P;
	float u, v;
	SDFace *startFace;
	SDVertex *child;
	bool regular, boundary;
};

struct SDFace {
	// SDFace Constructor
	SDFace() {
		for (u_int i = 0; i < 3; ++i) {
			v[i] = NULL;
			f[i] = NULL;
		}
		for (u_int i = 0; i < 4; ++i)
			children[i] = NULL;
	}
	// SDFace Methods
	u_int vnum(const Point &p) const {
		for (u_int i = 0; i < 3; ++i) {
			if (v[i]->P == p)
				return i;
		}
		LOG(LUX_SEVERE,LUX_BUG)<<"Basic logic error in SDFace::vnum()";
		return 0;
	}
	SDFace *nextFace(const Point &p) const {
		return f[vnum(p)];
	}
	SDFace *prevFace(const Point &p) const {
		return f[PREV(vnum(p))];
	}
	SDVertex *nextVert(const Point &p) const {
		return v[NEXT(vnum(p))];
	}
	SDVertex *prevVert(const Point &p) const {
		return v[PREV(vnum(p))];
	}
	SDVertex *otherVert(const Point &p0, const Point &p1) const {
		for (u_int i = 0; i < 3; ++i) {
			if (v[i]->P != p0 && v[i]->P != p1)
				return v[i];
		}
		LOG(LUX_SEVERE,LUX_BUG)<<"Basic logic error in SDVertex::otherVert()";
		return NULL;
	}
	SDVertex *v[3];
	SDFace *f[3];
	SDFace *children[4];
};

struct SDEdge {
	// SDEdge Constructor
	SDEdge(const Point &p0, const Point &p1) {
		if (PInf(p0, p1)) {
			p[0] = p0;
			p[1] = p1;
		} else {
			p[0] = p1;
			p[1] = p0;
		}
		f[0] = f[1] = NULL;
		f0edgeNum = 0;
	}
	// SDEdge Comparison Function
	bool PInf(const Point &p1, const Point &p2) const {
		if (p1.x == p2.x)
			return p1.y == p2.y ? p1.z < p2.z : p1.y < p2.y;
		return p1.x < p2.x;
	}
	bool operator<(const SDEdge &e2) const {
		if (p[0] == e2.p[0]) return PInf(p[1], e2.p[1]);
		return PInf(p[0], e2.p[0]);
	}
	Point p[2];
	SDFace *f[2];
	u_int f0edgeNum;
};

// LoopSubdiv Declarations
class LoopSubdiv : public Shape {
public:
	// LoopSubdiv Public Methods
	LoopSubdiv(const Transform &o2w, bool ro,
			u_int nt, u_int nv, const int *vi,
			const Point *P, const float *uv, u_int nlevels,
			const boost::shared_ptr<Texture<float> > &dismap,
			float dmscale, float dmoffset,
			bool dmnormalsmooth, bool dmsharpboundary);
	virtual ~LoopSubdiv();
	virtual bool CanIntersect() const;
	virtual void Refine(vector<boost::shared_ptr<Shape> > &refined) const;
	virtual BBox ObjectBound() const;
	virtual BBox WorldBound() const;

	static Shape *CreateShape(const Transform &o2w, bool reverseOrientation,
			const ParamSet &params);

	class SubdivResult {
	public:
		SubdivResult(u_int aNtris, u_int aNverts, const int* aIndices,
			const Point *aP, const Normal *aN, const float *aUv)
			: ntris(aNtris), nverts(aNverts), indices(aIndices),
			P(aP), N(aN), uv(aUv)
		{
		}
		~SubdivResult() {
			delete[] indices;
			delete[] P;
			delete[] N;
			delete[] uv;
		}

		const u_int ntris;
		const u_int nverts;

		const int * const indices;
		const Point * const P;
		const Normal * const N;
		const float * const uv;
	};
	boost::shared_ptr<SubdivResult> Refine() const;

private:
	// LoopSubdiv Private Methods
	float beta(u_int valence) const {
		if (valence == 3) return 3.f/16.f;
		else return 3.f / (8.f * valence);
	}
	void weightOneRing(SDVertex *destVert, SDVertex *vert, float beta) const ;
	void weightBoundary(SDVertex *destVert, SDVertex *vert, float beta) const;
	float gamma(u_int valence) const {
		return 1.f / (valence + 3.f / (8.f * beta(valence)));
	}
	static void GenerateNormals(const vector<SDVertex *> verts, Normal *Ns);

	void ApplyDisplacementMap(const vector<SDVertex *> verts,
		const Normal *norms) const;

	// LoopSubdiv Private Data
	u_int nLevels;
	vector<SDVertex *> vertices;
	vector<SDFace *> faces;

	// Dade - optional displacement map
	boost::shared_ptr<Texture<float> > displacementMap;
	float displacementMapScale;
	float displacementMapOffset;

	bool hasUV, displacementMapNormalSmooth, displacementMapSharpBoundary;

	// Lotus - a pointer to the refined mesh to avoid double refinement or deletion
	mutable boost::shared_ptr<Shape> refinedShape;
};

// LoopSubdiv Inline Functions
inline u_int SDVertex::valence() const {
	SDFace *f = startFace;
	if (!boundary) {
		// Compute valence of interior vertex
		u_int nf = 0;
		do {
			SDVertex *v = f->v[f->vnum(P)];
			if (v->startFace != startFace)
				v->startFace = startFace;
			++nf;
			f = f->nextFace(v->P);
		} while (f != startFace);
		return nf;
	} else {
		// Compute valence of boundary vertex
		u_int nf = 0;
		while (f) {
			SDVertex *v = f->v[f->vnum(P)];
			if (v->startFace != startFace)
				v->startFace = startFace;
			++nf;
			f = f->nextFace(v->P);
		}

		f = startFace;
		while (f) {
			SDVertex *v = f->v[f->vnum(P)];
			if (v->startFace != startFace)
				v->startFace = startFace;
			++nf;
			f = f->prevFace(v->P);
		}
		return nf;
	}
}

}//namespace lux

