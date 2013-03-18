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

#include "mesh.h"
#include "dynload.h"
#include "context.h"
#include "loopsubdiv.h"

#include "./mikktspace/mikktspace.h"
#include "./mikktspace/weldmesh.h"

#include "luxrays/core/trianglemesh.h"

using namespace lux;

Mesh::Mesh(const Transform &o2w, bool ro, const string &name,
	MeshAccelType acceltype,
	u_int nv, const Point *P, const Normal *N, const float *UV, const float *C,
	MeshTriangleType tritype, u_int trisCount, const int *tris,
	MeshQuadType quadtype, u_int nquadsCount, const int *quads,
	MeshSubdivType subdivtype, u_int nsubdivlevels,
	boost::shared_ptr<Texture<float> > &dmMap, float dmScale, float dmOffset,
	bool dmNormalSmooth, bool dmSharpBoundary, bool normalsplit, bool genTangents)
	: Shape(o2w, ro, name)
{
	accelType = acceltype;

	subdivType = subdivtype;
	nSubdivLevels = nsubdivlevels;
	displacementMap = dmMap;
	displacementMapScale = dmScale;
	displacementMapOffset = dmOffset;
	displacementMapNormalSmooth = dmNormalSmooth;
	displacementMapSharpBoundary = dmSharpBoundary;
	normalSplit = normalsplit;
	mustSubdivide = nSubdivLevels > 0;

	// TODO: use AllocAligned

	// Dade - copy vertex data
	nverts = nv;
	p = new Point[nverts];
	// Dade - transform mesh vertices to world space
	for (u_int i  = 0; i < nverts; ++i)
		p[i] = ObjectToWorld * P[i];

	// Dade - copy UV and N vertex data, if present
	if (UV) {
		uvs = new float[2 * nverts];
		memcpy(uvs, UV, 2 * nverts * sizeof(float));
	} else
		uvs = NULL;

	if (N) {
		n = new Normal[nverts];
		// Dade - transform mesh normals to world space
		for (u_int i  = 0; i < nverts; ++i) {
			if (ro)
				n[i] = Normalize(-(ObjectToWorld * N[i]));
			else
				n[i] = Normalize(ObjectToWorld * N[i]);
		}
	} else
		n = NULL;

	if (C) {
		cols = new float[3 * nverts];
		memcpy(cols, C, 3 * nverts * sizeof(float));
	} else
		cols = NULL;

	if (genTangents && !uvs) {
		SHAPE_LOG(name, LUX_ERROR,LUX_CONSISTENCY)<< "Cannot generate tangent space for mesh, mesh does not have UV coordinates.";
		generateTangents = false;
	} else
		generateTangents = genTangents;
	// will be allocated in GenerateTangentSpace if needed
	t = NULL;
	btsign = NULL;

	// Dade - copy quad data
	quadType = quadtype;
	nquads = nquadsCount;
	vector<int> quadsOk;
	vector<int> quadsToSplit;
	if (nquads == 0)
		quadVertexIndex = NULL;
	else {
		// Dade - check quads and split them if required
		for (u_int i = 0; i < nquads; i++) {
			const u_int idx = 4 * i;
/*			const Point &p0 = p[quads[idx]];
			const Point &p1 = p[quads[idx + 1]];
			const Point &p2 = p[quads[idx + 2]];
			const Point &p3 = p[quads[idx + 3]];

			// Split the quad if using subdivision, tangent space generation (only possible on tri's) or if its not planar or convex
			//bool quadOk = MeshQuadrilateral::IsPlanar(p0, p1, p2, p3) && MeshQuadrilateral::IsConvex(p0, p1, p2, p3);*/
			// TODO - quads have issues with normals and uvs, split them
			bool quadOk = false;
			if (!mustSubdivide && !generateTangents && quadOk) {
				quadsOk.push_back(quads[idx]);
				quadsOk.push_back(quads[idx + 1]);
				quadsOk.push_back(quads[idx + 2]);
				quadsOk.push_back(quads[idx + 3]);
			} else {
				// Dade - split in 2 x triangle
				quadsToSplit.push_back(quads[idx]);
				quadsToSplit.push_back(quads[idx + 1]);
				quadsToSplit.push_back(quads[idx + 2]);
				quadsToSplit.push_back(quads[idx + 3]);
			}
		}

		nquads = quadsOk.size() / 4;
		if (nquads == 0)
			quadVertexIndex = NULL;
		else {
			quadVertexIndex = new int[4 * nquads];
			for (u_int i = 0; i < 4 * nquads; ++i)
				quadVertexIndex[i] = quadsOk[i];
		}
	}

	if (!quadsToSplit.empty()) {
		std::stringstream ss;
		ss << "Mesh: splitting " << (quadsToSplit.size() / 4) << " quads";
		// TODO - quads have issues with normals and uvs, don't specify		
		//if( nSubdivLevels > 0 )
		//	ss << " to allow subdivision";
		//else
		//	ss << " because they are non-planar or non-convex";		
		SHAPE_LOG(name, LUX_INFO,LUX_NOERROR)<< ss.str().c_str();
	}

	// Dade - copy triangle data
	triType = tritype;
	ntris = trisCount;
	// Dade - add quads to split
	const size_t nquadsToSplit = quadsToSplit.size() / 4;
	ntris += 2 * nquadsToSplit;
	if (ntris == 0)
		triVertexIndex = NULL;
	else {
		triVertexIndex = new int[3 * ntris];
		memcpy(triVertexIndex, tris, 3 * trisCount * sizeof(int));

		for (size_t i = 0; i < nquadsToSplit; ++i) {
			const size_t qidx = 4 * i;
			const size_t tidx = 3 * trisCount + 2 * 3 * i;

			const u_int qi0 = quadsToSplit[qidx + 0];
			const u_int qi1 = quadsToSplit[qidx + 1];
			const u_int qi2 = quadsToSplit[qidx + 2];
			const u_int qi3 = quadsToSplit[qidx + 3];

			bool splitfirstdiag = true;

			// split along shortest diagonal to generate consistent triangles 
			// for tangent space calculations
			const float dlen1 = DistanceSquared(p[qi0], p[qi2]);
			const float dlen2 = DistanceSquared(p[qi1], p[qi3]);

			splitfirstdiag = dlen1 < dlen2;

			if (dlen1 == dlen2 && uvs) {
				// determine split using UV coords instead
				const float tlen1 = DistanceSquared(Point(uvs[2*qi0+0], uvs[2*qi0+1], 0.f), 
					Point(uvs[2*qi2+0], uvs[2*qi2+1], 0.f));
				const float tlen2 = DistanceSquared(Point(uvs[2*qi1+0], uvs[2*qi1+1], 0.f), 
					Point(uvs[2*qi3+0], uvs[2*qi3+1], 0.f));

				splitfirstdiag = tlen1 < tlen2;
			}

			if (splitfirstdiag) {
				// triangle A
				triVertexIndex[tidx + 0] = quadsToSplit[qidx + 0];
				triVertexIndex[tidx + 1] = quadsToSplit[qidx + 1];
				triVertexIndex[tidx + 2] = quadsToSplit[qidx + 2];
				// triangle B
				triVertexIndex[tidx + 3] = quadsToSplit[qidx + 0];
				triVertexIndex[tidx + 4] = quadsToSplit[qidx + 2];
				triVertexIndex[tidx + 5] = quadsToSplit[qidx + 3];
			} else {
				// triangle A
				triVertexIndex[tidx + 0] = quadsToSplit[qidx + 1];
				triVertexIndex[tidx + 1] = quadsToSplit[qidx + 2];
				triVertexIndex[tidx + 2] = quadsToSplit[qidx + 3];
				// triangle B
				triVertexIndex[tidx + 3] = quadsToSplit[qidx + 1];
				triVertexIndex[tidx + 4] = quadsToSplit[qidx + 3];
				triVertexIndex[tidx + 5] = quadsToSplit[qidx + 0];
			}
		}
	}
}

