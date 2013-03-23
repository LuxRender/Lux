/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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
#include "spectrumwavelengths.h"
#include "geometry/raydifferential.h"
#include "shape.h"

#include <boost/pool/object_pool.hpp>

using namespace lux;

// LoopSubdiv Method Definitions
LoopSubdiv::LoopSubdiv(u_int nfaces, u_int nvertices, const int *vertexIndices,
	const Point *P, const float *uv, const Normal *n,
	const float *cols, const float *alphas, u_int nl,
	const boost::shared_ptr<Texture<float> > &dismap, float dmscale,
	float dmoffset, bool dmnormalsmooth, bool dmsharpboundary,
	bool normalsplit, const string &sname)
	: displacementMap(dismap),
	displacementMapScale(dmscale), displacementMapOffset(dmoffset),
	displacementMapNormalSmooth(dmnormalsmooth),
	displacementMapSharpBoundary(dmsharpboundary),
	name(sname)
{
	nLevels = nl;
	hasUV = (uv != NULL);
	hasCol = (cols != NULL);
	hasAlpha = (alphas != NULL);
	normalSplit = normalsplit && n != NULL;

	// Allocate _LoopSubdiv_ vertices and faces
	SDVertex *verts = new SDVertex[nvertices];
	vertices.reserve(nvertices);
	for (u_int i = 0; i < nvertices; ++i) {
		verts[i] = SDVertex(P[i],
				hasUV ? uv[2 * i] : 0.f,
				hasUV ? uv[2 * i + 1] : 0.f,
				normalSplit ? n[i] : Normal(0.f, 0.f, 0.f),
				hasCol ? RGBColor(cols[3 * i], cols[3 * i + 1], cols[3 * i + 2]) : RGBColor(1.f),
				hasAlpha ? alphas[i] : 1.f);

		vertices.push_back(&verts[i]);
	}

	SDFace *fs = new SDFace[nfaces];
	faces.reserve(nfaces);
	for (u_int i = 0; i < nfaces; ++i)
		faces.push_back(&fs[i]);
	// Set face to vertex pointers
	const int *vp = vertexIndices;
	for (u_int i = 0; i < nfaces; ++i) {
		SDFace *f = faces[i];
		for (u_int j = 0; j < 3; ++j) {
			SDVertex *v = vertices[vp[j]];
			f->v[j] = v;
			f->f[j] = NULL;
			f->children[j] = NULL;
			v->startFace = f;
		}
		f->children[3] = NULL;
		vp += 3;
	}

	// Set neighbor pointers in _faces_
	set<SDEdge> edges;
	for (u_int i = 0; i < nfaces; ++i) {
		SDFace *f = faces[i];
		for (u_int edgeNum = 0; edgeNum < 3; ++edgeNum) {
			// Update neighbor pointer for _edgeNum_
			u_int v0 = edgeNum, v1 = NEXT(edgeNum);
			Normal n0, n1;
			SDEdge e(f->v[v0], f->v[v1]);
			if (edges.find(e) == edges.end()) {
				// Handle new edge
				e.f[0] = f;
				e.f0edgeNum = edgeNum;
				edges.insert(e);
			} else {
				// Handle previously-seen edge
				e = *edges.find(e);
				e.f[0]->f[e.f0edgeNum] = f;
				f->f[edgeNum] = e.f[0];
				// NOTE - lordcrc - check winding of 
				// other face is opposite of the 
				// current face, otherwise we have 
				// inconsistent winding
				u_int otherv0 = e.f[0]->vnum(f->v[v0]->P);
				u_int otherv1 = e.f[0]->vnum(f->v[v1]->P);
				if (PREV(otherv0) != otherv1) {
					SHAPE_LOG(name, LUX_ERROR,LUX_CONSISTENCY)<< "Inconsistent vertex winding in mesh, aborting subdivision.";
					// prevent subdivision
					nLevels = 0;
					return;
				};
				edges.erase(e);
			}
		}
	}

	// Finish vertex initialization
	for (u_int i = 0; i < nvertices; ++i) {
		SDVertex *v = vertices[i];
		SDFace *f = v->startFace;
		do {
			f = f->nextFace(v->P);
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

boost::shared_ptr<LoopSubdiv::SubdivResult> LoopSubdiv::Refine() const {

	// check that we should do any subdivision
	if (nLevels < 1) {
		return boost::shared_ptr<LoopSubdiv::SubdivResult>();
	}

	SHAPE_LOG(name, LUX_INFO,LUX_NOERROR) << "Applying " << nLevels << " levels of loop subdivision to " << faces.size() << " triangles";

	vector<SDFace *> f = faces;
	vector<SDVertex *> v = vertices;
	boost::object_pool<SDVertex> vertexArena;
	boost::object_pool<SDFace> faceArena;

	for (u_int i = 0; i < nLevels; ++i) {
		// Update _f_ and _v_ for next level of subdivision
		vector<SDFace *> newFaces;
		vector<SDVertex *> newVertices;

		// Allocate next level of children in mesh tree
		newVertices.reserve(v.size());
		for (u_int j = 0; j < v.size(); ++j) {
			v[j]->child = vertexArena.construct();//new (vertexArena) SDVertex;
			v[j]->child->regular = v[j]->regular;
			v[j]->child->boundary = v[j]->boundary;
			newVertices.push_back(v[j]->child);
		}
		newFaces.reserve(f.size());
		for (u_int j = 0; j < f.size(); ++j) {
			for (u_int k = 0; k < 4; ++k) {
				f[j]->children[k] = faceArena.construct();//new (faceArena) SDFace;
				newFaces.push_back(f[j]->children[k]);
			}
		}

		// Update vertex positions and create new edge vertices
		// Update vertex positions for even vertices
		for (u_int j = 0; j < v.size(); ++j) {
			SDVertex *vert = v[j];
			if (!vert->boundary) {
				// Apply one-ring rule for even vertex
				if (vert->regular)
					weightOneRing(vert->child, vert, 1.f/16.f);
				else
					weightOneRing(vert->child, vert, beta(vert->valence()));
			} else {
				// Apply boundary rule for even vertex
				weightBoundary(vert->child, vert, 1.f/8.f);
			}
			// Update even vertex face pointers
			const SDFace *sf = vert->startFace;
			vert->child->startFace = sf->children[sf->vnum(vert->P)];
		}

		// Compute new odd edge vertices
		// Update new mesh topology
		map<SDEdge, SDVertex *> edgeVerts;
		for (u_int j = 0; j < f.size(); ++j) {
			SDFace *face = f[j];
			for (u_int k = 0; k < 3; ++k) {
				// Update face neighbor pointers
				// Update children _f_ pointers for siblings
				face->children[3]->f[k] = face->children[NEXT(k)];
				face->children[k]->f[NEXT(k)] = face->children[3];
				// Update children _f_ pointers for neighbor children
				SDFace *f2 = face->f[PREV(k)];
				face->children[k]->f[PREV(k)] =
					f2 ? f2->children[f2->vnum(face->v[k]->P)] : NULL;
				f2 = face->f[k];
				face->children[k]->f[k] =
					f2 ? f2->children[f2->vnum(face->v[k]->P)] : NULL;
				// Update child vertex pointer to new even vertex
				face->children[k]->v[k] = face->v[k]->child;
				// Compute odd vertex on _k_th edge
				SDVertex *v0 = face->v[k], *v1 = face->v[NEXT(k)];
				SDEdge edge(v0, v1);
				SDVertex *vert = edgeVerts[edge];
				if (!vert) {
					edge.f[0] = face;
					edge.f0edgeNum = k;
					// Create and initialize new odd vertex
					vert = vertexArena.construct();//new (vertexArena) SDVertex;
					newVertices.push_back(vert);
					vert->regular = true;
					vert->boundary = (f2 == NULL);
					vert->startFace = face->children[3];
					// Apply edge rules to compute new vertex position
					if (vert->boundary) {
						vert->P =  0.5f * (v0->P + v1->P);

						vert->u = 0.5f * (v0->u + v1->u);
						vert->v = 0.5f * (v0->v + v1->v);
						vert->n = 0.5f * (v0->n + v1->n);
						vert->col = 0.5f * (v0->col + v1->col);
						vert->alpha = 0.5f * (v0->alpha + v1->alpha);
					} else {
						SDVertex *ov1 = face->v[PREV(k)];
						SDVertex *ov2 = f2->otherVert(edge.v[0]->P, edge.v[1]->P);
						vert->P =  3.f/8.f * (v0->P + v1->P);
						vert->P += 1.f/8.f * (ov1->P + ov2->P);

						// If UV are different on each side of the edge interpolate as boundary
						if (f2->v[f2->vnum(v0->P)]->u == v0->u &&
							f2->v[f2->vnum(v0->P)]->v == v0->v &&
							f2->v[f2->vnum(v0->P)]->col == v0->col &&
							f2->v[f2->vnum(v0->P)]->alpha == v0->alpha &&
							f2->v[f2->vnum(v1->P)]->u == v1->u &&
							f2->v[f2->vnum(v1->P)]->v == v1->v &&
							f2->v[f2->vnum(v1->P)]->col == v1->col &&
							f2->v[f2->vnum(v1->P)]->alpha == v1->alpha) {
							vert->u = 3.f/8.f * (v0->u + v1->u);
							vert->u += 1.f/8.f * (ov1->u + ov2->u);

							vert->v = 3.f/8.f * (v0->v + v1->v);
							vert->v += 1.f/8.f * (ov1->v + ov2->v);

							vert->col = 3.f/8.f * (v0->col + v1->col);
							vert->col += 1.f/8.f * (ov1->col + ov2->col);

							vert->alpha = 3.f/8.f * (v0->alpha + v1->alpha);
							vert->alpha += 1.f/8.f * (ov1->alpha + ov2->alpha);
						} else {
							vert->u = 0.5f * (v0->u + v1->u);
							vert->v = 0.5f * (v0->v + v1->v);
							vert->col = 0.5f * (v0->col + v1->col);
							vert->alpha = 0.5f * (v0->alpha + v1->alpha);
						}
						vert->n =  3.f/8.f * (v0->n + v1->n);
						vert->n += 1.f/8.f * (ov1->n + ov2->n);
					}
					edgeVerts[edge] = vert;
				} else {
					// If UV are different on each side of the edge create a new vertex
					if (!vert->boundary &&
						(f2->v[f2->vnum(v0->P)]->u != v0->u ||
						f2->v[f2->vnum(v0->P)]->v != v0->v ||
						f2->v[f2->vnum(v0->P)]->col != v0->col ||
						f2->v[f2->vnum(v0->P)]->alpha != v0->alpha ||
						f2->v[f2->vnum(v1->P)]->u != v1->u ||
						f2->v[f2->vnum(v1->P)]->v != v1->v ||
						f2->v[f2->vnum(v1->P)]->col != v1->col ||
						f2->v[f2->vnum(v1->P)]->alpha != v1->alpha)) {
						const Point &P(vert->P);
						const Normal &N(vert->n);
						SDFace *startFace = vert->startFace;
						vert = vertexArena.construct();//new (vertexArena) SDVertex;
						newVertices.push_back(vert);
						vert->regular = true;
						vert->boundary = false;
						vert->startFace = startFace;
						// Standard point interpolation
						vert->P = P;
						// Boundary interpolation for UV
						vert->u = 0.5f * (v0->u + v1->u);
						vert->v = 0.5f * (v0->v + v1->v);
						vert->n = N;
						vert->col = 0.5f * (v0->col + v1->col);
						vert->alpha = 0.5f * (v0->alpha + v1->alpha);
					}
					edgeVerts.erase(edge);
				}
				// Update face vertex pointers
				// Update child vertex pointer to new odd vertex
				face->children[k]->v[NEXT(k)] = vert;
				face->children[NEXT(k)]->v[k] = vert;
				face->children[3]->v[k] = vert;
			}
		}
		edgeVerts.clear();

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
		v[i]->n = Vlimit[i].n;
		v[i]->col = Vlimit[i].col;
		v[i]->alpha = Vlimit[i].alpha;
	}
	delete[] Vlimit;

	// Create _TriangleMesh_ from subdivision mesh
	u_int ntris = f.size();
	u_int nverts = v.size();
	int *verts = new int[3*ntris];
	int *vp = verts;
	map<SDVertex *, int> usedVerts;
	for (u_int i = 0; i < nverts; ++i)
		usedVerts[v[i]] = i;
	for (u_int i = 0; i < ntris; ++i) {
		for (u_int j = 0; j < 3; ++j) {
			*vp = usedVerts[f[i]->v[j]];
			++vp;
		}
	}

	// Dade - calculate vertex UVs if required
	float *UVLimit = NULL;
	if (hasUV) {
		UVLimit = new float[2 * nverts];
		for (u_int i = 0; i < nverts; ++i) {
			UVLimit[2 * i] = v[i]->u;
			UVLimit[2 * i + 1] = v[i]->v;
		}
	}

	// Dade - calculate vertex colors if required
	float *colLimit = NULL;
	if (hasCol) {
		colLimit = new float[3 * nverts];
		for (u_int i = 0; i < nverts; ++i) {
			colLimit[3 * i] = v[i]->col.c[0];
			colLimit[3 * i + 1] = v[i]->col.c[1];
			colLimit[3 * i + 2] = v[i]->col.c[2];
		}
	}

	// Dade - calculate vertex alphas if required
	float *alphaLimit = NULL;
	if (hasAlpha) {
		alphaLimit = new float[nverts];
		for (u_int i = 0; i < nverts; ++i)
			alphaLimit[i] = v[i]->alpha;
	}

	SHAPE_LOG(name, LUX_INFO,LUX_NOERROR) << "Subdivision complete, got " << ntris << " triangles";

	if (displacementMap) {
		// Dade - apply the displacement map
		GenerateNormals(v);
		ApplyDisplacementMap(v);
	}

	// Dade - create trianglemesh vertices
	Point *Plimit = new Point[nverts];
	for (u_int i = 0; i < nverts; ++i)
		Plimit[i] = v[i]->P;

	Normal *Ns = NULL;
	if (displacementMapNormalSmooth) {
		// Dade - calculate normals
		// FIXME - GenerateNormals should be called in all cases
		// but the displacement messes the data in some rare cases
		// when using the normal split option
		if (!displacementMap || !normalSplit)
			GenerateNormals(v);

		Ns = new Normal[nverts];
		for (u_int i = 0; i < nverts; ++i)
			Ns[i] = v[i]->n;
	}

	return boost::shared_ptr<SubdivResult>(new SubdivResult(ntris, nverts, verts, Plimit, Ns, UVLimit, colLimit, alphaLimit));
}

void LoopSubdiv::GenerateNormals(const vector<SDVertex *> v) {
	// Compute vertex tangents on limit surface
	u_int ringSize = 16;
	Point *Pring = new Point[ringSize];
	for (u_int i = 0; i < v.size(); ++i) {
		SDVertex *vert = v[i];
		Vector S(0,0,0), T(0,0,0);
		u_int valence = vert->valence();
		if (valence > ringSize) {
			ringSize = valence;
			delete[] Pring;
			Pring = new Point[ringSize];
		}
		vert->oneRing(Pring);
	
		if (!vert->boundary) {
			// Compute tangents of interior face
			for (u_int k = 0; k < valence; ++k) {
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
				float theta = M_PI / static_cast<float>(valence - 1);
				T = Vector(sinf(theta) * (Pring[0] + Pring[valence-1]));
				for (u_int k = 1; k < valence - 1; ++k) {
					float wt = (2*cosf(theta) - 2) * sinf((k) * theta);
					T += Vector(wt * Pring[k]);
				}
				T = -T;
			}
		}
		vert->n = Normal(Normalize(Cross(T, S)));
	}
}

void LoopSubdiv::ApplyDisplacementMap(const vector<SDVertex *> verts) const
{
	// Dade - apply the displacement map
	SHAPE_LOG(name, LUX_INFO,LUX_NOERROR) << "Applying displacement map to " << verts.size() << " vertices";
	SpectrumWavelengths swl;
	swl.Sample(.5f);

	for (u_int i = 0; i < verts.size(); i++) {
		SDVertex *v = verts[i];
		// Special use of the child member to detect that
		// the vertex has already been displaced
		if (v->child == v)
			continue;
		Vector dpdu, dpdv;
		CoordinateSystem(Vector(v->n), &dpdu, &dpdv);
		Vector displacement(v->n);

		SDFace *face = v->startFace;
		u_int nf = 0;
		float dl = 0.f;
		vector<SDVertex *> vlist;
		vlist.reserve(v->valence());
		if (v->boundary) {
			do {
				SDVertex *vv = face->v[face->vnum(v->P)];
				DifferentialGeometry dg(v->P, v->n, dpdu, dpdv,
					Normal(0, 0, 0), Normal(0, 0, 0),
					vv->u, vv->v, NULL);

				dl += displacementMap->Evaluate(swl, dg);
				++nf;
				if (vv->child != vv) {
					vlist.push_back(vv);
					vv->child = vv;
				}
				face = face->nextFace(v->P);
			} while (face);
			face = v->startFace->prevFace(v->P);
			while (face) {
				SDVertex *vv = face->v[face->vnum(v->P)];
				DifferentialGeometry dg(v->P, v->n, dpdu, dpdv,
					Normal(0, 0, 0), Normal(0, 0, 0),
					vv->u, vv->v, NULL);

				dl += displacementMap->Evaluate(swl, dg);
				++nf;
				if (vv->child != vv) {
					vlist.push_back(vv);
					vv->child = vv;
				}
				face = face->prevFace(v->P);
			}
		} else {
			do {
				SDVertex *vv = face->v[face->vnum(v->P)];
				DifferentialGeometry dg(v->P, v->n, dpdu, dpdv,
					Normal(0, 0, 0), Normal(0, 0, 0),
					vv->u, vv->v, NULL);

				dl += displacementMap->Evaluate(swl, dg);
				++nf;
				if (vv->child != vv) {
					vlist.push_back(vv);
					vv->child = vv;
				}
				face = face->nextFace(v->P);
			} while (face != v->startFace);
		}
		dl = (dl * displacementMapScale / nf + displacementMapOffset);
		// Average the displacement
		displacement *= dl;

		// Apply displacement to all vertices in the list
		for (u_int j = 0; j < vlist.size(); ++j)
			vlist[j]->P += displacement;
	}
}

void LoopSubdiv::weightOneRing(SDVertex *destVert, SDVertex *vert,
	float beta) const
{
	// Put _vert_ one-ring in _Pring_
	u_int valence = vert->valence();
	SDVertex **Vring = (SDVertex **)alloca(valence * sizeof(SDVertex *));
	SDVertex **VR = Vring;
	// Get one ring vertices for interior vertex
	SDFace *face = vert->startFace;
	bool uvSplit = false;
	bool colSplit = false;
	bool alphaSplit = false;
	do {
		SDVertex *v = face->v[face->vnum(vert->P)];

		if (v->u != vert->u || v->v != vert->v)
			uvSplit = true;
		if (v->col != vert->col)
			colSplit = true;
		if (v->alpha != vert->alpha)
			alphaSplit = true;

		SDVertex *v2 = face->nextVert(vert->P);
		float vu = v2->u;
		float vv = v2->v;
		RGBColor vc = v2->col;
		float va = v2->alpha;

		*VR++ = v2;
		face = face->nextFace(vert->P);
		v2 = face->prevVert(vert->P);

		if (vu != v2->u || vv != v2->v)
			uvSplit = true;
		if (vc != v2->col)
			colSplit = true;
		if (va != v2->alpha)
			alphaSplit = true;
	} while (face != vert->startFace);

	Point P((1 - valence * beta) * vert->P);
	float u = (1 - valence * beta) * vert->u;
	float v = (1 - valence * beta) * vert->v;
	RGBColor col = (1 - valence * beta) * vert->col;
	float alpha = (1 - valence * beta) * vert->alpha;
	Normal N((1 - valence * beta) * vert->n);

	for (u_int i = 0; i < valence; ++i) {
		P += beta * Vring[i]->P;
		u += beta * Vring[i]->u;
		v += beta * Vring[i]->v;
		N += beta * Vring[i]->n;
		col += beta * Vring[i]->col;
		alpha += beta * Vring[i]->alpha;
	}

	destVert->P = P;
	if (uvSplit) {
		destVert->u = vert->u;
		destVert->v = vert->v;
	} else {
		destVert->u = u;
		destVert->v = v;
	}
	if (colSplit)
		destVert->col = vert->col;
	else
		destVert->col = col;
	if (alphaSplit)
		destVert->alpha = vert->alpha;
	else
		destVert->alpha = alpha;
	destVert->n = N;
}

void SDVertex::oneRing(Point *Pring) const
{
	if (!boundary) {
		// Get one ring vertices for interior vertex
		SDFace *face = startFace;
		do {
			*Pring++ = face->nextVert(P)->P;
			face = face->nextFace(P);
		} while (face != startFace);
	} else {
		// Get one ring vertices for boundary vertex
		SDFace *face = startFace, *f2;
		while ((f2 = face->nextFace(P)) != NULL)
			face = f2;
		*Pring++ = face->nextVert(P)->P;
		do {
			*Pring++ = face->prevVert(P)->P;
			face = face->prevFace(P);
		} while (face != NULL);
	}
}

void LoopSubdiv::weightBoundary(SDVertex *destVert,  SDVertex *vert,
	float beta) const
{
	// Put _vert_ one-ring in _Pring_
	u_int valence = vert->valence();
	if (displacementMapSharpBoundary) {
		destVert->P = vert->P;
		destVert->u = vert->u;
		destVert->v = vert->v;
		destVert->n = vert->n;
		destVert->col = vert->col;
		destVert->alpha = vert->alpha;
		return;
	}
	SDVertex **Vring = (SDVertex **)alloca(valence * sizeof(SDVertex *));
	SDVertex **VR = Vring;
	// Get one ring vertices for boundary vertex
	SDFace *face = vert->startFace, *f2;
	// Go to the last face in the list
	while ((f2 = face->nextFace(vert->P)) != NULL)
		face = f2;
	// Add the last vertex (on the boundary)
	*VR++ = face->nextVert(vert->P);
	// Add all vertices up to the first one (on the boundary)
	bool uvSplit = false;
	bool colSplit = false;
	bool alphaSplit = false;
	do {
		SDVertex *v = face->v[face->vnum(vert->P)];

		if (v->u != vert->u || v->v != vert->v)
			uvSplit = true;
		if (v->col != vert->col)
			colSplit = true;
		if (v->alpha != vert->alpha)
			alphaSplit = true;

		SDVertex *v2 = face->prevVert(vert->P);
		float vu = v2->u;
		float vv = v2->v;
		RGBColor vc = v2->col;
		float va = v2->alpha;

		*VR++ = v2;
		face = face->prevFace(vert->P);
		if (face) {
			v2 = face->nextVert(vert->P);

			if (vu != v2->u || vv != v2->v)
				uvSplit = true;
			if (vc != v2->col)
				colSplit = true;
			if (va != v2->alpha)
				alphaSplit = true;
		}
	} while (face != NULL);

	Point P((1 - 2 * beta) * vert->P);
	P += beta * Vring[0]->P;
	P += beta * Vring[valence - 1]->P;
	destVert->P = P;

	if (uvSplit) {
		destVert->u = vert->u;
		destVert->v = vert->v;
	} else {
		float u = (1.f - 2.f * beta) * vert->u;
		float v = (1.f - 2.f * beta) * vert->v;
		u += beta * (Vring[0]->u + Vring[valence - 1]->u);
		v += beta * (Vring[0]->v + Vring[valence - 1]->v);
		destVert->u = u;
		destVert->v = v;
	}
	if (colSplit)
		destVert->col = vert->col;
	else {
		RGBColor col = (1.f - 2.f * beta) * vert->col;
		col += beta * (Vring[0]->col + Vring[valence - 1]->col);
		destVert->col = col;
	}
	if (alphaSplit)
		destVert->alpha = vert->alpha;
	else {
		float alpha = (1.f - 2.f * beta) * vert->alpha;
		alpha += beta * (Vring[0]->alpha + Vring[valence - 1]->alpha);
		destVert->alpha = alpha;
	}

	Normal N((1 - 2 * beta) * vert->n);
	N += beta * Vring[0]->n;
	N += beta * Vring[valence - 1]->n;
	destVert->n = N;
}

