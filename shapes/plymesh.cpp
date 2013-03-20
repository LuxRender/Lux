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

#include "plymesh.h"
#include "paramset.h"
#include "context.h"
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

// rply st/uv callback
static int TexCoordCB(p_ply_argument argument)
{
	long userIndex = 0;
	void *userData = NULL;
	ply_get_argument_user_data(argument, &userData, &userIndex);

	float* uv = *static_cast<float **>(userData);

	long vertIndex;
	ply_get_argument_element(argument, NULL, &vertIndex);

	if (userIndex == 0)
		uv[2*vertIndex] =
			static_cast<float>(ply_get_argument_value(argument));
	else if (userIndex == 1)
		uv[2*vertIndex+1] =
			static_cast<float>(ply_get_argument_value(argument));
/*	else
		return 0;*/

	return 1;
}

// rply color callback
static int ColorCB(p_ply_argument argument) {
	long userIndex = 0;
	void *userData = NULL;
	ply_get_argument_user_data(argument, &userData, &userIndex);

	float *c = *static_cast<float **> (userData);

	long colIndex;
	ply_get_argument_element(argument, NULL, &colIndex);

	if (userIndex == 0)
		c[colIndex * 3] =
			static_cast<float>(ply_get_argument_value(argument) / 255.0);
	else if (userIndex == 1)
		c[colIndex * 3 + 1] =
			static_cast<float>(ply_get_argument_value(argument) / 255.0);
	else if (userIndex == 2)
		c[colIndex * 3 + 2] =
			static_cast<float>(ply_get_argument_value(argument) / 255.0);

	return 1;
}

// rply vertex callback
static int AlphaCB(p_ply_argument argument) {
	long userIndex = 0;
	void *userData = NULL;
	ply_get_argument_user_data(argument, &userData, &userIndex);

	float *c = *static_cast<float **> (userData);

	long alphaIndex;
	ply_get_argument_element(argument, NULL, &alphaIndex);

	if (userIndex == 0)
		c[alphaIndex] =
			static_cast<float>(ply_get_argument_value(argument) / 255.0);

	return 1;
}

class FaceData {
public:
	FaceData() : triVerts(), quadVerts() { }

	std::vector<int> triVerts;
	std::vector<int> quadVerts;
};

// rply face callback
static int FaceCB(p_ply_argument argument)
{
	void *userData = NULL;
	ply_get_argument_user_data(argument, &userData, NULL);

	FaceData *fd = static_cast<FaceData *>(userData);

	long faceIndex;
	ply_get_argument_element(argument, NULL, &faceIndex);

	long length, valueIndex;
	ply_get_argument_property(argument, NULL, &length, &valueIndex);	

	const int nTris = fd->triVerts.size();

	switch (length) {
		case 3:
			if (valueIndex < 0)
				// preallocate items
				fd->triVerts.resize(nTris + 3);
			else if (valueIndex < 3)
				fd->triVerts[nTris-3+valueIndex] = static_cast<int>(ply_get_argument_value(argument));
			break;
		case 4:
			if (valueIndex < 0)
				// preallocate items
				fd->quadVerts.resize(fd->quadVerts.size() + 4);
			else if (valueIndex < 4)
				fd->quadVerts[fd->quadVerts.size()-4+valueIndex] = static_cast<int>(ply_get_argument_value(argument));
			break;
	}

	return 1;
}

static void ErrorCB(const char *message)
{
	LOG(LUX_ERROR, LUX_SYSTEM) << "PLY loader error: " << message;
}