Mesh::~Mesh()
{
	delete[] triVertexIndex;
	delete[] quadVertexIndex;
	delete[] p;
	delete[] n;
	delete[] uvs;
	delete[] cols;
	delete[] t;
	delete[] btsign;
}

BBox Mesh::ObjectBound() const
{
	BBox bobj;
	for (u_int i = 0; i < nverts; ++i)
		bobj = Union(bobj, Inverse(ObjectToWorld) * p[i]);
	return bobj;
}

BBox Mesh::WorldBound() const
{
	BBox worldBounds;
	for (u_int i = 0; i < nverts; ++i)
		worldBounds = Union(worldBounds, p[i]);
	return worldBounds;
}

template<class T>
class MeshElemSharedPtr : public T
{
public:
	MeshElemSharedPtr(const Mesh* m, u_int n,
		const boost::shared_ptr<Primitive> &aPtr)
	: T(m,n), ptr(aPtr) { }
private:
	const boost::shared_ptr<Primitive> ptr;
};

void Mesh::Refine(vector<boost::shared_ptr<Primitive> > &refined,
	const PrimitiveRefinementHints &refineHints,
	const boost::shared_ptr<Primitive> &thisPtr)
{
	if (ntris + nquads == 0)
		return;

	// Possibly subdivide the triangles
	if (mustSubdivide) {
		MeshSubdivType concreteSubdivType = subdivType;
		switch (concreteSubdivType) {
			case SUBDIV_LOOP: {
				// TODO: add the support for vertex colors too

				// Apply subdivision
				LoopSubdiv loopsubdiv(ntris, nverts,
					triVertexIndex, p, uvs, n,
					nSubdivLevels, displacementMap,
					displacementMapScale,
					displacementMapOffset,
					displacementMapNormalSmooth,
					displacementMapSharpBoundary,
					normalSplit, name);
				boost::shared_ptr<LoopSubdiv::SubdivResult> res(loopsubdiv.Refine());
				// Check if subdivision was successfull
				if (!res)
					break;

				// Remove the old mesh data
				delete[] p;
				delete[] n;
				delete[] uvs;
				delete[] cols;
				delete[] triVertexIndex;

				// Copy the new mesh data
				nverts = res->nverts;
				ntris = res->ntris;
				triVertexIndex = new int[3 * ntris];
				memcpy(triVertexIndex, res->indices, 3 * ntris * sizeof(int));
				p = new Point[nverts];
				memcpy(p, res->P, nverts * sizeof(Point));
				if (res->uv) {
					uvs = new float[2 * nverts];
					memcpy(uvs, res->uv, 2 * nverts * sizeof(float));
				} else
					uvs = NULL;

				if (res->N) {
					n = new Normal[nverts];
					memcpy(n, res->N, nverts * sizeof(Normal));
				} else
					n = NULL;

				break;
			}
			case SUBDIV_MICRODISPLACEMENT:
				// TODO: add the support for vertex colors too

				if (displacementMap) {
					// get min/max displacement for MD
					displacementMap->GetMinMaxFloat(&displacementMapMin, &displacementMapMax);

					if (displacementMapMin < -1.f || displacementMapMax > 1.f)
						SHAPE_LOG(name, LUX_WARNING, LUX_LIMIT) << "Displacement map for microdisplacement reported min/max values of (" 
							<< displacementMapMin << "," << displacementMapMax << "), actual displacement values will be clamped to [-1,1]";

					triType = TRI_MICRODISPLACEMENT;
				} else {
					SHAPE_LOG(name, LUX_WARNING, LUX_CONSISTENCY) << "No displacement map for microdisplacement, disabling";
					triType = TRI_AUTO;
				}

				break;
			default: {
				SHAPE_LOG(name, LUX_ERROR,LUX_CONSISTENCY) << "Unknown subdivision type in a mesh: " << concreteSubdivType;
				break;
			}
		}

		mustSubdivide = false; // only subdivide on the first refine!!!
	}

	if (generateTangents) {
		GenerateTangentSpace();
	}



	vector<boost::shared_ptr<Primitive> > refinedPrims;
	refinedPrims.reserve(ntris + nquads);
	// Dade - refine triangles
	MeshTriangleType concreteTriType = triType;
	if (triType == TRI_AUTO) {
		// If there is 1 unique vertex (with normals and uv coordinates) for each triangle:
		//  bary = 52 bytes/triangle
		//  wald = 128 bytes/triangle
		// Note: this ignores some accel data
		//  the following are accounted for: vertices, vertex indices, Mesh*Triangle data
		//  and one shared_ptr in the accel
		//TODO Lotus - find good values
		if (ntris <= 200000)
			concreteTriType = TRI_WALD;
		else
			concreteTriType = TRI_BARY;
	}

	inconsistentShadingTris = 0;

	switch (concreteTriType) {
		case TRI_WALD:
			for (u_int i = 0; i < ntris; ++i) {
				MeshWaldTriangle *currTri;
				if (refinedPrims.size() > 0)
					currTri = new MeshWaldTriangle(this, i);
				else
					currTri = new MeshElemSharedPtr<MeshWaldTriangle>(this, i, thisPtr);
				if (!currTri->isDegenerate()) {
					boost::shared_ptr<Primitive> o(currTri);
					refinedPrims.push_back(o);
				} else
					delete currTri;
			}
			break;
		case TRI_BARY:
			for (u_int i = 0; i < ntris; ++i) {
				MeshBaryTriangle *currTri;
				if (refinedPrims.size() > 0)
					currTri = new MeshBaryTriangle(this, i);
				else
					currTri = new MeshElemSharedPtr<MeshBaryTriangle>(this, i, thisPtr);
				if (!currTri->isDegenerate()) {
					boost::shared_ptr<Primitive> o(currTri);
					refinedPrims.push_back(o);
				} else
					delete currTri;
			}
			break;
		case TRI_MICRODISPLACEMENT:
			for (u_int i = 0; i < ntris; ++i) {
				MeshMicroDisplacementTriangle *currTri;
				if (refinedPrims.size() > 0)
					currTri = new MeshMicroDisplacementTriangle(this, i);
				else
					currTri = new MeshElemSharedPtr<MeshMicroDisplacementTriangle>(this, i, thisPtr);
				if (!currTri->isDegenerate()) {
					boost::shared_ptr<Primitive> o(currTri);
					refinedPrims.push_back(o);
				} else
					delete currTri;
			}
			break;
		default: {
			SHAPE_LOG(name, LUX_ERROR,LUX_CONSISTENCY) << "Unknown triangle type: " << concreteTriType;
			break;
		}
	}

	if (inconsistentShadingTris > 0) {
		SHAPE_LOG(name, LUX_DEBUG, LUX_CONSISTENCY) <<
			"Inconsistent shading normals in " << 
			inconsistentShadingTris << " triangle" << (inconsistentShadingTris > 1 ? "s" : "");
	}

	u_int numConcreteTris = refinedPrims.size();

	// Dade - refine quads
	switch (quadType) {
		case QUAD_QUADRILATERAL:
			for (u_int i = 0; i < nquads; ++i) {
				MeshQuadrilateral* currQuad = new MeshQuadrilateral(this, i);
				if (!currQuad->isDegenerate()) {
					if (refinedPrims.size() > 0) {
						boost::shared_ptr<Primitive> o(currQuad);
						refinedPrims.push_back(o);
					} else {
						delete currQuad;
						boost::shared_ptr<Primitive> o(
							new MeshElemSharedPtr<MeshQuadrilateral>(this, i, thisPtr));
						refinedPrims.push_back(o);
					}
				} else
					delete currQuad;
			}
			break;
		default: {
			SHAPE_LOG(name, LUX_ERROR,LUX_CONSISTENCY) << "Unknown quad type in a mesh: " << quadType;
			break;
		}
	}
	u_int numConcreteQuads = refinedPrims.size() - numConcreteTris;

	// Select best acceleration structure
	MeshAccelType concreteAccelType = accelType;
	if (accelType == ACCEL_AUTO) {
		if (refinedPrims.size() <= 250000)
			concreteAccelType = ACCEL_NONE;
		else if (refinedPrims.size() <= 500000)
			concreteAccelType = ACCEL_KDTREE;
		else
			concreteAccelType = ACCEL_QBVH;
	}

	// Report selections used
	std::stringstream ss;
	ss << "Mesh: accel = ";
	switch (concreteAccelType) {
		case ACCEL_NONE:
			ss << "none (global)";
			break;
		case ACCEL_QBVH:
			ss << "qbvh";
			break;
		case ACCEL_GRID:
			ss << "grid";
			break;
		case ACCEL_BRUTEFORCE:
			ss << "bruteforce";
			break;
		case ACCEL_KDTREE:
			ss << "kdtree";
			break;
		default:
			ss << "?";
	}
	ss << ", triangles = " << numConcreteTris << " ";
	switch (concreteTriType) {
		case TRI_BARY:
			ss << "bary";
			break;
		case TRI_WALD:
			ss << "wald";
			break;
		case TRI_MICRODISPLACEMENT:
			ss << "microdisp";
			break;
		default:
			ss << "?";
	}
	ss << ", quads = " << numConcreteQuads << " ";
	switch (quadType) {
		case QUAD_QUADRILATERAL:
			ss << "quadrilateral";
			break;
		default:
			ss << "?";
	}
	SHAPE_LOG(name, LUX_DEBUG,LUX_NOERROR)<< ss.str().c_str();

	// Build acceleration structure
	if (concreteAccelType == ACCEL_NONE) {
		// Copy primitives
		// NOTE - lordcrc - use resize+swap to avoid shared_ptr count from changing
		const u_int offset = refined.size();
		refined.resize(refined.size() + refinedPrims.size());
		for(u_int i = 0; i < refinedPrims.size(); ++i)
			refined[offset+i].swap(refinedPrims[i]);
	} else  {
		//FIXME: QBVH doesn't play well with PrimitiveSet
		if (refineHints.forSampling && concreteAccelType == ACCEL_QBVH)
			concreteAccelType = ACCEL_KDTREE;
		ParamSet paramset;
		boost::shared_ptr<Aggregate> accel;
		switch (concreteAccelType) {
			case ACCEL_KDTREE:
				accel = MakeAccelerator("kdtree", refinedPrims, paramset);
				break;
			case ACCEL_QBVH:
				accel = MakeAccelerator("qbvh", refinedPrims, paramset);
				break;
			case ACCEL_GRID:
				accel = MakeAccelerator("grid", refinedPrims, paramset);
				break;
			case ACCEL_BRUTEFORCE:
				accel = MakeAccelerator("bruteforce", refinedPrims, paramset);
				break;
			default:
				SHAPE_LOG(name, LUX_ERROR,LUX_CONSISTENCY) << "Unknown accel type: " << concreteAccelType;
		}
		if (refineHints.forSampling)
			// Lotus - create primitive set to allow sampling
			refined.push_back(boost::shared_ptr<Primitive>(new PrimitiveSet(accel)));
		else
			refined.push_back(accel);
	}
}

