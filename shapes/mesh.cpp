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

#include "mesh.h"
#include "dynload.h"
#include "context.h"
#include "loopsubdiv.h"

#include "luxrays/core/trianglemesh.h"

using namespace lux;

Mesh::Mesh(const Transform &o2w, bool ro, MeshAccelType acceltype,
	u_int nv, const Point *P, const Normal *N, const float *UV,
	MeshTriangleType tritype, u_int trisCount, const int *tris,
	MeshQuadType quadtype, u_int nquadsCount, const int *quads,
	MeshSubdivType subdivtype, u_int nsubdivlevels,
	boost::shared_ptr<Texture<float> > &dmMap, float dmScale, float dmOffset,
	bool dmNormalSmooth, bool dmSharpBoundary) : Shape(o2w, ro)
{
	accelType = acceltype;

	subdivType = subdivtype;
	nSubdivLevels = nsubdivlevels;
	displacementMap = dmMap;
	displacementMapScale = dmScale;
	displacementMapOffset = dmOffset;
	displacementMapNormalSmooth = dmNormalSmooth;
	displacementMapSharpBoundary = dmSharpBoundary;
	mustSubdivide = nSubdivLevels > 0;

	// TODO: use AllocAligned

	// Dade - copy vertex data
	nverts = nv;
	p = new Point[nverts];
	if (!mustSubdivide) {
		// Dade - transform mesh vertices to world space
		for (u_int i  = 0; i < nverts; ++i)
			p[i] = ObjectToWorld(P[i]);
	} else {
		// Dont transform the mesh vertices to world space yet if subdivision is required
		memcpy(p, P, nverts * sizeof(Point));
	}

	// Dade - copy UV and N vertex data, if present
	if (UV) {
		uvs = new float[2 * nverts];
		memcpy(uvs, UV, 2 * nverts*sizeof(float));
	} else
		uvs = NULL;

	if (N) {
		n = new Normal[nverts];
		if (!mustSubdivide) {
			// Dade - transform mesh normals to world space
			for (u_int i  = 0; i < nverts; ++i) {
				if (ro)
					n[i] = Normalize(-ObjectToWorld(N[i]));
				else
					n[i] = Normalize(ObjectToWorld(N[i]));
			}
		} else {
			// Dont transform the mesh normals to world space yet if subdivision is required
			memcpy(n, N, nverts * sizeof(Normal));
		}
	} else
		n = NULL;

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
			const Point &p0 = p[quads[idx]];
			const Point &p1 = p[quads[idx + 1]];
			const Point &p2 = p[quads[idx + 2]];
			const Point &p3 = p[quads[idx + 3]];

			// Split the quad if subdivision is necessary (only possible on tri's) or if its not planar or convex
			bool quadOk = MeshQuadrilateral::IsPlanar(p0, p1, p2, p3) && MeshQuadrilateral::IsConvex(p0, p1, p2, p3);
			// TODO - quads have UV mapping issue, split them if they have UVs
			quadOk &= uvs == NULL;
			if (!mustSubdivide && quadOk) {
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
		if( nSubdivLevels > 0 )
			ss << " to allow subdivision";
		else
			ss << " because they are non-planar or non-convex";
		LOG(LUX_INFO,LUX_NOERROR)<< ss.str().c_str();
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

			// Dade - triangle A
			triVertexIndex[tidx] = quadsToSplit[qidx];
			triVertexIndex[tidx + 1] = quadsToSplit[qidx + 1];
			triVertexIndex[tidx + 2] = quadsToSplit[qidx + 2];
			// Dade - triangle B
			triVertexIndex[tidx + 3] = quadsToSplit[qidx];
			triVertexIndex[tidx + 4] = quadsToSplit[qidx + 2];
			triVertexIndex[tidx + 5] = quadsToSplit[qidx + 3];
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
}

BBox Mesh::ObjectBound() const
{
	BBox bobj;
	for (u_int i = 0; i < nverts; ++i)
		bobj = Union(bobj, WorldToObject(p[i]));
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
				// Apply subdivision
				LoopSubdiv loopsubdiv(ObjectToWorld, reverseOrientation,
					ntris, nverts, triVertexIndex, p, uvs,
					nSubdivLevels, displacementMap,
					displacementMapScale, displacementMapOffset,
					displacementMapNormalSmooth, displacementMapSharpBoundary);
				boost::shared_ptr<LoopSubdiv::SubdivResult> res(loopsubdiv.Refine());
				// Check if subdivision was successfull
				if (!res)
					break;

				// Remove the old mesh data
				delete[] p;
				delete[] n;
				delete[] uvs;
				delete[] triVertexIndex;

				// Copy the new mesh data
				nverts = res->nverts;
				ntris = res->ntris;
				triVertexIndex = new int[3 * ntris];
				memcpy(triVertexIndex, res->indices, 3 * ntris * sizeof(int));
				p = new Point[nverts];
				for (u_int i = 0; i < nverts; ++i)
					p[i] = ObjectToWorld(res->P[i]);
				if (res->uv) {
					uvs = new float[2 * nverts];
					memcpy(uvs, res->uv, 2 * nverts * sizeof(float));
				} else
					uvs = NULL;

				if (res->N) {
					n = new Normal[nverts];
					for (u_int i = 0; i < nverts; ++i)
						n[i] = Normalize(ObjectToWorld(res->N[i]));
				} else
					n = NULL;

				break;
			}
			default: {
				LOG( LUX_ERROR,LUX_CONSISTENCY) << "Unknow subdivision type in a mesh: " << concreteSubdivType;
				break;
			}
		}

		mustSubdivide = false; // only subdivide on the first refine!!!
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
		default: {
			LOG( LUX_ERROR,LUX_CONSISTENCY) << "Unknow triangle type in a mesh: " << concreteTriType;
			break;
		}
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
			LOG( LUX_ERROR,LUX_CONSISTENCY) << "Unknow quad type in a mesh: " << quadType;
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
	LOG(LUX_DEBUG,LUX_NOERROR)<< ss.str().c_str();

	// Build acceleration structure
	if (concreteAccelType == ACCEL_NONE) {
		// Copy primitives
		// NOTE - lordcrc - use resize+swap to avoid shared_ptr count from changing
		const u_int offset = refined.size();
		refined.resize(refined.size() + refinedPrims.size());
		for(u_int i = 0; i < refinedPrims.size(); ++i)
			refined[offset+i].swap(refinedPrims[i]);
	} else  {
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
				LOG( LUX_ERROR,LUX_CONSISTENCY) << "Unknow accel type in a mesh: " << concreteAccelType;
		}
		if (refineHints.forSampling)
			// Lotus - create primitive set to allow sampling
			refined.push_back(boost::shared_ptr<Primitive>(new PrimitiveSet(accel)));
		else
			refined.push_back(accel);
	}
}

