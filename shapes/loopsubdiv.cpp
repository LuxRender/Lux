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

// loopsubdiv.cpp*
#include "loopsubdiv.h"
#include "context.h"
#include "paramset.h"
#include "dynload.h"

#include <boost/pool/object_pool.hpp>

using namespace lux;

// LoopSubdiv Method Definitions
LoopSubdiv::LoopSubdiv(const Transform &o2w, bool ro,
        int nfaces, int nvertices,
		const int *vertexIndices,
		const Point *P,
		const float *uv,
		int nl,
		const boost::shared_ptr<Texture<float> > dismap,
		float dmscale, float dmoffset,
		bool dmnormalsmooth, bool dmsharpboundary)
	: Shape(o2w, ro), displacementMap(dismap), displacementMapScale(dmscale),
	displacementMapOffset(dmoffset), displacementMapNormalSmooth(dmnormalsmooth),
	displacementMapSharpBoundary(dmsharpboundary) {
	nLevels = nl;
	hasUV = (uv != NULL);

	// Allocate _LoopSubdiv_ vertices and faces
	int i;
	SDVertex *verts = new SDVertex[nvertices];
	for (i = 0; i < nvertices; ++i) {
		if (hasUV)
			verts[i] = SDVertex(P[i], uv[2 * i], uv[2 * i + 1]);
		else
			verts[i] = SDVertex(P[i]);

		vertices.push_back(&verts[i]);
	}

	SDFace *fs = new SDFace[nfaces];
	for (i = 0; i < nfaces; ++i)
		faces.push_back(&fs[i]);
	// Set face to vertex pointers
	const int *vp = vertexIndices;
	for (i = 0; i < nfaces; ++i) {
		SDFace *f = faces[i];
		for (int j = 0; j < 3; ++j) {
			SDVertex *v = vertices[vp[j]];
			f->v[j] = v;
			v->startFace = f;
		}
		vp += 3;
	}

	// Set neighbor pointers in _faces_
	set<SDEdge> edges;
	for (i = 0; i < nfaces; ++i) {
		SDFace *f = faces[i];
		for (int edgeNum = 0; edgeNum < 3; ++edgeNum) {
			// Update neighbor pointer for _edgeNum_
			int v0 = edgeNum, v1 = NEXT(edgeNum);
			SDEdge e(f->v[v0], f->v[v1]);
			if (edges.find(e) == edges.end()) {
				// Handle new edge
				e.f[0] = f;
				e.f0edgeNum = edgeNum;
				edges.insert(e);
			}
			else {
				// Handle previously-seen edge
				e = *edges.find(e);
				e.f[0]->f[e.f0edgeNum] = f;
				f->f[edgeNum] = e.f[0];
				// NOTE - lordcrc - check winding of 
				// other face is opposite of the 
				// current face, otherwise we have 
				// inconsistent winding
				int otherv0 = e.f[0]->vnum(f->v[v0]);
				int otherv1 = e.f[0]->vnum(f->v[v1]);
				if (PREV(otherv0) != otherv1) {
					luxError(LUX_CONSISTENCY, LUX_ERROR, "Inconsistent vertex winding in mesh, aborting subdivision.");
					// prevent subdivision
					nLevels = 0;
					return;
				};
				edges.erase(e);
			}
		}
	}

	// Finish vertex initialization
	for (i = 0; i < nvertices; ++i) {
		SDVertex *v = vertices[i];
		SDFace *f = v->startFace;
		do {
			f = f->nextFace(v);
		} while (f && f != v->startFace);
		v->boundary = (f == NULL);
		if (!v->boundary && v->valence() == 6)
			v->regular = true;
		else if (v->boundary && v->valence() == 4)
			v->regular = true;
		else
			v->regular = false;
	}
}

LoopSubdiv::~LoopSubdiv() {
	delete[] vertices[0];
	delete[] faces[0];
}

BBox LoopSubdiv::ObjectBound() const {
	// Dade - todo: the bbox returned doesn't include the effect of displacement map
	BBox b;
	for (u_int i = 0; i < vertices.size(); i++)
		b = Union(b, vertices[i]->P);
	return b;
}