void Mesh::Tessellate(vector<luxrays::TriangleMesh *> *meshList, vector<const Primitive *> *primitiveList) const {
	// A little hack with pointers
	luxrays::TriangleMesh *tm = new luxrays::TriangleMesh(
			nverts, ntris, p, (luxrays::Triangle *)triVertexIndex);

	meshList->push_back(tm);
	primitiveList->push_back(this);
}

void Mesh::ExtTessellate(vector<luxrays::ExtTriangleMesh *> *meshList, vector<const Primitive *> *primitiveList) const {
	// A little hack with pointers
	luxrays::ExtTriangleMesh *tm = new luxrays::ExtTriangleMesh(
			nverts, ntris, p, (luxrays::Triangle *)triVertexIndex,
			n, (luxrays::UV *)uvs);

	meshList->push_back(tm);
	primitiveList->push_back(this);
}

void Mesh::GetIntersection(const luxrays::RayHit &rayHit, const u_int index, Intersection *isect) const {
	const u_int triIndex = index * 3;
	const u_int v0 = triVertexIndex[triIndex];
	const u_int v1 = triVertexIndex[triIndex + 1];
	const u_int v2 = triVertexIndex[triIndex + 2];
	const Point &p1 = p[v0];
	const Point &p2 = p[v1];
	const Point &p3 = p[v2];
	const Vector e1 = p2 - p1;
	const Vector e2 = p3 - p1;

	// Fill in _DifferentialGeometry_ from triangle hit
	// Compute triangle partial derivatives
	Vector dpdu, dpdv;
	float uv[3][2];
	if (uvs) {
		uv[0][0] = uvs[2 * v0];
		uv[0][1] = uvs[2 * v0 + 1];
		uv[1][0] = uvs[2 * v1];
		uv[1][1] = uvs[2 * v1 + 1];
		uv[2][0] = uvs[2 * v2];
		uv[2][1] = uvs[2 * v2 + 1];
	} else {
		uv[0][0] = .5f;//p[v[0]].x;
		uv[0][1] = .5f;//p[v[0]].y;
		uv[1][0] = .5f;//p[v[1]].x;
		uv[1][1] = .5f;//p[v[1]].y;
		uv[2][0] = .5f;//p[v[2]].x;
		uv[2][1] = .5f;//p[v[2]].y;
	}

	// Compute deltas for triangle partial derivatives
	const float du1 = uv[0][0] - uv[2][0];
	const float du2 = uv[1][0] - uv[2][0];
	const float dv1 = uv[0][1] - uv[2][1];
	const float dv2 = uv[1][1] - uv[2][1];
	const Vector dp1 = p1 - p3, dp2 = p2 - p3;
	const float determinant = du1 * dv2 - dv1 * du2;
	if (determinant == 0.f) {
		// Handle 0 determinant for triangle partial derivative matrix
		CoordinateSystem(Normalize(Cross(e1, e2)), &dpdu, &dpdv);
	} else {
		const float invdet = 1.f / determinant;
		dpdu = ( dv2 * dp1 - dv1 * dp2) * invdet;
		dpdv = (-du2 * dp1 + du1 * dp2) * invdet;
	}

	const float b0 = 1.f - rayHit.b1 - rayHit.b2;
	const float b1 = rayHit.b1;
	const float b2 = rayHit.b2;

	// Interpolate $(u,v)$ triangle parametric coordinates
	const float tu = b0 * uv[0][0] + b1 * uv[1][0] + b2 * uv[2][0];
	const float tv = b0 * uv[0][1] + b1 * uv[1][1] + b2 * uv[2][1];

	const Normal nn = Normal(Normalize(Cross(e1, e2)));
	const Point pp(p1 + b1 * e1 + b2 * e2);

	isect->dg = DifferentialGeometry(pp, nn, dpdu, dpdv,
		Normal(0, 0, 0), Normal(0, 0, 0), tu, tv, this);

	isect->Set(ObjectToWorld, this, GetMaterial(),
		GetExterior(), GetInterior());
	isect->dg.iData.mesh.coords[0] = b0;
	isect->dg.iData.mesh.coords[1] = b1;
	isect->dg.iData.mesh.coords[2] = b2;
	isect->dg.iData.mesh.triIndex = triIndex;
}

