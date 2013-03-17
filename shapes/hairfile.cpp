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

#include <boost/foreach.hpp>

#include "hairfile.h"
#include "sphere.h"
#include "dynload.h"

using namespace lux;

HairFile::HairFile(const Transform &o2w, bool ro, const string &name, const Point *cameraPos,
		const string &aType, boost::shared_ptr<cyHairFile> &hair) : Shape(o2w, ro, name) {
	hasCameraPosition = (cameraPos != NULL);
	if (hasCameraPosition) {
		// Transform the camera position in local coordinate
		cameraPosition = Inverse(ObjectToWorld) * (*cameraPos);
	}

	accelType = aType;
	hairFile = hair;
}

HairFile::~HairFile() {
}

BBox HairFile::ObjectBound() const {
	BBox objectBound;
	
	const cyHairFileHeader &header = hairFile->GetHeader();
	const float *points = hairFile->GetPointsArray();
	const float *thickness = hairFile->GetThicknessArray();

	for (u_int i = 0; i < header.point_count; ++i) {
		const unsigned int index = i * 3;
		const Point p(points[index], points[index + 1], points[index + 2]);
		BBox pointBBox(p);

		const float radius = ((thickness) ? thickness[i] : header.d_thickness) * .5f;
		pointBBox.Expand(radius);

		objectBound = Union(objectBound, pointBBox);
	}

	return objectBound;
}

void HairFile::TessellateRibbon(const vector<Point> &hairPoints, const vector<float> &hairSize,
		vector<Point> &meshVerts, vector<Normal> &meshNorms,
		vector<int> &meshTris, vector<float> &meshUVs) const {
	// Create the mesh vertices
	const u_int baseOffset = meshVerts.size();
	for (int i = 0; i < (int)hairPoints.size(); ++i) {
		Vector z;
		// I need a special case for the very last point
		if (i == (int)hairPoints.size() - 1)
			z = Normalize(hairPoints[i] - hairPoints[i - 1]);
		else
			z = Normalize(hairPoints[i + 1] - hairPoints[i]);

		Vector x, y;
		// Check if I have to face the ribbon in a specific direction
		if (hasCameraPosition) {
			y = Normalize(cameraPosition - hairPoints[i]);

			if (AbsDot(z, y) < 1.f - 0.05f) {
				x = Normalize(Cross(z, y));
				y = Normalize(Cross(x, z));
			} else
				CoordinateSystem(z, &x, &y);
		} else
			CoordinateSystem(z, &x, &y);

		const Point p0 = hairPoints[i] + hairSize[i] * x;
		const Point p1 = hairPoints[i] - hairSize[i] * x;
		meshVerts.push_back(p0);
		meshNorms.push_back(Normal());
		meshVerts.push_back(p1);
		meshNorms.push_back(Normal());

		const float v = i / (float)hairPoints.size();
		meshUVs.push_back(1.f);
		meshUVs.push_back(v);
		meshUVs.push_back(-1.f);
		meshUVs.push_back(v);
	}

	// Triangulate the vertex mesh
	for (int i = 0; i < (int)hairPoints.size() - 1; ++i) {
		const u_int index = baseOffset + i * 2;

		const u_int i0 = index;
		const u_int i1 = index + 1;
		const u_int i2 = index + 2;
		const u_int i3 = index + 3;

		// First triangle
		meshTris.push_back(i0);
		meshTris.push_back(i1);
		meshTris.push_back(i2);
		// First triangle normal
		const Normal n0 = Normal(Cross(meshVerts[i1] - meshVerts[i0], meshVerts[i2] - meshVerts[i0]));
		meshNorms[i0] += n0;
		meshNorms[i1] += n0;
		meshNorms[i2] += n0;

		// Second triangle
		meshTris.push_back(i1);
		meshTris.push_back(i3);
		meshTris.push_back(i2);
		// Second triangle normal
		const Normal n1 = Normal(Cross(meshVerts[i3] - meshVerts[i1], meshVerts[i2] - meshVerts[i1]));
		meshNorms[i1] += n1;
		meshNorms[i2] += n1;
		meshNorms[i3] += n1;
	}
}