BBox LoopSubdiv::WorldBound() const {
	// Dade - todo: the bbox returned doesn't include the effect of displacement map
	BBox b;
	for (u_int i = 0; i < vertices.size(); i++)
		b = Union(b, ObjectToWorld(vertices[i]->P));
	return b;
}

bool LoopSubdiv::CanIntersect() const {
	return false;
}

boost::shared_ptr<LoopSubdiv::SubdivResult> LoopSubdiv::Refine() const {

	// check that we should do any subdivision
	if (nLevels < 1) {
		return boost::shared_ptr<LoopSubdiv::SubdivResult>((LoopSubdiv::SubdivResult*)NULL);
	}

	std::stringstream ss;
	ss << "Applying " << nLevels << " levels of loop subdivision to " << faces.size() << " triangles";
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	vector<SDFace *> f = faces;
	vector<SDVertex *> v = vertices;
	boost::object_pool<SDVertex> vertexArena;
	boost::object_pool<SDFace> faceArena;

	for (int i = 0; i < nLevels; ++i) {
		// Update _f_ and _v_ for next level of subdivision
		vector<SDFace *> newFaces;
		vector<SDVertex *> newVertices;

		// Allocate next level of children in mesh tree
		for (u_int j = 0; j < v.size(); ++j) {
			v[j]->child = vertexArena.malloc();//new (vertexArena) SDVertex;
			v[j]->child->regular = v[j]->regular;
			v[j]->child->boundary = v[j]->boundary;
			newVertices.push_back(v[j]->child);
		}
		for (u_int j = 0; j < f.size(); ++j)
			for (int k = 0; k < 4; ++k) {
				f[j]->children[k] = faceArena.malloc();//new (faceArena) SDFace;
				newFaces.push_back(f[j]->children[k]);
			}

		// Update vertex positions and create new edge vertices
		// Update vertex positions for even vertices
		for (u_int j = 0; j < v.size(); ++j) {
			if (!v[j]->boundary) {
				// Apply one-ring rule for even vertex
				if (v[j]->regular)
					weightOneRing(v[j]->child, v[j], 1.f/16.f);
				else
					weightOneRing(v[j]->child, v[j], beta(v[j]->valence()));
			} else {
				// Apply boundary rule for even vertex
				weightBoundary(v[j]->child, v[j], 1.f/8.f);
			}
		}

		// Compute new odd edge vertices
		map<SDEdge, SDVertex *> edgeVerts;
		for (u_int j = 0; j < f.size(); ++j) {
			SDFace *face = f[j];
			for (int k = 0; k < 3; ++k) {
				// Compute odd vertex on _k_th edge
				SDEdge edge(face->v[k], face->v[NEXT(k)]);
				SDVertex *vert = edgeVerts[edge];
				if (!vert) {
					// Create and initialize new odd vertex
					vert = vertexArena.malloc();//new (vertexArena) SDVertex;
					newVertices.push_back(vert);
					vert->regular = true;
					vert->boundary = (face->f[k] == NULL);
					vert->startFace = face->children[3];
					// Apply edge rules to compute new vertex position
					if (vert->boundary) {
						vert->P =  0.5f * edge.v[0]->P;
						vert->P += 0.5f * edge.v[1]->P;

						vert->u = 0.5f * (edge.v[0]->u + edge.v[1]->u);
						vert->v = 0.5f * (edge.v[0]->v + edge.v[1]->v);
					} else {
						vert->P =  3.f/8.f * edge.v[0]->P;
						vert->P += 3.f/8.f * edge.v[1]->P;
						SDVertex *ov1 = face->otherVert(edge.v[0], edge.v[1]);
						vert->P += 1.f/8.f * ov1->P;
						SDVertex *ov2 = face->f[k]->otherVert(edge.v[0], edge.v[1]);
						vert->P += 1.f/8.f * ov2->P;

						vert->u = 3.f/8.f * edge.v[0]->u;
						vert->u += 3.f/8.f * edge.v[1]->u;
						vert->u += 1.f/8.f * ov1->u;
						vert->u += 1.f/8.f * ov2->u;

						vert->v = 3.f/8.f * edge.v[0]->v;
						vert->v += 3.f/8.f * edge.v[1]->v;
						vert->v += 1.f/8.f * ov1->v;
						vert->v += 1.f/8.f * ov2->v;
					}
					edgeVerts[edge] = vert;
				}
			}
		}

		// Update new mesh topology
		// Update even vertex face pointers
		for (u_int j = 0; j < v.size(); ++j) {
			SDVertex *vert = v[j];
			int vertNum = vert->startFace->vnum(vert);
			vert->child->startFace =
			    vert->startFace->children[vertNum];
		}

		// Update face neighbor pointers
		for (u_int j = 0; j < f.size(); ++j) {
			SDFace *face = f[j];
			for (int k = 0; k < 3; ++k) {
				// Update children _f_ pointers for siblings
				face->children[3]->f[k] = face->children[NEXT(k)];
				face->children[k]->f[NEXT(k)] = face->children[3];
				// Update children _f_ pointers for neighbor children
				SDFace *f2 = face->f[k];
				face->children[k]->f[k] =
					f2 ? f2->children[f2->vnum(face->v[k])] : NULL;
				f2 = face->f[PREV(k)];
				face->children[k]->f[PREV(k)] =
					f2 ? f2->children[f2->vnum(face->v[k])] : NULL;
			}
		}

		// Update face vertex pointers
		for (u_int j = 0; j < f.size(); ++j) {
			SDFace *face = f[j];
			for (int k = 0; k < 3; ++k) {
				// Update child vertex pointer to new even vertex
				face->children[k]->v[k] = face->v[k]->child;
				// Update child vertex pointer to new odd vertex
				SDVertex *vert =
					edgeVerts[SDEdge(face->v[k], face->v[NEXT(k)])];
				face->children[k]->v[NEXT(k)] = vert;
				face->children[NEXT(k)]->v[k] = vert;
				face->children[3]->v[k] = vert;
			}
		}

		// Prepare for next level of subdivision
		f = newFaces;
		v = newVertices;
	}

	// Push vertices to limit surface
	SDVertex *Vlimit = new SDVertex[v.size()];
	for (u_int i = 0; i < v.size(); ++i) {
		if (v[i]->boundary)
			weightBoundary(&Vlimit[i], v[i], 1.f/5.f);
		else
			weightOneRing(&Vlimit[i], v[i], gamma(v[i]->valence()));
	}
	for (u_int i = 0; i < v.size(); ++i) {
		v[i]->P = Vlimit[i].P;
		v[i]->u = Vlimit[i].u;
		v[i]->v = Vlimit[i].v;
	}
	delete[] Vlimit;

	// Create _TriangleMesh_ from subdivision mesh
	u_int ntris = u_int(f.size());
	u_int nverts = u_int(v.size());
	int *verts = new int[3*ntris];
	int *vp = verts;
	map<SDVertex *, int> usedVerts;
	for (u_int i = 0; i < nverts; ++i)
		usedVerts[v[i]] = i;
	for (u_int i = 0; i < ntris; ++i) {
		for (int j = 0; j < 3; ++j) {
			*vp = usedVerts[f[i]->v[j]];
			++vp;
		}
	}

	// Dade - calculate normals
	Normal* Ns = new Normal[ nverts ];
	GenerateNormals(v, Ns);

	// Dade - calculate vertex UVs if required
	float *UVlimit = NULL;
	if (hasUV) {
		UVlimit = new float[2 * nverts];
		for (u_int i = 0; i < nverts; ++i) {
			UVlimit[2 * i] = v[i]->u;
			UVlimit[2 * i + 1] = v[i]->v;
		}
	}

	ss.str("");
	ss << "Subdivision complete, got " << ntris << " triangles";
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	if (displacementMap.get() != NULL) {
		// Dade - apply the displacement map

		ApplyDisplacementMap(v, Ns, UVlimit);

		if (displacementMapNormalSmooth) {
			// Dade - recalculate normals after the displacement
			GenerateNormals(v, Ns);
		}
	}

	// Dade - create trianglemesh vertices
	Point *Plimit = new Point[nverts];
	for (u_int i = 0; i < nverts; ++i)
		Plimit[i] = v[i]->P;

	if( !displacementMapNormalSmooth ) {
		delete[] Ns;
		Ns = NULL;
	}

	return boost::shared_ptr<SubdivResult>(new SubdivResult(ntris, nverts, verts, Plimit, Ns, UVlimit));
}