void Mesh::GetShadingGeometry(const Transform &obj2world,
	const DifferentialGeometry &dg, DifferentialGeometry *dgShading) const
{
	if (!n) {
		*dgShading = dg;
		return;
	}

	const u_int v0 = triVertexIndex[dg.iData.mesh.triIndex];
	const u_int v1 = triVertexIndex[dg.iData.mesh.triIndex + 1];
	const u_int v2 = triVertexIndex[dg.iData.mesh.triIndex + 2];

	// Use _n_ to compute shading tangents for triangle, _ss_ and _ts_
	const Normal nsi = dg.iData.mesh.coords[0] * n[v0] +
		dg.iData.mesh.coords[1] * n[v1] + dg.iData.mesh.coords[2] * n[v2];
	const Normal ns = Normalize(nsi);

	Vector ss, ts;
	Vector tangent, bitangent;
	float sign;
	// if we got a generated tangent space, use that
	if (t) {
		// length of these vectors is essential for sampled normal mapping
		// they should be normalized at vertex level, and NOT normalized after interpolation
		tangent = dg.iData.mesh.coords[0] * t[v0] +
			dg.iData.mesh.coords[1] * t[v1] + dg.iData.mesh.coords[2] * t[v2];
		// only degenerate triangles will have different vertex signs
		bitangent = Cross(nsi, tangent);
		// store sign, and also magnitude of interpolated normal so we can recover it
		sign = (btsign[v0] ? 1.f : -1.f) * nsi.Length();

		ss = Normalize(tangent);
		ts = Normalize(bitangent);
	} else {
		ts = Normalize(Cross(ns, dg.dpdu));
		ss = Cross(ts, ns);

		ts *= Dot(dg.dpdv, ts) > 0.f ? 1.f : -1.f;

		tangent = ss;
		bitangent = ts;

		sign = (Dot(ts, ns) > 0.f ? 1.f : -1.f);
	}

	// the length of dpdu/dpdv can be important for bumpmapping
	ss *= dg.dpdu.Length();
	ts *= dg.dpdv.Length();

	Normal dndu, dndv;
	// Compute \dndu and \dndv for triangle shading geometry
	float uv[3][2];
	if (uvs) {
		uv[0][0] = uvs[2 * v0];
		uv[0][1] = uvs[2 * v0 + 1];
		uv[1][0] = uvs[2 * v1];
		uv[1][1] = uvs[2 * v1 + 1];
		uv[2][0] = uvs[2 * v2];
		uv[2][1] = uvs[2 * v2 + 1];
	} else {
		uv[0][0] = .5f;//p[v[0]].x;
		uv[0][1] = .5f;//p[v[0]].y;
		uv[1][0] = .5f;//p[v[1]].x;
		uv[1][1] = .5f;//p[v[1]].y;
		uv[2][0] = .5f;//p[v[2]].x;
		uv[2][1] = .5f;//p[v[2]].y;
	}

	// Compute deltas for triangle partial derivatives of normal
	const float du1 = uv[0][0] - uv[2][0];
	const float du2 = uv[1][0] - uv[2][0];
	const float dv1 = uv[0][1] - uv[2][1];
	const float dv2 = uv[1][1] - uv[2][1];
	const Normal dn1 = n[v0] - n[v2];
	const Normal dn2 = n[v1] - n[v2];
	const float determinant = du1 * dv2 - dv1 * du2;

	if (determinant == 0.f)
		dndu = dndv = Normal(0, 0, 0);
	else {
		const float invdet = 1.f / determinant;
		dndu = ( dv2 * dn1 - dv1 * dn2) * invdet;
		dndv = (-du2 * dn1 + du1 * dn2) * invdet;
	}

	*dgShading = DifferentialGeometry(dg.p, ns, ss, ts,
		dndu, dndv, tangent, bitangent, sign, dg.u, dg.v, this);
}

