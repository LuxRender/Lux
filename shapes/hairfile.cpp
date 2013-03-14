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

#include "hairfile.h"
#include "sphere.h"
#include "dynload.h"

using namespace lux;

HairFile::HairFile(const Transform &o2w, bool ro, const string &name,
		boost::shared_ptr<cyHairFile> &hair) : Shape(o2w, ro, name) {
	hairFile = hair;
}

HairFile::~HairFile() {
}

BBox HairFile::ObjectBound() const {
	BBox objectBound;
	
	const cyHairFileHeader &header = hairFile->GetHeader();
	const float *points = hairFile->GetPointsArray();
	const float *thickness = hairFile->GetThicknessArray();

	for (u_int i = 0; i < header.hair_count; ++i) {
		const unsigned int index = i * 3;
		const Point p(points[index], points[index + 1], points[index + 2]);
		BBox pointBBox(p);

		const float size = (thickness) ? thickness[i] : header.d_thickness;
		pointBBox.Expand(size);

		objectBound = Union(objectBound, pointBBox);
	}

	return objectBound;
}

void HairFile::Refine(vector<boost::shared_ptr<Shape> > &refined) const {
	const cyHairFileHeader &header = hairFile->GetHeader();

	if (header.hair_count == 0)
		return;

	const float *points = hairFile->GetPointsArray();
	const float *thickness = hairFile->GetThicknessArray();
	for (u_int i = 0; i < header.hair_count; ++i) {
		const unsigned int index = i * 3;
		const Vector vert(points[index], points[index + 1], points[index + 2]);
		const float radius = ((thickness) ? thickness[i] : header.d_thickness);

		boost::shared_ptr<Shape> shape(new Sphere(ObjectToWorld * Translate(vert), reverseOrientation, name, radius, -radius, radius, 360.f));
		refined.push_back(shape);
	}
}

Shape *HairFile::CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params) {
	string name = params.FindOneString("name", "'hairfile'");
	const string filename = AdjustFilename(params.FindOneString("filename", "none"));

	boost::shared_ptr<cyHairFile> hairFile(new cyHairFile());
	int hairCount = hairFile->LoadFromFile(filename.c_str());
	if (hairCount <= 0) {
		SHAPE_LOG("hairfile", LUX_ERROR, LUX_SYSTEM) << "Unable to read hair file '" << filename << "'";
		return NULL;
	}

	return new HairFile(o2w, reverseOrientation, name, hairFile);
}

static DynamicLoader::RegisterShape<HairFile> r("hairfile");