Shape* PlyMesh::CreateShape(const Transform &o2w,
		bool reverseOrientation, const ParamSet &params) {
	string name = params.FindOneString("name", "'plymesh'");
	const string filename = AdjustFilename(params.FindOneString("filename", "none"));
	bool smooth = params.FindOneBool("smooth", false);

	SHAPE_LOG(name, LUX_INFO,LUX_NOERROR) << "Loading PLY mesh file: '" << filename << "'...";

	p_ply plyfile = ply_open(filename.c_str(), ErrorCB);
	if (!plyfile) {
		SHAPE_LOG(name, LUX_ERROR,LUX_SYSTEM) << "Unable to read PLY mesh file '" << filename << "'";
		return NULL;
	}

	if (!ply_read_header(plyfile)) {
		SHAPE_LOG(name, LUX_ERROR,LUX_BADFILE) << "Unable to read PLY header from '" << filename << "'";
		return NULL;
	}

	Point *p;
	long plyNbVerts = ply_set_read_cb(plyfile, "vertex", "x",
		VertexCB, &p, 0);
	ply_set_read_cb(plyfile, "vertex", "y", VertexCB, &p, 1);
	ply_set_read_cb(plyfile, "vertex", "z", VertexCB, &p, 2);
	if (plyNbVerts <= 0) {
		SHAPE_LOG(name, LUX_ERROR,LUX_BADFILE) << "No vertices found in '" << filename << "'";
		return NULL;
	}

	FaceData faceData;
	long plyNbFaces = ply_set_read_cb(plyfile, "face", "vertex_indices",
		FaceCB, &faceData, 0);
	if (plyNbFaces <= 0) {
		SHAPE_LOG(name, LUX_ERROR,LUX_BADFILE) << "No faces found in '" << filename << "'";
		return NULL;
	}

	Normal *n;
	long plyNbNormals = ply_set_read_cb(plyfile, "vertex", "nx",
		NormalCB, &n, 0);
	ply_set_read_cb(plyfile, "vertex", "ny", NormalCB, &n, 1);
	ply_set_read_cb(plyfile, "vertex", "nz", NormalCB, &n, 2);

	// try both st and uv for texture coordinates
	// st before uv
	float *uv;
	long plyNbUVs = ply_set_read_cb(plyfile, "vertex", "s",
		TexCoordCB, &uv, 0);
	ply_set_read_cb(plyfile, "vertex", "t", TexCoordCB, &uv, 1);

	if (plyNbUVs <= 0) {
		plyNbUVs = ply_set_read_cb(plyfile, "vertex", "u",
			TexCoordCB, &uv, 0);
		ply_set_read_cb(plyfile, "vertex", "v", TexCoordCB, &uv, 1);
	}

	// Check if the file includes color informations
	float *cols;
	long plyNbColors = ply_set_read_cb(plyfile, "vertex", "red", ColorCB, &cols, 0);
	ply_set_read_cb(plyfile, "vertex", "green", ColorCB, &cols, 1);
	ply_set_read_cb(plyfile, "vertex", "blue", ColorCB, &cols, 2);

	// Check if the file includes alpha informations
	float *alphas;
	long plyNbAlphas = ply_set_read_cb(plyfile, "vertex", "alpha", AlphaCB, &cols, 0);

	p = new Point[plyNbVerts];
	if (plyNbNormals <= 0)
		n = NULL;
	else
		n = new Normal[plyNbNormals];

	if (plyNbUVs <= 0)
		uv = NULL;
	else
		uv = new float[2*plyNbUVs];

	if (plyNbColors == 0)
		cols = NULL;
	else
		cols = new float[3 * plyNbVerts];

	if (plyNbAlphas == 0)
		alphas = NULL;
	else
		alphas = new float[plyNbVerts];

	if (!ply_read(plyfile)) {
		SHAPE_LOG(name, LUX_ERROR,LUX_SYSTEM) << "Unable to parse PLY file '" << filename << "'";
		delete[] p;
		delete[] n;
		delete[] uv;
		delete[] cols;
		delete[] alphas;
		return NULL;
	}

	ply_close(plyfile);

	int plyNbTris = faceData.triVerts.size()/3;
	int plyNbQuads = faceData.quadVerts.size()/4;

	if (smooth || plyNbVerts != plyNbNormals) {
		if (n) {
			SHAPE_LOG(name, (smooth ? LUX_DEBUG : LUX_WARNING), LUX_NOERROR) << "Overriding plymesh normals";
			delete[] n;
		}
		// generate face normals
		n = new Normal[plyNbVerts];
		int *nf = new int[plyNbVerts];
		for (int i = 0; i < plyNbVerts; ++i) {
			n[i] = Normal(0.f, 0.f, 0.f);
			nf[i] = 0.f;
		}

		for (int face = 0; face < plyNbTris; ++face) {
			int i = 3 * face;
			// Compute edge vectors
			const Vector e10(p[faceData.triVerts[i + 1]] -
				p[faceData.triVerts[i]]);
			const Vector e12(p[faceData.triVerts[i + 1]] -
				p[faceData.triVerts[i + 2]]);

			Normal fn(Normalize(Cross(e12, e10)));

			n[faceData.triVerts[i + 0]] += fn;
			n[faceData.triVerts[i + 1]] += fn;
			n[faceData.triVerts[i + 2]] += fn;
			nf[faceData.triVerts[i + 0]]++;
			nf[faceData.triVerts[i + 1]]++;
			nf[faceData.triVerts[i + 2]]++;
		}

		// compute normals of both triangles in each quad separately
		// this should be more consistent if quads and tris share vertex
		// and/or if the quad is non-planar
		for (int face = 0; face < plyNbQuads; ++face) {
			int i = 4 * face;
			// Compute edge vectors
			const Vector e10(p[faceData.quadVerts[i + 1]] -
				p[faceData.quadVerts[i]]);
			const Vector e12(p[faceData.quadVerts[i + 1]] -
				p[faceData.quadVerts[i + 2]]);

			Normal fn1(Normalize(Cross(e12, e10)));
			
			n[faceData.quadVerts[i + 0]] += fn1;
			n[faceData.quadVerts[i + 1]] += fn1;
			n[faceData.quadVerts[i + 2]] += fn1;
			nf[faceData.quadVerts[i + 0]]++;
			nf[faceData.quadVerts[i + 1]]++;
			nf[faceData.quadVerts[i + 2]]++;

			// Compute edge vectors for second tri
			const Vector e30(p[faceData.quadVerts[i + 3]] -
				p[faceData.quadVerts[i]]);
			const Vector e32(p[faceData.quadVerts[i + 3]] -
				p[faceData.quadVerts[i + 2]]);

			Normal fn2(Normalize(Cross(e30, e32)));

			n[faceData.quadVerts[i + 0]] += fn2;
			n[faceData.quadVerts[i + 2]] += fn2;
			n[faceData.quadVerts[i + 3]] += fn2;
			nf[faceData.quadVerts[i + 0]]++;
			nf[faceData.quadVerts[i + 2]]++;
			nf[faceData.quadVerts[i + 3]]++;
		}

		// divide by contributions
		for (int i = 0; i < plyNbVerts; ++i)
			n[i] /= nf[i];

		delete[] nf;
	}

	if (uv && (plyNbVerts != plyNbUVs)) {
		SHAPE_LOG(name, LUX_ERROR, LUX_CONSISTENCY)<< "Incorrect number of uv coordinates";
		delete[] uv;
		uv = NULL;
	}

	if (cols && (plyNbVerts != plyNbColors)) {
		SHAPE_LOG(name, LUX_ERROR, LUX_CONSISTENCY)<< "Incorrect number of color values";
		delete[] cols;
		cols = NULL;
	}

	if (alphas && (plyNbVerts != plyNbAlphas)) {
		SHAPE_LOG(name, LUX_ERROR, LUX_CONSISTENCY)<< "Incorrect number of alpha values";
		delete[] cols;
		cols = NULL;
	}

	const int *triVerts = plyNbTris > 0 ? &faceData.triVerts[0] : NULL;
	const int *quadVerts = plyNbQuads > 0 ? &faceData.quadVerts[0] : NULL;

	// subdiv and displacement params
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

	string subdivscheme = params.FindOneString("subdivscheme", "loop");
	int nsubdivlevels = params.FindOneInt("nsubdivlevels", 0);

	Mesh::MeshSubdivType subdivType;
	if (subdivscheme == "loop")
		subdivType = Mesh::SUBDIV_LOOP;
	else if (subdivscheme == "microdisplacement")
		subdivType = Mesh::SUBDIV_MICRODISPLACEMENT;
	else {
		SHAPE_LOG(name, LUX_WARNING,LUX_BADTOKEN) << "Subdivision type  '" << subdivscheme << "' unknown. Using \"loop\".";
		subdivType = Mesh::SUBDIV_LOOP;
	}

	bool genTangents = params.FindOneBool("generatetangents", false);

	boost::shared_ptr<Texture<float> > dummytex;
	Mesh *mesh = new Mesh(o2w, reverseOrientation, name, Mesh::ACCEL_AUTO,
		plyNbVerts, p, n, uv, cols, alphas, Mesh::TRI_AUTO, plyNbTris, triVerts,
		Mesh::QUAD_QUADRILATERAL, plyNbQuads, quadVerts, subdivType,
		nsubdivlevels, displacementMap, displacementMapScale,
		displacementMapOffset, displacementMapNormalSmooth,
		displacementMapSharpBoundary, normalSplit, genTangents);
	delete[] p;
	delete[] n;
	delete[] uv;
	delete[] cols;
	delete[] alphas;
	return mesh;
}

static DynamicLoader::RegisterShape<PlyMesh> r("plymesh");

}//namespace lux