void LoopSubdiv::Refine(vector<boost::shared_ptr<Shape> > &refined) const {
	if(refinedShape) {
		refined.push_back(refinedShape);
		return;
	}

	ParamSet paramSet;

	{
		boost::shared_ptr<SubdivResult> res = Refine();

		paramSet.AddInt("indices", res->indices, 3 * res->ntris);
		paramSet.AddPoint("P", res->P, res->nverts);
		paramSet.AddNormal("N", res->N, res->nverts);
		if (res->uv)
			paramSet.AddFloat("uv", res->uv, 2 * res->nverts);
	}
	
	this->refinedShape = MakeShape("trianglemesh", ObjectToWorld,
		reverseOrientation, paramSet);
	refined.push_back(this->refinedShape);
}

void LoopSubdiv::GenerateNormals(const vector<SDVertex *> v, Normal *Ns) {
	// Compute vertex tangents on limit surface
	int ringSize = 16;
	Point *Pring = new Point[ringSize];
	for (u_int i = 0; i < v.size(); ++i) {
		SDVertex *vert = v[i];
		Vector S(0,0,0), T(0,0,0);
		int valence = vert->valence();
		if (valence > ringSize) {
			ringSize = valence;
			delete[] Pring;
			Pring = new Point[ringSize];
		}
		vert->oneRing(Pring);
	
		if (!vert->boundary) {
			// Compute tangents of interior face
			for (int k = 0; k < valence; ++k) {
				S += cosf(2.f*M_PI*k/valence) * Vector(Pring[k]);
				T += sinf(2.f*M_PI*k/valence) * Vector(Pring[k]);
			}
		} else {
			// Compute tangents of boundary face
			S = Pring[valence-1] - Pring[0];
			if (valence == 2)
				T = Vector(Pring[0] + Pring[1] - 2 * vert->P);
			else if (valence == 3)
				T = Pring[1] - vert->P;
			else if (valence == 4) // regular
				T = Vector(-1*Pring[0] + 2*Pring[1] + 2*Pring[2] +
					-1*Pring[3] + -2*vert->P);
			else {
				float theta = M_PI / float(valence-1);
				T = Vector(sinf(theta) * (Pring[0] + Pring[valence-1]));
				for (int k = 1; k < valence-1; ++k) {
					float wt = (2*cosf(theta) - 2) * sinf((k) * theta);
					T += Vector(wt * Pring[k]);
				}
				T = -T;
			}
		}
		Ns[i] = Normal(Cross(T, S));
	}
}