// Class for storing mesh data pointers and holding returned tangent space data
class MikkTSData {
public:
	MikkTSData(int n, int *vertexIndex, Point *pp, Normal *nn, float *uvs) 
		: ntris(n), idx(vertexIndex), p(pp), n(nn), uv(uvs) {
		t = new Vector[3*ntris];
		sign = new float[3*ntris];
	}

	~MikkTSData() {
		delete[] t;
		delete[] sign;
	}

	int ntris;
	int *idx;
	Point *p;
	Normal *n;
	float *uv;

	Vector *t;
	float *sign;
};

// Returns the number of faces (triangles/quads) on the mesh to be processed.
int mikkts_getNumFaces(const SMikkTSpaceContext * pContext) {
	MikkTSData *data = static_cast<MikkTSData*>(pContext->m_pUserData);

	return data->ntris;
}

// Returns the number of vertices on face number iFace
// iFace is a number in the range {0, 1, ..., getNumFaces()-1}
int mikkts_getNumVerticesOfFace(const SMikkTSpaceContext * pContext, const int iFace) {
	return 3; // quads are split in constructor
}

// returns the position/normal/texcoord of the referenced face of vertex number iVert.
// iVert is in the range {0,1,2} for triangles and {0,1,2,3} for quads.
void mikkts_getPosition(const SMikkTSpaceContext * pContext, float fvPosOut[], const int iFace, const int iVert) {
	MikkTSData *data = static_cast<MikkTSData*>(pContext->m_pUserData);

	const Point& pos(data->p[data->idx[3*iFace + iVert]]);
	fvPosOut[0] = pos.x;
	fvPosOut[1] = pos.y;
	fvPosOut[2] = pos.z;
}
void mikkts_getNormal(const SMikkTSpaceContext * pContext, float fvNormOut[], const int iFace, const int iVert) {
	MikkTSData *data = static_cast<MikkTSData*>(pContext->m_pUserData);

	const Normal& norm(data->n[data->idx[3*iFace + iVert]]);
	fvNormOut[0] = norm.x;
	fvNormOut[1] = norm.y;
	fvNormOut[2] = norm.z;
}
void mikkts_getTexCoord(const SMikkTSpaceContext * pContext, float fvTexcOut[], const int iFace, const int iVert) {
	MikkTSData *data = static_cast<MikkTSData*>(pContext->m_pUserData);

	const float* const tc = &data->uv[2*data->idx[3*iFace + iVert]];
	fvTexcOut[0] = tc[0];
	fvTexcOut[1] = tc[1];
}

