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

#include "shape.h"
#include "paramset.h"
#include "./cyhair/cyHairFile.h"

#include "luxrays/luxrays.h"

namespace lux
{

class HairFile : public Shape {
public:
	HairFile(const Transform &o2w, bool ro, const string &name, const Point *cameraPos,
		boost::shared_ptr<cyHairFile> &hairFile);
	virtual ~HairFile();

	virtual BBox ObjectBound() const;
	virtual bool CanIntersect() const { return false; }
	virtual bool CanSample() const { return false; }

	virtual void Refine(vector<boost::shared_ptr<Shape> > &refined) const;

	virtual void Tessellate(vector<luxrays::TriangleMesh *> *meshList,
		vector<const Primitive *> *primitiveList) const;
	virtual void ExtTessellate(vector<luxrays::ExtTriangleMesh *> *meshList,
		vector<const Primitive *> *primitiveList) const;

	static Shape *CreateShape(const Transform &o2w, bool reverseOrientation,
		const ParamSet &params);

protected:
	void TessellateRibbon(const vector<Point> &hairPoints, const vector<float> &hairSize,
		vector<Point> &meshVerts, vector<Normal> &meshNorms,
		vector<int> &meshTris, vector<float> &meshUVs) const;

	bool hasCameraPosition;
	Point cameraPosition;

	boost::shared_ptr<cyHairFile> hairFile;

	// I need to keep alive refined Shapes for Tessellate() and ExtTessellate() methods
	mutable vector<boost::shared_ptr<Shape> > refinedHairs;
};

}//namespace lux