void LoopSubdiv::ApplyDisplacementMap(
		const vector<SDVertex *> verts,
		const Normal *norms,
		const float *uvs) const {
	// Dade - apply the displacement map
	std::stringstream ss;
	ss << "Applying displacement map to " << verts.size() << " vertices";
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	for (u_int i = 0; i < verts.size(); i++) {
		Point pp = ObjectToWorld(verts[i]->P);
		Normal nn = Normalize(norms[i]);	
		Vector dpdu, dpdv;
		CoordinateSystem(Vector(nn), &dpdu, &dpdv);

		float u, v;
		if (uvs != NULL) {
			u = uvs[2 * i];
			v = uvs[2 * i + 1];
		} else {
			u = pp.x;
			v = pp.y;
		}
	
		DifferentialGeometry dg = DifferentialGeometry(
				pp,
				nn,
				dpdu, dpdv,
				Normal(0, 0, 0), Normal(0, 0, 0),
				u, v, this);

		Vector displacement(nn);
		displacement *=	- (
				displacementMap.get()->Evaluate(NULL, dg) * displacementMapScale +
				displacementMapOffset);

		verts[i]->P += displacement;
	}
}

void LoopSubdiv::weightOneRing(SDVertex *destVert, SDVertex *vert, float beta) const {
	// Put _vert_ one-ring in _Pring_
	int valence = vert->valence();
	SDVertex **Vring = (SDVertex **)alloca(valence * sizeof(SDVertex *));
	vert->oneRing(Vring);

	Point P = (1 - valence * beta) * vert->P;
	float u = (1 - valence * beta) * vert->u;
	float v = (1 - valence * beta) * vert->v;

	for (int i = 0; i < valence; ++i) {
		P += beta * Vring[i]->P;
		u += beta * Vring[i]->u;
		v += beta * Vring[i]->v;
	}

	destVert->P = P;
	destVert->u = u;
	destVert->v = v;
}

