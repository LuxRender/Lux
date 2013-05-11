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

#include "deferred.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

DeferredLoadShape::DeferredLoadShape(const Transform &o2w, bool ro, const string &nm,
		const BBox &bbox, const ParamSet &ps) : Shape(o2w, ro, nm) {
	shapeBBox = bbox;

	// Make a copy of the parameters
	params = new ParamSet();
	params->Add(ps);
}

DeferredLoadShape::~DeferredLoadShape() {
	delete params;
}

void DeferredLoadShape::LoadShape() const {
	// Check if I have yet to load the shape
	if (params) {
		boost::mutex::scoped_lock(loadMutex);

		// Just in case some other thread was faster than me
		if (params) {
			const string objName = params->FindOneString("shapename", "'deferred'");
			LOG(LUX_DEBUG, LUX_NOERROR) << "Loading deferred object: " << objName;

			// Remove DeferredLoadShape specific parameters
			params->EraseFloat("shapename");
			params->EraseFloat("shapebbox");

			shape = MakeShape(objName, ObjectToWorld, reverseOrientation, *params);

			shape->SetMaterial(material);
			shape->SetExterior(exterior);
			shape->SetInterior(interior);

			// Check if I have to refine the shape
			if (!shape->CanIntersect()) {
				vector<boost::shared_ptr<Primitive> > refined;
				shape->Refine(refined, PrimitiveRefinementHints(false), shape);
				accelerator = MakeAccelerator("qbvh", refined, ParamSet());
				prim = accelerator.get();
			} else
				prim = shape.get();

			delete params;
			params = NULL;
		}
	}
}

Shape *DeferredLoadShape::CreateShape(const Transform &o2w, bool reverseOrientation,
		const ParamSet &params) {
	string name = params.FindOneString("name", "'deferredload'");

	// Read the bounding box
	u_int count;
	const float *bboxData = params.FindFloat("shapebbox", &count);
	if (count != 6)
		throw new std::runtime_error("Wrong number of components in a DeferredLoadShape bounding box: " + boost::lexical_cast<string>(count));
	BBox bbox(Point(bboxData[0], bboxData[1], bboxData[2]),
			Point(bboxData[3], bboxData[4], bboxData[5]));

	// Mark all params as used to avoid annoying warnings (the parsing of
	// the shape is deferred too)
	params.MarkAllUsed();

	return new DeferredLoadShape(o2w, reverseOrientation, name, bbox, params);
}

static DynamicLoader::RegisterShape<DeferredLoadShape> r("deferred");
