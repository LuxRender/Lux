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

// shape.cpp*
#include "shape.h"
#include "primitive.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Shape Method Definitions
Shape::Shape(const Transform &o2w, bool ro)
	: ObjectToWorld(o2w), WorldToObject(o2w.GetInverse()),
	reverseOrientation(ro),
	transformSwapsHandedness(o2w.SwapsHandedness()) {
}
Shape::Shape(const Transform &o2w, bool ro, boost::shared_ptr<Material> mat)
	: ObjectToWorld(o2w), WorldToObject(o2w.GetInverse()),
	reverseOrientation(ro),
	transformSwapsHandedness(o2w.SwapsHandedness()),
	material(mat)
{
	// Update shape creation statistics
	// radiance - disabled for threading // static StatsCounter nShapesMade("Geometry","Total shapes created");
	// radiance - disabled for threading // ++nShapesMade;
}

// PrimitiveSet Method Definitions
PrimitiveSet::PrimitiveSet(boost::shared_ptr<Aggregate> a) {
	a->GetPrimitives(primitives);

	initAreas();

	accelerator = a;
}
PrimitiveSet::PrimitiveSet(const vector<boost::shared_ptr<Primitive> > &p) {
	primitives = p;

	initAreas();

	// NOTE - ratow - Use accelerator for complex lights.
	if(primitives.size() <= 16) {
		accelerator = boost::shared_ptr<Primitive>((Primitive*)NULL);
		worldbound = BBox();
		for(u_int i = 0; i < primitives.size(); i++)
			worldbound = Union(worldbound, primitives[i]->WorldBound());
		// NOTE - ratow - Correctly expands bounds when pMin is not negative or pMax is not positive.
		worldbound.pMin -= (worldbound.pMax-worldbound.pMin)*0.01f;
		worldbound.pMax += (worldbound.pMax-worldbound.pMin)*0.01f;
	} else {
		accelerator = boost::shared_ptr<Primitive>(
				MakeAccelerator("kdtree", primitives, ParamSet()));
		if (!accelerator)
			luxError(LUX_BUG,LUX_SEVERE,"Unable to find \"kdtree\" accelerator");
	}
}
bool PrimitiveSet::Intersect(const Ray &ray, Intersection *in) const {
	if(accelerator) {
		return accelerator->Intersect(ray, in);
	} else if(worldbound.IntersectP(ray)) {
		// NOTE - ratow - Testing each shape for intersections again because the _PrimitiveSet_ can be non-planar.
		bool anyHit = false;
		for (u_int i = 0; i < primitives.size(); ++i) {
			if (primitives[i]->Intersect(ray, in)) {
				anyHit = true;
			}
		}
		return anyHit;
	}
	return false;
}

bool PrimitiveSet::IntersectP(const Ray &ray) const {
	if(accelerator) {
		return accelerator->IntersectP(ray);
	} else if(worldbound.IntersectP(ray)) {
		for (u_int i = 0; i < primitives.size(); ++i) {
			if (primitives[i]->IntersectP(ray)) {
				return true;
			}
		}
	}
	return false;
}

void PrimitiveSet::initAreas() {
	area = 0;
	vector<float> areas;
	areas.reserve(primitives.size());
	for (u_int i = 0; i < primitives.size(); ++i) {
		float a = primitives[i]->Area();
		area += a;
		areas.push_back(a);
	}
	float prevCDF = 0;
	areaCDF.reserve(primitives.size());
	for (u_int i = 0; i < primitives.size(); ++i) {
		areaCDF.push_back(prevCDF + areas[i] / area);
		prevCDF = areaCDF[i];
	}
}