void SDVertex::oneRing(SDVertex **V) {
	if (!boundary) {
		// Get one ring vertices for interior vertex
		SDFace *face = startFace;
		do {
			*V = face->nextVert(this);
			V++;
			face = face->nextFace(this);
		} while (face != startFace);
	} else {
		// Get one ring vertices for boundary vertex
		SDFace *face = startFace, *f2;
		while ((f2 = face->nextFace(this)) != NULL)
			face = f2;
		*V = face->nextVert(this);
		V++;
		do {
			*V = face->prevVert(this);
			V++;
			face = face->prevFace(this);
		} while (face != NULL);
	}
}

void SDVertex::oneRing(Point *P) {
	if (!boundary) {
		// Get one ring vertices for interior vertex
		SDFace *face = startFace;
		do {
			*P++ = face->nextVert(this)->P;
			face = face->nextFace(this);
		} while (face != startFace);
	} else {
		// Get one ring vertices for boundary vertex
		SDFace *face = startFace, *f2;
		while ((f2 = face->nextFace(this)) != NULL)
			face = f2;
		*P++ = face->nextVert(this)->P;
		do {
			*P++ = face->prevVert(this)->P;
			face = face->prevFace(this);
		} while (face != NULL);
	}
}

void LoopSubdiv::weightBoundary(SDVertex *destVert,  SDVertex *vert,
                                 float beta) const {
	// Put _vert_ one-ring in _Pring_
	int valence = vert->valence();
	SDVertex **Vring = (SDVertex **)alloca(valence * sizeof(SDVertex *));
	vert->oneRing(Vring);

	if(!displacementMapSharpBoundary) {
		Point P = (1 - 2 * beta) * vert->P;
		P += beta * Vring[0]->P;
		P += beta * Vring[valence - 1]->P;
		destVert->P = P;
	} else
		destVert->P = vert->P;

	float u = (1 - 2 * beta) * vert->u;
	float v = (1 - 2 * beta) * vert->v;
	u += beta * Vring[0]->u;
	v += beta * Vring[0]->v;
	u += beta * Vring[valence - 1]->u;
	v += beta * Vring[valence - 1]->v;

	destVert->u = u;
	destVert->v = v;
}

Shape *LoopSubdiv::CreateShape(
		const Transform &o2w,
		bool reverseOrientation,
		const ParamSet &params)
{
	map<string, boost::shared_ptr<Texture<float> > > *floatTextures = Context::getActiveFloatTextures();
	int nlevels = params.FindOneInt("nlevels", 3);
	u_int nps, nIndices, nuvi;
	const int *vi = params.FindInt("indices", &nIndices);
	const Point *P = params.FindPoint("P", &nps);
	if (!vi || !P) return NULL;

	const float *uvs = params.FindFloat("uv", &nuvi);

	// Dade - the optional displacement map
	string displacementMapName = params.FindOneString("displacementmap", "");
	float displacementMapScale = params.FindOneFloat("dmscale", 0.1f);
	float displacementMapOffset = params.FindOneFloat("dmoffset", 0.0f);
	bool displacementMapNormalSmooth = params.FindOneBool("dmnormalsmooth", true);
	bool displacementMapSharpBoundary = params.FindOneBool("dmsharpboundary", false);

	boost::shared_ptr<Texture<float> > displacementMap;
	if (displacementMapName != "") {
		displacementMap = (*floatTextures)[displacementMapName];

		if (displacementMap.get() == NULL) {
            std::stringstream ss;
            ss << "Unknown float texture '" << displacementMapName << "' in a LoopSubdiv shape.";
            luxError(LUX_SYNTAX, LUX_WARNING, ss.str().c_str());
		}
	}

	// don't actually use this for now...
	string scheme = params.FindOneString("scheme", "loop");

	return new LoopSubdiv(o2w, reverseOrientation, nIndices/3, nps,
		vi, P, uvs, nlevels, displacementMap,
		displacementMapScale, displacementMapOffset,
		displacementMapNormalSmooth, displacementMapSharpBoundary);
}

// Lotus - Handled by mesh shape
//static DynamicLoader::RegisterShape<LoopSubdiv> r("loopsubdiv");
