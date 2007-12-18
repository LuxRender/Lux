/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#include "plymesh.h"

#include "trianglemesh.h"
#include "./plymesh/rply.h"

// rply vertex callback
static int VertexCB(p_ply_argument argument) 
{
	long current_cmpnt = 0;
	void* pointer_usrdata = NULL;
	ply_get_argument_user_data(argument, &pointer_usrdata, &current_cmpnt);

	PlyMesh* mesh = static_cast<PlyMesh*>(pointer_usrdata);

	long vertindex;
	ply_get_argument_element(argument, NULL, &vertindex);

	if(current_cmpnt == 0)
		mesh->p[vertindex].x = (float) ply_get_argument_value(argument);
	else if(current_cmpnt == 1)
		mesh->p[vertindex].y = (float) ply_get_argument_value(argument);
	else if(current_cmpnt == 2)
		mesh->p[vertindex].z = (float) ply_get_argument_value(argument);

    return 1;
}

// rply face callback
static int FaceCB(p_ply_argument argument) 
{
	void* pointer_usrdata = NULL;
	ply_get_argument_user_data(argument, &pointer_usrdata, NULL);

	PlyMesh* mesh = static_cast<PlyMesh*>(pointer_usrdata);

	long tri_index;
	ply_get_argument_element(argument, NULL, &tri_index);

	long length, value_index;
    ply_get_argument_property(argument, NULL, &length, &value_index);

	if(value_index >= 0 && value_index < 3)
	{
		mesh->vertexIndex[(tri_index * 3) + value_index] = 
			(int)ply_get_argument_value(argument);
	}

    return 1;
}

// PlyMesh Method Definitions
PlyMesh::PlyMesh(const Transform &o2w, bool ro, string filename, bool smooth)
	: Shape(o2w, ro) {

	printf("Loading PLY mesh file: '%s'...\n", filename.c_str());

    p_ply plyfile = ply_open(filename.c_str(), NULL);

	if(!plyfile) {
		std::stringstream ss;
		ss<<"Unable to read PLY mesh file '"<<filename<<"'";
		luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
		return;
	}
    
	if(!ply_read_header(plyfile)) {
		std::stringstream ss;
		ss<<"Unable to read PLY header '"<<filename<<"'";
		luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
		return;
	}

	long plyNverts = ply_set_read_cb(plyfile, "vertex", "x", VertexCB, this, 0);
	nverts = (int) plyNverts;
	p = new Point[nverts];
	ply_set_read_cb(plyfile, "vertex", "y", VertexCB, this, 1);
    ply_set_read_cb(plyfile, "vertex", "z", VertexCB, this, 2);

	long plyNtris = ply_set_read_cb(plyfile, "face", "vertex_indices", FaceCB, this, 0);
	ntris = (int) plyNtris;
	vertexIndex = new int[3 * ntris];

	uvs = NULL;
	n = NULL;
	s = NULL;

	if(!ply_read(plyfile)) {
		std::stringstream ss;
		ss<<"Unable to parse PLY file '"<<filename<<"'";
		luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
		nverts = ntris = 0;
		return;
	}

    ply_close(plyfile);

	// Transform mesh vertices to world space
	for (int i  = 0; i < nverts; ++i)
		p[i] = ObjectToWorld(p[i]);

	if(!smooth) return;


	// NOTE - radiance - normal generation unfinished, does not work correctly yet !


	// generate face normals if 'smooth' = true
	n = new Normal[nverts];
	int *nf = new int[nverts];
	for(int i=0; i < nverts; i++) {
		n[i] = Normal(0., 0., 0.);
		nf[i] = 0;
	}

	for(int tri=0; tri < ntris; tri++) {
		/* Compute edge vectors */
		float x10 = p[vertexIndex[(3*tri)+1]].x - p[vertexIndex[(3*tri)]].x;
		float y10 = p[vertexIndex[(3*tri)+1]].y - p[vertexIndex[(3*tri)]].y;
		float z10 = p[vertexIndex[(3*tri)+1]].z - p[vertexIndex[(3*tri)]].z;
		float x12 = p[vertexIndex[(3*tri)+1]].x - p[vertexIndex[(3*tri)+2]].x;
		float y12 = p[vertexIndex[(3*tri)+1]].y - p[vertexIndex[(3*tri)+2]].y;
		float z12 = p[vertexIndex[(3*tri)+1]].z - p[vertexIndex[(3*tri)+2]].z;

		/* Compute the cross product */
		float cpx = (z10 * y12) - (y10 * z12);
		float cpy = (x10 * z12) - (z10 * x12);
		float cpz = (y10 * x12) - (x10 * y12);

		/* Normalize the result to get the unit-length facet normal */
		float r = sqrtf(cpx * cpx + cpy * cpy + cpz * cpz);
		float nx = cpx / r;
		float ny = cpy / r;
		float nz = cpz / r;
		Normal fn = Normal(nx, ny, nz);

		// add to face normal of triangle's vertex normals
		n[vertexIndex[3*tri]] += fn;
		n[vertexIndex[(3*tri)+1]] += fn;
		n[vertexIndex[(3*tri)+2]] += fn;

		// increment contributions
		nf[vertexIndex[3*tri]]++;
		nf[vertexIndex[(3*tri)+1]]++;
		nf[vertexIndex[(3*tri)+2]]++;
	}

	// divide by contributions
	for(int i=0; i < nverts; i++) {
		n[i] /= nf[i];
	}

	delete[] nf;

	printf("Done.\n");
}
PlyMesh::~PlyMesh() {
	delete[] vertexIndex;
	delete[] p;
	delete[] s;
	delete[] n;
	delete[] uvs;
}
BBox PlyMesh::ObjectBound() const {
	BBox bobj;
	for (int i = 0; i < nverts; i++)
		bobj = Union(bobj, WorldToObject(p[i]));
	return bobj;
}
BBox PlyMesh::WorldBound() const {
	BBox worldBounds;
	for (int i = 0; i < nverts; i++)
		worldBounds = Union(worldBounds, p[i]);
	return worldBounds;
}

void
PlyMesh::Refine(vector<ShapePtr > &refined)
const {
	for (int i = 0; i < ntris; ++i) {
		ShapePtr o (new Triangle(ObjectToWorld,
		                               reverseOrientation,
                                       (TriangleMesh *)this,
									   i));
		refined.push_back(o);
	}
}

Shape* PlyMesh::CreateShape(const Transform &o2w,
		bool reverseOrientation, const ParamSet &params) {
	string filename = params.FindOneString("filename", "none");
	bool smooth = params.FindOneBool("smooth", false);
	return new PlyMesh(o2w, reverseOrientation, filename, smooth);
}
