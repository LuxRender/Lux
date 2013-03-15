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
		boost::shared_ptr<cyHairFile> &hair) : Shape(o2w, ro, name) {
	hasCameraPosition = (cameraPos != NULL);
	if (hasCameraPosition) {
		// Transform the camera position in local coordinate
		cameraPosition = Inverse(ObjectToWorld) * (*cameraPos);
	}

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

	const float *points = hairFile->GetPointsArray();
	const float *thickness = hairFile->GetThicknessArray();
	const u_short *segments = hairFile->GetSegmentsArray();

	if (segments || (header.d_segments > 0)) {
		u_int pointIndex = 0;

		vector<Point> hairPoints;
		vector<Point> meshVerts;
		vector<int> meshTris;
		vector<float> meshUVs;
		for (u_int i = 0; i < header.hair_count; ++i) {
			// segmentSize must be a signed 
			const int segmentSize = segments ? segments[i] : header.d_segments;
			if (segmentSize == 0)
				continue;

			// Collect the segment points
			hairPoints.clear();
			for (int j = 0; j < segmentSize; ++j) {
				hairPoints.push_back(Point(points[pointIndex], points[pointIndex + 1], points[pointIndex + 2]));
				pointIndex += 3;
			}

			// Create the mesh vertices
			meshVerts.clear();
			meshUVs.clear();
			for (int j = 0; j < segmentSize - 1; ++j) {
				const Vector z = Normalize(hairPoints[j + 1] - hairPoints[j]);
				Vector x, y;
				// Check if I have to face the ribbon in a specific direction
				if (hasCameraPosition) {
					y = Normalize(cameraPosition - hairPoints[j]);

					if (AbsDot(z, y) < 1.f - 0.05f) {
						x = Normalize(Cross(z, y));
						y = Normalize(Cross(x, z));
					} else
						CoordinateSystem(z, &x, &y);
				} else
					CoordinateSystem(z, &x, &y);
				const float radius = ((thickness) ? thickness[i] : header.d_thickness) * .5f;

				const Point p0 = hairPoints[j] + radius * x;
				const Point p1 = hairPoints[j] - radius * x;
				meshVerts.push_back(p0);
				meshVerts.push_back(p1);

				const float v = j / (float)hairPoints.size();
				meshUVs.push_back(1.f);
				meshUVs.push_back(v);
				meshUVs.push_back(-1.f);
				meshUVs.push_back(v);
			}
			// Add the cap vertex
			meshVerts.push_back(hairPoints.back());
			meshUVs.push_back(0.f);
			meshUVs.push_back(1.f);

			// Triangulate the vertex mesh
			meshTris.clear();
			for (int j = 0; j < segmentSize - 2; ++j) {
				const u_int index = j * 2;

				// First triangle
				meshTris.push_back(index);
				meshTris.push_back(index + 1);
				meshTris.push_back(index + 2);

				// Second triangle
				meshTris.push_back(index + 1);
				meshTris.push_back(index + 3);
				meshTris.push_back(index + 2);
			}
			// Add the cap
			meshTris.push_back(meshVerts.size() - 3);
			meshTris.push_back(meshVerts.size() - 2);
			meshTris.push_back(meshVerts.size() - 1);

			// Create the mesh Shape
			ParamSet paramSet;
			paramSet.AddInt("indices", &meshTris[0], meshTris.size());
			paramSet.AddFloat("uv", &meshUVs[0], meshUVs.size());
			paramSet.AddPoint("P", &meshVerts[0], meshVerts.size());

			boost::shared_ptr<Shape> shape = MakeShape("trianglemesh",
					ObjectToWorld, reverseOrientation, paramSet);

			refined.reserve(refined.size() + meshTris.size() / 3);	
			refined.push_back(shape);
			refinedHairs.reserve(meshTris.size() / 3);
			refinedHairs.push_back(shape);
		}
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
}

void HairFile::Tesselate(vector<luxrays::TriangleMesh *> *meshList,
		vector<const Primitive *> *primitiveList) const {
	// Refine the primitive
	vector<boost::shared_ptr<Shape> > refined;
	Refine(refined);

	// Tesselate all generated primitives
	for (u_int i = 0; i < refined.size(); ++i)
		refined[i]->Tesselate(meshList, primitiveList);
}

void HairFile::ExtTesselate(vector<luxrays::ExtTriangleMesh *> *meshList,
		vector<const Primitive *> *primitiveList) const {
	// Refine the primitive
	vector<boost::shared_ptr<Shape> > refined;
	Refine(refined);

	// Tesselate all generated primitives
	for (u_int i = 0; i < refined.size(); ++i)
		refined[i]->ExtTesselate(meshList, primitiveList);
}

Shape *HairFile::CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params) {
	string name = params.FindOneString("name", "'hairfile'");
	const string filename = AdjustFilename(params.FindOneString("filename", "none"));
	u_int nItems;
	const Point *cameraPos = params.FindPoint("camerapos", &nItems);

	boost::shared_ptr<cyHairFile> hairFile(new cyHairFile());
	int hairCount = hairFile->LoadFromFile(filename.c_str());
	if (hairCount <= 0) {
		SHAPE_LOG("hairfile", LUX_ERROR, LUX_SYSTEM) << "Unable to read hair file '" << filename << "'";
		return NULL;
	}

	return new HairFile(o2w, reverseOrientation, name, cameraPos, hairFile);
}

static DynamicLoader::RegisterShape<HairFile> r("hairfile");