// This function is used to return the tangent and fSign to the application.
// fvTangent is a unit length vector.
// For normal maps it is sufficient to use the following simplified version of the bitangent which is generated at pixel/vertex level.
// bitangent = fSign * cross(vN, tangent);
// Note that the results are returned unindexed. It is possible to generate a new index list
// But averaging/overwriting tangent spaces by using an already existing index list WILL produce INCRORRECT results.
// DO NOT! use an already existing index list.
void mikkts_setTSpaceBasic(const SMikkTSpaceContext * pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert) {
	MikkTSData *data = static_cast<MikkTSData*>(pContext->m_pUserData);

	data->t[3*iFace + iVert] = Vector(fvTangent[0], fvTangent[1], fvTangent[2]);
	data->sign[3*iFace + iVert] = fSign;
}

void Mesh::GenerateTangentSpace() {
	SHAPE_LOG(name, LUX_INFO,LUX_NOERROR)<< "Generating tangent space.";

	// set up data structures for mikktspace, use defaults
	SMikkTSpaceInterface mif;
	mif.m_getNumFaces = mikkts_getNumFaces;
	mif.m_getNumVerticesOfFace = mikkts_getNumVerticesOfFace;
	mif.m_getPosition = mikkts_getPosition;
	mif.m_getNormal = mikkts_getNormal;
	mif.m_getTexCoord = mikkts_getTexCoord;
	mif.m_setTSpaceBasic = mikkts_setTSpaceBasic;
	mif.m_setTSpace = NULL;

	MikkTSData data(static_cast<int>(ntris), triVertexIndex, p, n, uvs);

	SMikkTSpaceContext mctx;
	mctx.m_pInterface = &mif;
	mctx.m_pUserData = &data;

	if (!data.t || !data.sign) {
		SHAPE_LOG(name, LUX_ERROR,LUX_SYSTEM)<< "Failed to generate tangent space, out of memory.";
		return;
	}

	// generate tangent space
	if (!genTangSpaceDefault(&mctx)) {
		SHAPE_LOG(name, LUX_ERROR,LUX_SYSTEM)<< "Failed to generate tangent space.";
		return;
	}

	// tangents are returned unindexed, need to generate new index list
	// as some vertices may share normals and uv, but have different tangents
	SHAPE_LOG(name, LUX_DEBUG,LUX_NOERROR)<< "Generating new index list.";

	const u_int floatsPerVert = 3 + 3 + 2 + 3 + 1;
	float* vertDataIn = new float[3 * ntris * floatsPerVert];

	if (!vertDataIn) {
		SHAPE_LOG(name, LUX_ERROR,LUX_SYSTEM)<< "Failed to generate tangent space, out of memory.";

		delete[] vertDataIn;
		return;
	}

	// copy mesh data into "fat" array for welding
	for (u_int i = 0; i < 3*ntris; i++) {
		const u_int tvidx = triVertexIndex[i];
		const u_int idx = i * floatsPerVert;
		vertDataIn[idx + 0] = p[tvidx].x;
		vertDataIn[idx + 1] = p[tvidx].y;
		vertDataIn[idx + 2] = p[tvidx].z;
		vertDataIn[idx + 3] = n[tvidx].x;
		vertDataIn[idx + 4] = n[tvidx].y;
		vertDataIn[idx + 5] = n[tvidx].z;
		vertDataIn[idx + 6] = uvs[2*tvidx + 0];
		vertDataIn[idx + 7] = uvs[2*tvidx + 1];
		vertDataIn[idx + 8] = data.t[i].x;
		vertDataIn[idx + 9] = data.t[i].y;
		vertDataIn[idx + 10] = data.t[i].z;
		vertDataIn[idx + 11] = data.sign[i];
	}

	// free here to conserve memory
	delete[] data.t;
	data.t = NULL;
	delete[] data.sign;
	data.sign = NULL;

	float* vertDataOut = new float[3 * ntris * floatsPerVert];
	int* remapTable = new int[3 * ntris];

	if (!vertDataOut || !remapTable) {
		SHAPE_LOG(name, LUX_ERROR,LUX_SYSTEM)<< "Failed to generate tangent space, out of memory.";

		delete[] vertDataIn;
		delete[] vertDataOut;
		delete[] remapTable;
		return;
	}

	// safe to free mesh data
	delete[] triVertexIndex;
	delete[] p;
	delete[] n;
	delete[] uvs;

	// perform the weld
	nverts = WeldMesh(remapTable, vertDataOut, vertDataIn, 3 * ntris, floatsPerVert);
	delete[] vertDataIn;	

	triVertexIndex = remapTable;
	p = new Point[nverts];
	n = new Normal[nverts];
	uvs = new float[2*nverts];
	t = new Vector[nverts];
	btsign = new bool[nverts];

	// copy vertex data back into mesh
	for (u_int i = 0; i < nverts; i++) {
		const u_int vidx = i * floatsPerVert;

		p[i] = Point(vertDataOut[vidx + 0], vertDataOut[vidx + 1], vertDataOut[vidx + 2]);
		n[i] = Normal(vertDataOut[vidx + 3], vertDataOut[vidx + 4], vertDataOut[vidx + 5]);
		uvs[2*i+0] = vertDataOut[vidx + 6];
		uvs[2*i+1] = vertDataOut[vidx + 7];
		t[i] = Vector(vertDataOut[vidx + 8], vertDataOut[vidx + 9], vertDataOut[vidx + 10]);
		btsign[i] = vertDataOut[vidx + 11] > 0.f;
	}

	delete[] vertDataOut;
}