void Mesh::Tesselate(vector<luxrays::TriangleMesh *> *meshList, vector<const Primitive *> *primitiveList) const {
	// A little hack with pointers
	luxrays::TriangleMesh *tm = new luxrays::TriangleMesh(
			nverts, ntris,
			(luxrays::Point *)p, (luxrays::Triangle *)triVertexIndex);

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

	const float b0 = 1.f - (rayHit.b1 + rayHit.b2);
	const float b1 = rayHit.b1;
	const float b2 = rayHit.b2;

	// Interpolate $(u,v)$ triangle parametric coordinates
	const float tu = b0 * uv[0][0] + b1 * uv[1][0] + b2 * uv[2][0];
	const float tv = b0 * uv[0][1] + b1 * uv[1][1] + b2 * uv[2][1];

	const Normal nn = Normal(Normalize(Cross(e1, e2)));
	const Point pp(p1 + b1 * e1 + b2 * e2);

	isect->dg = DifferentialGeometry(pp, nn, dpdu, dpdv,
		Normal(0, 0, 0), Normal(0, 0, 0), tu, tv, this);

	isect->Set(WorldToObject, this, GetMaterial(),
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
	const Normal ns = Normalize(dg.iData.mesh.coords[0] * n[v0] +
		dg.iData.mesh.coords[1] * n[v1] + dg.iData.mesh.coords[2] * n[v2]);

	Vector ts(Normalize(Cross(ns, dg.dpdu)));
	Vector ss(Cross(ts, ns));
	// Lotus - the length of dpdu/dpdv can be important for bumpmapping
	ss *= dg.dpdu.Length();
	if (Dot(dg.dpdv, ts) < 0.f)
		ts *= -dg.dpdv.Length();
	else
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
		dndu, dndv, dg.u, dg.v, this);
}

