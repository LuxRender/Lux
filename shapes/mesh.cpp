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

#include "mesh.h"
#include "dynload.h"

using namespace lux;

Mesh::Mesh(const Transform &o2w, bool ro,
			MeshAccelType acceltype,
			int nv, const Point *P, const Normal *N, const float *UV,
			MeshTriangleType tritype, int trisCount, const int *tris,
			MeshQuadType quadtype, int nquadsCount, const int *quads) : Shape(o2w, ro)
{
	accelType = acceltype;

	// TODO: use AllocAligned

	// Dade - copy vertex data
    nverts = nv;
	p = new Point[nverts];
    // Dade - transform mesh vertices to world space
    for (int i  = 0; i < nverts; ++i)
        p[i] = ObjectToWorld(P[i]);

	// Dade - copy UV and N vertex data, if present
    if (UV) {
        uvs = new float[2 * nverts];
        memcpy(uvs, UV, 2 * nverts*sizeof(float));
    } else
		uvs = NULL;

    if (N) {
        n = new Normal[nverts];
        memcpy(n, N, nverts * sizeof(Normal));
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
		// Dade - check if quads are not planar and split them if required
		for (int i = 0; i < nquads; i++) {
			const int idx = 4 * i;
			const Point &p0 = p[quads[idx]];
			const Point &p1 = p[quads[idx + 1]];
			const Point &p2 = p[quads[idx + 2]];
			const Point &p3 = p[quads[idx + 3]];

			if (MeshQuadrilateral::IsPlanar(p0, p1, p2, p3)) {
				quadsOk.push_back(quads[idx]);
				quadsOk.push_back(quads[idx + 1]);
				quadsOk.push_back(quads[idx + 2]);
				quadsOk.push_back(quads[idx + 3]);
			} else {
				// Dade - it is a no planar quad, I need to split in 2 x triangles
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
			for (int i = 0; i < nquads; i++) {
				const int idx = 4 * i;
				quadVertexIndex[idx] = quadsOk[idx];
				quadVertexIndex[idx + 1] = quadsOk[idx + 1];
				quadVertexIndex[idx + 2] = quadsOk[idx + 2];
				quadVertexIndex[idx + 3] = quadsOk[idx + 3];
			}
		}
	}

	std::stringstream ss;
	ss << "Mesh: " <<
			trisCount << " triangles  " <<
			(quadsOk.size() / 4) << " planar quads  " <<
			(quadsToSplit.size() / 4) << " no-planar quads";
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	// Dade - copy triangle data
	triType = tritype;
	ntris = trisCount;
	if (ntris == 0) {
		if (quadsToSplit.size() == 0)
			triVertexIndex = NULL;
		else {
			// Dade - add quads to split
			const size_t nquadsToSplit = quadsToSplit.size() / 4;
			ntris = 2 * nquadsToSplit;
			triVertexIndex = new int[3 * ntris];

			for (size_t i = 0; i < nquadsToSplit; i++) {
				const size_t qidx = 4 * i;
				const size_t tidx = 2 * 3 * i;

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
	} else {
		const size_t nquadsToSplit = quadsToSplit.size() / 4;
		ntris += 2 * nquadsToSplit;
		triVertexIndex = new int[3 * ntris];
		memcpy(triVertexIndex, tris, 3 * trisCount * sizeof(int));

		// Dade - add quads to split
		for (size_t i = 0; i < nquadsToSplit; i++) {
			const size_t qidx = 4 * i;
			const size_t tidx = 3 * trisCount + 2 * 3 * i;

			// Dade - triangle A
			triVertexIndex[tidx] = quadsToSplit[qidx];
			triVertexIndex[tidx + 1] = quadsToSplit[qidx + 1];
			triVertexIndex[tidx + 2] = quadsToSplit[qidx + 2];
			// Dade - triangle b
			triVertexIndex[tidx + 3] = quadsToSplit[qidx];
			triVertexIndex[tidx + 4] = quadsToSplit[qidx + 2];
			triVertexIndex[tidx + 5] = quadsToSplit[qidx + 3];
		}
	}
}

Mesh::~Mesh() {
    delete[] triVertexIndex;
	delete[] quadVertexIndex;
    delete[] p;
    delete[] n;
    delete[] uvs;
}

BBox Mesh::ObjectBound() const {
    BBox bobj;
    for (int i = 0; i < nverts; i++)
        bobj = Union(bobj, WorldToObject(p[i]));
    return bobj;
}

BBox Mesh::WorldBound() const {
    BBox worldBounds;
    for (int i = 0; i < nverts; i++)
        worldBounds = Union(worldBounds, p[i]);
    return worldBounds;
}

template<class T>
class MeshElemSharedPtr : public T {
public:
	MeshElemSharedPtr(const Mesh* m, int n,
			boost::shared_ptr<Primitive> aPtr)
	: T(m,n), ptr(aPtr)
	{
	}
private:
	const boost::shared_ptr<Primitive> ptr;
};

void Mesh::Refine(vector<boost::shared_ptr<Primitive> > &refined,
		const PrimitiveRefinementHints &refineHints,
		boost::shared_ptr<Primitive> thisPtr)
{
	vector<boost::shared_ptr<Primitive> > refinedPrims;
	refinedPrims.reserve(ntris + nquads);
	// Dade - refine triangles
	MeshTriangleType concreteTriType = triType;
	if(triType == TRI_AUTO) {
		// If there is 1 unique vertex (with normals and uv coordinates) for each triangle:
		//  bary = 52 bytes/triangle
		//  wald = 128 bytes/triangle
		// Note: this ignores some accel data
		//  the following are accounted for: vertices, vertex indices, Mesh*Triangle data
		//									 and one shared_ptr in the accel
		//TODO Lotus - find good values
		if(ntris <= 500000)
			concreteTriType = TRI_WALD;
		else
			concreteTriType = TRI_BARY;
	}
	switch (concreteTriType) {
		case TRI_WALD:
			for (int i = 0; i < ntris; ++i) {
				MeshWaldTriangle* currTri = new MeshWaldTriangle(this, i);
				if(!currTri->isDegenerate()) {
					if(refinedPrims.size() > 0) {
						boost::shared_ptr<Primitive> o(currTri);
						refinedPrims.push_back(o);
					}
					else {
						delete currTri;
						boost::shared_ptr<Primitive> o(
								new MeshElemSharedPtr<MeshWaldTriangle>(this, i, thisPtr));
						refinedPrims.push_back(o);
					}
				}
				else
					delete currTri;
			}
			break;
		case TRI_BARY:
			for (int i = 0; i < ntris; ++i) {
				MeshBaryTriangle* currTri = new MeshBaryTriangle(this, i);
				if(!currTri->isDegenerate()) {
					if(refinedPrims.size() > 0) {
						boost::shared_ptr<Primitive> o(currTri);
						refinedPrims.push_back(o);
					}
					else {
						delete currTri;
						boost::shared_ptr<Primitive> o(
								new MeshElemSharedPtr<MeshBaryTriangle>(this, i, thisPtr));
						refinedPrims.push_back(o);
					}
				}
				else
					delete currTri;
			}
			break;
		default:
			{
				std::stringstream ss;
				ss.str("");
				ss << "Unknow triangle type in a mesh: " << concreteTriType;
				luxError(LUX_CONSISTENCY, LUX_ERROR, ss.str().c_str());
			}
			break;
	}

	// Dade - refine quads
	switch (quadType) {
		case QUAD_QUADRILATERAL:
			for (int i = 0; i < nquads; ++i) {
				MeshQuadrilateral* currQuad = new MeshQuadrilateral(this, i);
				if(!currQuad->isDegenerate()) {
					if(refinedPrims.size() > 0) {
						boost::shared_ptr<Primitive> o(currQuad);
						refinedPrims.push_back(o);
					}
					else {
						delete currQuad;
						boost::shared_ptr<Primitive> o(
								new MeshElemSharedPtr<MeshQuadrilateral>(this, i, thisPtr));
						refinedPrims.push_back(o);
					}
				}
				else
					delete currQuad;
			}
			break;
		default:
			{
				std::stringstream ss;
				ss.str("");
				ss << "Unknow quad type in a mesh: " << quadType;
				luxError(LUX_CONSISTENCY, LUX_ERROR, ss.str().c_str());
			}
			break;
	}

	// Lotus - Create acceleration structure
	MeshAccelType concreteAccelType = accelType;
	if(accelType == ACCEL_AUTO) {
		//TODO find good values
		if(refinedPrims.size() <= 3)
			concreteAccelType = ACCEL_NONE;
		else if(refinedPrims.size() <= 1200000)
			concreteAccelType = ACCEL_KDTREE;
		else
			concreteAccelType = ACCEL_GRID;
	}
	if(concreteAccelType == ACCEL_NONE) {
		// Copy primitives
		for(u_int i=0; i < refinedPrims.size(); i++)
			refined.push_back(refinedPrims[i]);
	}
	else  {
		ParamSet paramset;
		boost::shared_ptr<Aggregate> accel;
		if(concreteAccelType == ACCEL_KDTREE) {
			accel = MakeAccelerator("kdtree", refined, paramset);
		}
		else if(concreteAccelType == ACCEL_GRID) {
			accel = MakeAccelerator("grid", refined, paramset);
		}
		else {
			std::stringstream ss;
			ss.str("");
			ss << "Unknow accel type in a mesh: " << concreteAccelType;
			luxError(LUX_CONSISTENCY, LUX_ERROR, ss.str().c_str());
		}
		if(refineHints.forSampling) {
			// Lotus - create primitive set to allow sampling
			refined.push_back(boost::shared_ptr<Primitive>(new PrimitiveSet(accel)));
		}
		else {
			refined.push_back(accel);
		}
	}
}

Shape *Mesh::CreateShape(const Transform &o2w,
		bool reverseOrientation, const ParamSet &params) {
	// Lotus - read general data
	MeshAccelType accelType;
	string accelTypeStr = params.FindOneString("acceltype", "auto");
	if (accelTypeStr == "kdtree") accelType = ACCEL_KDTREE;
	else if (accelTypeStr == "grid") accelType = ACCEL_GRID;
	else if (accelTypeStr == "none") accelType = ACCEL_NONE;
	else if (accelTypeStr == "auto") accelType = ACCEL_AUTO;
	else {
		std::stringstream ss;
		ss << "Acceleration structure type  '" << accelTypeStr << "' unknown. Using \"auto\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		accelType = ACCEL_AUTO;
	}

	// Dade - read vertex data
    int npi;
    const Point *P = params.FindPoint("P", &npi);
	int nuvi;
    const float *UV = params.FindFloat("UV", &nuvi);

    // NOTE - lordcrc - Bugfix, pbrt tracker id 0000085: check for correct number of uvs
    if (UV && (nuvi != npi * 2)) {
        luxError(LUX_CONSISTENCY, LUX_ERROR, "Number of \"UV\"s for mesh must match \"P\"s");
        UV = NULL;
    }
    if (!P) return NULL;

	int nni;
	const Normal *N = params.FindNormal("N", &nni);
    if (N && (nni != npi)) {
        luxError(LUX_CONSISTENCY, LUX_ERROR, "Number of \"N\"s for mesh must match \"P\"s");
        N = NULL;
    }

	// Dade - read triangle data
	MeshTriangleType triType;
	string triTypeStr = params.FindOneString("tritype", "auto");
	if (triTypeStr == "wald") triType = TRI_WALD;
	else if (triTypeStr == "bary") triType = TRI_BARY;
	else if (triTypeStr == "auto") triType = TRI_AUTO;
	else {
		std::stringstream ss;
		ss << "Triangle type  '" << triTypeStr << "' unknown. Using \"auto\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		triType = TRI_AUTO;
	}

	int triIndicesCount;
	const int *triIndices = params.FindInt("triindices", &triIndicesCount);
	if (triIndices) {
		for (int i = 0; i < triIndicesCount; ++i) {
			if (triIndices[i] >= npi) {
				std::stringstream ss;
				ss << "Mesh has out of-bounds triangle vertex index " << triIndices[i] <<
						" (" << npi << "  \"P\" values were given";
				luxError(LUX_CONSISTENCY, LUX_ERROR, ss.str().c_str());
				return NULL;
			}
		}

		triIndicesCount /= 3;
	} else
		triIndicesCount = 0;

	// Dade - read quad data
	MeshQuadType quadType;
	string quadTypeStr = params.FindOneString("quadtype", "quadrilateral");
	if (quadTypeStr == "quadrilateral") quadType = QUAD_QUADRILATERAL;
	else {
		std::stringstream ss;
		ss << "Quad type  '" << quadTypeStr << "' unknown. Using \"quadrilateral\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		quadType = QUAD_QUADRILATERAL;
	}

	int quadIndicesCount;
	const int *quadIndices = params.FindInt("quadindices", &quadIndicesCount);
	if (quadIndices) {
		for (int i = 0; i < quadIndicesCount; ++i) {
			if (quadIndices[i] >= npi) {
				std::stringstream ss;
				ss << "Mesh has out of-bounds quad vertex index " << quadIndices[i] <<
						" (" << npi << "  \"P\" values were given";
				luxError(LUX_CONSISTENCY, LUX_ERROR, ss.str().c_str());
				return NULL;
			}
		}

		quadIndicesCount /= 4;
	} else
		quadIndicesCount = 0;

	if ((!triIndices) && (!quadIndices)) return NULL;

    return new Mesh(o2w, reverseOrientation,
			accelType,
			npi, P, N, UV,
			triType, triIndicesCount, triIndices,
			quadType, quadIndicesCount, quadIndices);
}

static DynamicLoader::RegisterShape<Mesh> r("mesh");
