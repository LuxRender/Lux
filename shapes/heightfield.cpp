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

// heightfield.cpp*
#include "heightfield.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Heightfield Method Definitions
Heightfield::Heightfield(const Transform &o2w, bool ro, const string &name, 
		u_int x, u_int y, const float *zs)
	: Shape(o2w, ro, name) {
	nx = x;
	ny = y;
	z = new float[nx*ny];
	memcpy(z, zs, nx*ny*sizeof(float));
}
Heightfield::~Heightfield() {
	delete[] z;
}
BBox Heightfield::ObjectBound() const {
	float minz = z[0], maxz = z[0];
	for (u_int i = 1; i < nx*ny; ++i) {
		if (z[i] < minz) minz = z[i];
		if (z[i] > maxz) maxz = z[i];
	}
	return BBox(Point(0,0,minz), Point(1,1,maxz));
}
bool Heightfield::CanIntersect() const {
	return false;
}
void Heightfield::Refine(vector<boost::shared_ptr<Shape> > &refined) const {
	const u_int nVerts = nx * ny;
	Point *P = new Point[nVerts];
	float *uvs = new float[2 * nVerts];
	// Compute heightfield vertex positions
	u_int pos = 0;
	for (u_int y = 0; y < ny; ++y) {
		for (u_int x = 0; x < nx; ++x) {
			P[pos].x = uvs[2 * pos]   = static_cast<float>(x) / static_cast<float>(nx - 1);
			P[pos].y = uvs[2 * pos + 1] = static_cast<float>(y) / static_cast<float>(ny - 1);
			P[pos].z = z[pos];
			++pos;
		}
	}
	// Fill in heightfield vertex offset array
	const u_int nTris = 2 * (nx - 1) * (ny - 1);
	int *verts = new int[3 * nTris];
	int *vp = verts;
	for (u_int y = 0; y < ny - 1; ++y) {
		for (u_int x = 0; x < nx - 1; ++x) {
	#define VERT(x,y) static_cast<int>((x) + (y) * nx)
			*vp++ = VERT(x, y);
			*vp++ = VERT(x+1, y);
			*vp++ = VERT(x+1, y+1);
	
			*vp++ = VERT(x, y);
			*vp++ = VERT(x+1, y+1);
			*vp++ = VERT(x, y+1);
		}
	#undef VERT
	}
	ParamSet paramSet;
	paramSet.AddInt("indices", verts, 3 * nTris);
	paramSet.AddFloat("uv", uvs, 2 * nVerts);
	paramSet.AddPoint("P", P, nVerts);
	refined.reserve(nTris);
	refined.push_back(MakeShape("trianglemesh",
			ObjectToWorld, reverseOrientation, paramSet));
	delete[] P;
	delete[] uvs;
	delete[] verts;
}
Shape* Heightfield::CreateShape(const Transform &o2w,
		bool reverseOrientation, const ParamSet &params) {
	string name = params.FindOneString("name", "'heightfield'");
	int nu = params.FindOneInt("nu", 0);
	int nv = params.FindOneInt("nv", 0);
	u_int nItems;
	const float *Pz = params.FindFloat("Pz", &nItems);
	if (nu <= 0 || nv <= 0 || nItems != static_cast<u_int>(nu * nv))
		return NULL;
	BOOST_ASSERT(nItems == static_cast<u_int>(nu*nv));
	BOOST_ASSERT(nu != -1 && nv != -1 && Pz != NULL);
	return new Heightfield(o2w, reverseOrientation, name, nu, nv, Pz);
}

static DynamicLoader::RegisterShape<Heightfield> r("heightfield");