static Shape *CreateShape( const Transform &o2w, bool reverseOrientation, const ParamSet &params,
						   const string& accelTypeStr, const string& triTypeStr,
						   const int* triIndices, u_int triIndicesCount,
						   const float* UV, u_int UVCount,
						   const string& subdivSchemeStr, u_int nSubdivLevels) {
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
		LOG(LUX_WARNING,LUX_BADTOKEN) << "Acceleration structure type  '" << accelTypeStr << "' unknown. Using \"auto\".";
		accelType = Mesh::ACCEL_AUTO;
	}

	// Dade - read vertex data
	u_int npi;
	const Point *P = params.FindPoint("P", &npi);

	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000085: check for correct number of uvs
	if (UV && (UVCount != npi * 2)) {
		LOG( LUX_ERROR,LUX_CONSISTENCY)<< "Number of \"UV\"s for mesh must match \"P\"s";
		UV = NULL;
	}
	if (!P)
		return NULL;

	u_int nni;
	const Normal *N = params.FindNormal("N", &nni);
	if (N && (nni != npi)) {
		LOG( LUX_ERROR,LUX_CONSISTENCY)<< "Number of \"N\"s for mesh must match \"P\"s";
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
		LOG(LUX_WARNING,LUX_BADTOKEN) << "Triangle type  '" << triTypeStr << "' unknown. Using \"auto\".";
		triType = Mesh::TRI_AUTO;
	}

	if (triIndices) {
		for (u_int i = 0; i < triIndicesCount; ++i) {
			if (static_cast<u_int>(triIndices[i]) >= npi) {
				LOG( LUX_ERROR,LUX_CONSISTENCY) << "Mesh has out of-bounds triangle vertex index " << triIndices[i] <<
						" (" << npi << "  \"P\" values were given";
				return NULL;
			}
		}

		triIndicesCount /= 3;
	} else
		triIndicesCount = 0;

	// Dade - read quad data
	Mesh::MeshQuadType quadType;
	string quadTypeStr = params.FindOneString("quadtype", "quadrilateral");
	if (quadTypeStr == "quadrilateral") quadType = Mesh::QUAD_QUADRILATERAL;
	else {
		LOG(LUX_WARNING,LUX_BADTOKEN) << "Quad type  '" << quadTypeStr << "' unknown. Using \"quadrilateral\".";
		quadType = Mesh::QUAD_QUADRILATERAL;
	}

	u_int quadIndicesCount;
	const int *quadIndices = params.FindInt("quadindices", &quadIndicesCount);
	if (quadIndices) {
		for (u_int i = 0; i < quadIndicesCount; ++i) {
			if (static_cast<u_int>(quadIndices[i]) >= npi) {
				LOG( LUX_ERROR,LUX_CONSISTENCY) << "Mesh has out of-bounds quad vertex index " << quadIndices[i] <<
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

	boost::shared_ptr<Texture<float> > displacementMap;
	if (displacementMapName != "") {
		// Lotus - read subdivision data
		map<string, boost::shared_ptr<Texture<float> > > *floatTextures = Context::GetActiveFloatTextures();

		boost::shared_ptr<Texture<float> > dm((*floatTextures)[displacementMapName]);
		displacementMap = dm;

		if (displacementMap.get() == NULL) {
			LOG( LUX_WARNING,LUX_SYNTAX) << "Unknow float texture '" << displacementMapName << "' in a Mesh shape.";
		}
	}

	// don't actually use this for now...
	Mesh::MeshSubdivType subdivType;
	if (subdivSchemeStr == "loop")
		subdivType = Mesh::SUBDIV_LOOP;
	else {
		LOG(LUX_WARNING,LUX_BADTOKEN) << "Subdivision type  '" << subdivSchemeStr << "' unknown. Using \"loop\".";
		subdivType = Mesh::SUBDIV_LOOP;
	}

	return new Mesh(o2w, reverseOrientation,
		accelType,
		npi, P, N, UV,
		triType, triIndicesCount, triIndices,
		quadType, quadIndicesCount, quadIndices,
		subdivType, nSubdivLevels, displacementMap, displacementMapScale, displacementMapOffset,
		displacementMapNormalSmooth, displacementMapSharpBoundary);
}

Shape *Mesh::CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params) {
	string accelTypeStr = params.FindOneString("acceltype", "auto");

	string triTypeStr = params.FindOneString("tritype", "auto");
	u_int triIndicesCount;
	const int *triIndices = params.FindInt("triindices", &triIndicesCount);
	u_int uvCoordinatesCount;
	const float *uvCoordinates = params.FindFloat("uv", &uvCoordinatesCount);

	string subdivscheme = params.FindOneString("subdivscheme", "loop");
	int nsubdivlevels = params.FindOneInt("nsubdivlevels", 0);

	return ::CreateShape( o2w, reverseOrientation, params, accelTypeStr, triTypeStr,
		triIndices, triIndicesCount, uvCoordinates, uvCoordinatesCount,
		subdivscheme, max(0, nsubdivlevels));
}

static DynamicLoader::RegisterShape<Mesh> r("mesh");

Shape* Mesh::BaryMesh::CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params) {
	string accelTypeStr = "auto";
	string triTypeStr = "bary";
	u_int indicesCount;
	const int* indices = params.FindInt( "indices", &indicesCount );
	u_int uvCoordinatesCount;
	const float *uvCoordinates = params.FindFloat("uv", &uvCoordinatesCount);
	if (uvCoordinates == NULL) {
		uvCoordinates = params.FindFloat("st", &uvCoordinatesCount);
	}
	return ::CreateShape( o2w, reverseOrientation, params, accelTypeStr, triTypeStr,
		indices, indicesCount, uvCoordinates, uvCoordinatesCount,
		"loop", 0U);
}