static Shape *CreateShape( const Transform &o2w, bool reverseOrientation, const ParamSet &params,
						   const string& accelTypeStr, const string& triTypeStr, const string& quadTypeStr,
						   const int* triIndices, u_int triIndicesCount,
						   const int* quadIndices, u_int quadIndicesCount,
						   const float* UV, u_int UVCount,
							const float *cols, u_int colsCount,
						   const string& subdivSchemeStr, u_int nSubdivLevels,
						   const Point* P, u_int npi,
						   const Normal* N, u_int nni) {

	string name = params.FindOneString("name", "'mesh'");
	// Lotus - read general data
	Mesh::MeshAccelType accelType;
	if (accelTypeStr == "kdtree")
		accelType = Mesh::ACCEL_KDTREE;
	else if (accelTypeStr == "qbvh")
		accelType = Mesh::ACCEL_QBVH;
	else if (accelTypeStr == "bruteforce")
		accelType = Mesh::ACCEL_BRUTEFORCE;
	else if (accelTypeStr == "grid")
		accelType = Mesh::ACCEL_GRID;
	else if (accelTypeStr == "none")
		accelType = Mesh::ACCEL_NONE;
	else if (accelTypeStr == "auto")
		accelType = Mesh::ACCEL_AUTO;
	else {
		SHAPE_LOG(name, LUX_WARNING,LUX_BADTOKEN) << "Acceleration structure type  '" << accelTypeStr << "' unknown. Using \"auto\".";
		accelType = Mesh::ACCEL_AUTO;
	}

	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000085: check for correct number of uvs
	if (UV && (UVCount != npi * 2)) {
		SHAPE_LOG(name, LUX_ERROR,LUX_CONSISTENCY)<< "Number of \"UV\"s for mesh must match \"P\"s";
		UV = NULL;
	}
	if (cols && (colsCount != npi * 3)) {
		SHAPE_LOG(name, LUX_ERROR,LUX_CONSISTENCY)<< "Number of \"C\"s for mesh must match \"P\"s";
		cols = NULL;
	}
	if (!P)
		return NULL;

	if (N && (nni != npi)) {
		SHAPE_LOG(name, LUX_ERROR,LUX_CONSISTENCY)<< "Number of \"N\"s for mesh must match \"P\"s";
		N = NULL;
	}

	// Dade - read triangle data
	Mesh::MeshTriangleType triType;
	if (triTypeStr == "wald")
		triType = Mesh::TRI_WALD;
	else if (triTypeStr == "bary")
		triType = Mesh::TRI_BARY;
	else if (triTypeStr == "auto")
		triType = Mesh::TRI_AUTO;
	else {
		SHAPE_LOG(name, LUX_WARNING,LUX_BADTOKEN) << "Triangle type  '" << triTypeStr << "' unknown. Using \"auto\".";
		triType = Mesh::TRI_AUTO;
	}

	if (triIndices) {
		for (u_int i = 0; i < triIndicesCount; ++i) {
			if (static_cast<u_int>(triIndices[i]) >= npi) {
				SHAPE_LOG(name, LUX_ERROR,LUX_CONSISTENCY) << "Mesh has out of-bounds triangle vertex index " << triIndices[i] <<
						" (" << npi << "  \"P\" values were given";
				return NULL;
			}
		}

		triIndicesCount /= 3;
	} else
		triIndicesCount = 0;

	// Copy quad data
	Mesh::MeshQuadType quadType;
	if (quadTypeStr == "quadrilateral") quadType = Mesh::QUAD_QUADRILATERAL;
	else {
		SHAPE_LOG(name, LUX_WARNING,LUX_BADTOKEN) << "Quad type  '" << quadTypeStr << "' unknown. Using \"quadrilateral\".";
		quadType = Mesh::QUAD_QUADRILATERAL;
	}

	if (quadIndices) {
		for (u_int i = 0; i < quadIndicesCount; ++i) {
			if (static_cast<u_int>(quadIndices[i]) >= npi) {
				SHAPE_LOG(name, LUX_ERROR,LUX_CONSISTENCY) << "Mesh has out of-bounds quad vertex index " << quadIndices[i] <<
						" (" << npi << "  \"P\" values were given";
				return NULL;
			}
		}

		quadIndicesCount /= 4;
	} else
		quadIndicesCount = 0;

	if ((!triIndices) && (!quadIndices))
		return NULL;

	// Dade - the optional displacement map
	string displacementMapName = params.FindOneString("displacementmap", "");
	float displacementMapScale = params.FindOneFloat("dmscale", 0.1f);
	float displacementMapOffset = params.FindOneFloat("dmoffset", 0.0f);
	bool displacementMapNormalSmooth = params.FindOneBool("dmnormalsmooth", true);
	bool displacementMapSharpBoundary = params.FindOneBool("dmsharpboundary", false);
	bool normalSplit = params.FindOneBool("dmnormalsplit", false);

	boost::shared_ptr<Texture<float> > displacementMap;
	if (displacementMapName != "") {
		// Lotus - read subdivision data
		map<string, boost::shared_ptr<Texture<float> > > *floatTextures = Context::GetActiveFloatTextures();

		boost::shared_ptr<Texture<float> > dm((*floatTextures)[displacementMapName]);
		displacementMap = dm;

		if (!displacementMap) {
			SHAPE_LOG(name, LUX_WARNING,LUX_SYNTAX) << "Unknown float texture '" << displacementMapName << "'.";
		}
	}

	Mesh::MeshSubdivType subdivType;
	if (subdivSchemeStr == "loop")
		subdivType = Mesh::SUBDIV_LOOP;
	else if (subdivSchemeStr == "microdisplacement")
		subdivType = Mesh::SUBDIV_MICRODISPLACEMENT;
	else {
		SHAPE_LOG(name, LUX_WARNING,LUX_BADTOKEN) << "Subdivision type  '" << subdivSchemeStr << "' unknown. Using \"loop\".";
		subdivType = Mesh::SUBDIV_LOOP;
	}

	bool genTangents = params.FindOneBool("generatetangents", false);

	return new Mesh(o2w, reverseOrientation, name,
		accelType,
		npi, P, N, UV, cols,
		triType, triIndicesCount, triIndices,
		quadType, quadIndicesCount, quadIndices,
		subdivType, nSubdivLevels, displacementMap,
		displacementMapScale, displacementMapOffset,
		displacementMapNormalSmooth, displacementMapSharpBoundary,
		normalSplit, genTangents);
}

