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

#include "plymesh.h"
#include "paramset.h"
#include "dynload.h"

#include "mesh.h"
#include "./plymesh/rply.h"

namespace lux
{

// rply vertex callback
static int VertexCB(p_ply_argument argument)
{
	long userIndex = 0;
	void *userData = NULL;
	ply_get_argument_user_data(argument, &userData, &userIndex);

	Point* p = *static_cast<Point **>(userData);

	long vertIndex;
	ply_get_argument_element(argument, NULL, &vertIndex);

	if (userIndex == 0)
		p[vertIndex].x =
			static_cast<float>(ply_get_argument_value(argument));
	else if (userIndex == 1)
		p[vertIndex].y =
			static_cast<float>(ply_get_argument_value(argument));
	else if (userIndex == 2)
		p[vertIndex].z =
			static_cast<float>(ply_get_argument_value(argument));
/*	else
		return 0;*/

	return 1;
}

// rply normal callback
static int NormalCB(p_ply_argument argument)
{
	long userIndex = 0;
	void *userData = NULL;
	ply_get_argument_user_data(argument, &userData, &userIndex);

	Normal* n = *static_cast<Normal **>(userData);

	long vertIndex;
	ply_get_argument_element(argument, NULL, &vertIndex);

	if (userIndex == 0)
		n[vertIndex].x =
			static_cast<float>(ply_get_argument_value(argument));
	else if (userIndex == 1)
		n[vertIndex].y =
			static_cast<float>(ply_get_argument_value(argument));
	else if (userIndex == 2)
		n[vertIndex].z =
			static_cast<float>(ply_get_argument_value(argument));
/*	else
		return 0;*/

	return 1;
}

// rply face callback
static int FaceCB(p_ply_argument argument)
{
	void *userData = NULL;
	ply_get_argument_user_data(argument, &userData, NULL);

	int *verts = *static_cast<int **>(userData);

	long triIndex;
	ply_get_argument_element(argument, NULL, &triIndex);

	long length, valueIndex;
	ply_get_argument_property(argument, NULL, &length, &valueIndex);

	if (valueIndex >= 0 && valueIndex < 3) {
		verts[triIndex * 3 + valueIndex] =
			static_cast<int>(ply_get_argument_value(argument));
	}/* else
		return 0;*/

	return 1;
}

Shape* PlyMesh::CreateShape(const Transform &o2w,
		bool reverseOrientation, const ParamSet &params) {
	string filename = params.FindOneString("filename", "none");
	bool smooth = params.FindOneBool("smooth", false);

	std::stringstream ss;
	ss << "Loading PLY mesh file: '" << filename << "'...";
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	p_ply plyfile = ply_open(filename.c_str(), NULL);
	if (!plyfile) {
		ss.str("");
		ss << "Unable to read PLY mesh file '" << filename << "'";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		return NULL;
	}

	if (!ply_read_header(plyfile)) {
		ss.str("");
		ss << "Unable to read PLY header from '" << filename << "'";
		luxError(LUX_BADFILE, LUX_ERROR, ss.str().c_str());
		return NULL;
	}

	Point *p;
	long plyNbVerts = ply_set_read_cb(plyfile, "vertex", "x",
		VertexCB, &p, 0);
	ply_set_read_cb(plyfile, "vertex", "y", VertexCB, &p, 1);
	ply_set_read_cb(plyfile, "vertex", "z", VertexCB, &p, 2);
	if (plyNbVerts <= 0) {
		ss.str("");
		ss << "No vertices found in '" << filename << "'";
		luxError(LUX_BADFILE, LUX_ERROR, ss.str().c_str());
		return NULL;
	}

	int *vertexIndex;
	long plyNbTris = ply_set_read_cb(plyfile, "face", "vertex_indices",
		FaceCB, &vertexIndex, 0);
	if (plyNbTris <= 0) {
		ss.str("");
		ss << "No triangles found in '" << filename << "'";
		luxError(LUX_BADFILE, LUX_ERROR, ss.str().c_str());
		return NULL;
	}

	Normal *n;
	long plyNbNormals = ply_set_read_cb(plyfile, "vertex", "nx",
		NormalCB, &n, 0);
	ply_set_read_cb(plyfile, "vertex", "ny", NormalCB, &n, 1);
	ply_set_read_cb(plyfile, "vertex", "nz", NormalCB, &n, 2);

	p = new Point[plyNbVerts];
	vertexIndex = new int[3 * plyNbTris];
	if (plyNbNormals <= 0)
		n = NULL;
	else
		n = new Normal[plyNbNormals];

	if (!ply_read(plyfile)) {
		ss.str("");
		ss << "Unable to parse PLY file '" << filename << "'";
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		delete[] p;
		delete[] vertexIndex;
		delete[] n;
		return NULL;
	}

	ply_close(plyfile);

	if (smooth || plyNbVerts != plyNbNormals) {
		if (n) {
			ss.str("");
			ss << "Overriding plymesh normals";
			luxError(LUX_NOERROR, LUX_WARNING, ss.str().c_str());
			delete[] n;
		}
		// generate face normals
		n = new Normal[plyNbVerts];
		int *nf = new int[plyNbVerts];
		for (int i = 0; i < plyNbVerts; ++i) {
			n[i] = Normal(0.f, 0.f, 0.f);
			nf[i] = 0.f;
		}

		for (int tri = 0; tri < plyNbTris; ++tri) {
			/* Compute edge vectors */
			const Vector e10(p[vertexIndex[3 * tri + 1]] -
				p[vertexIndex[3 * tri]]);
			const Vector e12(p[vertexIndex[3 * tri + 1]] -
				p[vertexIndex[3 * tri + 2]]);

			Normal fn(Normalize(Cross(e12, e10)));

			// add to face normal of triangle's vertex normals
			n[vertexIndex[3 * tri]] += fn;
			n[vertexIndex[3 * tri + 1]] += fn;
			n[vertexIndex[3 * tri + 2]] += fn;

			// increment contributions
			nf[vertexIndex[3 * tri]]++;
			nf[vertexIndex[3 * tri + 1]]++;
			nf[vertexIndex[3 * tri + 2]]++;
		}

		// divide by contributions
		for (int i = 0; i < plyNbVerts; ++i)
			n[i] /= nf[i];

		delete[] nf;
	}

	boost::shared_ptr<Texture<float> > dummytex;
	Mesh *mesh = new Mesh(o2w, reverseOrientation, Mesh::ACCEL_AUTO,
		plyNbVerts, p, n, NULL, Mesh::TRI_AUTO, plyNbTris, vertexIndex,
		Mesh::QUAD_QUADRILATERAL, 0, NULL, Mesh::SUBDIV_LOOP, 0,
		dummytex, 1.f, 0.f, false, false);
	delete[] p;
	delete[] n;
	delete[] vertexIndex;
	return mesh;
}

static DynamicLoader::RegisterShape<PlyMesh> r("plymesh");

}//namespace lux