void HairFile::Refine(vector<boost::shared_ptr<Shape> > &refined) const {
	const cyHairFileHeader &header = hairFile->GetHeader();
	if (header.hair_count == 0)
		return;

	if (refinedHairs.size() > 0) {
		refined.reserve(refined.size() + refinedHairs.size());
		for (u_int i = 0; i < refinedHairs.size(); ++i)
			refined.push_back(refinedHairs[i]);
		return;
	}

	LOG(LUX_DEBUG, LUX_NOERROR) << "Refining " << header.hair_count << " strands";
	const double start = luxrays::WallClockTime();

	const float *points = hairFile->GetPointsArray();
	const float *thickness = hairFile->GetThicknessArray();
	const u_short *segments = hairFile->GetSegmentsArray();

	if (segments || (header.d_segments > 0)) {
		u_int pointIndex = 0;

		vector<Point> hairPoints;
		vector<float> hairSize;

		vector<Point> meshVerts;
		vector<Normal> meshNorms;
		vector<int> meshTris;
		vector<float> meshUVs;
		for (u_int i = 0; i < header.hair_count; ++i) {
			// segmentSize must be a signed 
			const int segmentSize = segments ? segments[i] : header.d_segments;
			if (segmentSize == 0)
				continue;

			// Collect the segment points and size
			hairPoints.clear();
			for (int j = 0; j < segmentSize; ++j) {
				hairPoints.push_back(Point(points[pointIndex], points[pointIndex + 1], points[pointIndex + 2]));
				hairSize.push_back(((thickness) ? thickness[i] : header.d_thickness) * .5f);

				pointIndex += 3;
			}

			TessellateRibbon(hairPoints, hairSize, meshVerts, meshNorms, meshTris, meshUVs);
		}

		// Normalize normals
		for (u_int i = 0; i < meshNorms.size(); ++i)
			meshNorms[i] = Normalize(meshNorms[i]);
		
		LOG(LUX_DEBUG, LUX_NOERROR) << "Strands mesh: " << meshTris.size() / 3 << " triangles";

		// Create the mesh Shape
		ParamSet paramSet;
		paramSet.AddInt("indices", &meshTris[0], meshTris.size());
		paramSet.AddFloat("uv", &meshUVs[0], meshUVs.size());
		paramSet.AddPoint("P", &meshVerts[0], meshVerts.size());
		paramSet.AddNormal("N", &meshNorms[0], meshNorms.size());
		paramSet.AddString("acceltype", &accelType, 1);

		boost::shared_ptr<Shape> shape = MakeShape("trianglemesh",
				ObjectToWorld, reverseOrientation, paramSet);

		refined.reserve(refined.size() + meshTris.size() / 3);	
		refined.push_back(shape);
		refinedHairs.reserve(meshTris.size() / 3);
		refinedHairs.push_back(shape);
	} else {
		// There are not segments so it must be a particles file. The Shape
		// is refined as a set of spheres.
		for (u_int i = 0; i < header.hair_count; ++i) {
			const unsigned int index = i * 3;
			const Vector vert(points[index], points[index + 1], points[index + 2]);
			const float radius = ((thickness) ? thickness[i] : header.d_thickness) * .5f;

			boost::shared_ptr<Shape> shape(new Sphere(ObjectToWorld * Translate(vert), reverseOrientation, name, radius, -radius, radius, 360.f));
			refined.push_back(shape);
		}
	}

	const float dt = luxrays::WallClockTime() - start;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Refining time: " << std::setprecision(3) << dt << " secs";
}

void HairFile::Tessellate(vector<luxrays::TriangleMesh *> *meshList,
		vector<const Primitive *> *primitiveList) const {
	// Refine the primitive
	vector<boost::shared_ptr<Shape> > refined;
	Refine(refined);

	// Tessellate all generated primitives
	for (u_int i = 0; i < refined.size(); ++i)
		refined[i]->Tessellate(meshList, primitiveList);
}

void HairFile::ExtTessellate(vector<luxrays::ExtTriangleMesh *> *meshList,
		vector<const Primitive *> *primitiveList) const {
	// Refine the primitive
	vector<boost::shared_ptr<Shape> > refined;
	Refine(refined);

	// Tessellate all generated primitives
	for (u_int i = 0; i < refined.size(); ++i)
		refined[i]->ExtTessellate(meshList, primitiveList);
}

Shape *HairFile::CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params) {
	string name = params.FindOneString("name", "'hairfile'");
	const string filename = AdjustFilename(params.FindOneString("filename", "none"));
	u_int nItems;
	const Point *cameraPos = params.FindPoint("camerapos", &nItems);
	const string accelType = params.FindOneString("acceltype", "qbvh");

	boost::shared_ptr<cyHairFile> hairFile(new cyHairFile());
	int hairCount = hairFile->LoadFromFile(filename.c_str());
	if (hairCount <= 0) {
		SHAPE_LOG("hairfile", LUX_ERROR, LUX_SYSTEM) << "Unable to read hair file '" << filename << "'";
		return NULL;
	}

	return new HairFile(o2w, reverseOrientation, name, cameraPos, accelType, hairFile);
}

static DynamicLoader::RegisterShape<HairFile> r("hairfile");