static Shape *CreateShape( const Transform &o2w, bool reverseOrientation, const ParamSet &params,
						   string accelTypeStr, string triTypeStr) {
	// Vertex and attributes
	u_int npi;
	const Point *P = params.FindPoint("P", &npi);
	
	u_int nni;
	const Normal *N = params.FindNormal("N", &nni);
	
	u_int UVCount;
	const float *UV = params.FindFloat("uv", &UVCount);
	if (UV == NULL) {
		UV = params.FindFloat("st", &UVCount);
	}

	u_int colsCount;
	const float *cols = params.FindFloat("C", &colsCount);

	// Triangles
	u_int triIndicesCount;
	const int *triIndices = params.FindInt("triindices", &triIndicesCount);
	if(triIndices == NULL)
	{
		triIndices = params.FindInt("indices", &triIndicesCount);
	}
 
	triTypeStr = params.FindOneString("tritype", triTypeStr);

	// Quads
	string quadTypeStr = params.FindOneString("quadtype", "quadrilateral");
	u_int quadIndicesCount;
	const int *quadIndices = params.FindInt("quadindices", &quadIndicesCount);

	// Mesh parameters
	accelTypeStr = params.FindOneString("acceltype", accelTypeStr);
 	string subdivscheme = params.FindOneString("subdivscheme", "loop");
	int nSubdivLevels = max(0, params.FindOneInt("nsubdivlevels", params.FindOneInt("nlevels", 0)));

	return CreateShape(o2w, reverseOrientation, params,
		accelTypeStr, triTypeStr, quadTypeStr,
		triIndices, triIndicesCount,
		quadIndices, quadIndicesCount,
		UV, UVCount,
		cols, colsCount,
		subdivscheme, nSubdivLevels,
		P, npi,
		N, nni);
}

Shape *Mesh::CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params) {
	return ::CreateShape( o2w, reverseOrientation, params, "auto", "auto");
}

static DynamicLoader::RegisterShape<Mesh> r("mesh");

Shape* Mesh::BaryMesh::CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params) {
	return ::CreateShape( o2w, reverseOrientation, params, "auto", "bary");
}

static DynamicLoader::RegisterShape<Mesh::BaryMesh> rbary("barytrianglemesh");
static DynamicLoader::RegisterShape<Mesh> rwald1("waldtrianglemesh");
static DynamicLoader::RegisterShape<Mesh> rwald2("trianglemesh");
static DynamicLoader::RegisterShape<Mesh> rloop("loopsubdiv");