static DynamicLoader::RegisterShape<Mesh::BaryMesh> rbary("barytrianglemesh");

Shape* Mesh::WaldMesh::CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params) {
	string accelTypeStr = "auto";
	string triTypeStr = "auto";
	u_int indicesCount;
	const int* indices = params.FindInt( "indices", &indicesCount );
	u_int uvCoordinatesCount;
	const float *uvCoordinates = params.FindFloat("uv", &uvCoordinatesCount);
	if (uvCoordinates == NULL) {
		uvCoordinates = params.FindFloat("st", &uvCoordinatesCount);
	}
	return ::CreateShape( o2w, reverseOrientation, params, accelTypeStr, triTypeStr,
		indices, indicesCount, uvCoordinates, uvCoordinatesCount,
		"loop", 0U);
}

static DynamicLoader::RegisterShape<Mesh::WaldMesh> rwald1("waldtrianglemesh");
static DynamicLoader::RegisterShape<Mesh::WaldMesh> rwald2("trianglemesh");

Shape* Mesh::LoopMesh::CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params) {
	string accelTypeStr = "auto";
	string triTypeStr = "auto";

	u_int indicesCount;
	const int* indices = params.FindInt( "indices", &indicesCount );
	u_int uvCoordinatesCount;
	const float *uvCoordinates = params.FindFloat("uv", &uvCoordinatesCount);
	if (uvCoordinates == NULL) {
		uvCoordinates = params.FindFloat("st", &uvCoordinatesCount);
	}

	string subdivscheme = params.FindOneString("scheme", "loop");
	int nsubdivlevels = params.FindOneInt("nlevels", 3);

	return ::CreateShape( o2w, reverseOrientation, params, accelTypeStr, triTypeStr,
		indices, indicesCount, uvCoordinates, uvCoordinatesCount,
		subdivscheme, max(0, nsubdivlevels));
}

static DynamicLoader::RegisterShape<Mesh::LoopMesh> rloop("loopsubdiv");
