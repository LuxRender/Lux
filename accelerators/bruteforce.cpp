
/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

// Brute force ray intersection (de)accelerator.
// Only useful for debugging as this is way slow on big scenes.

// bruteforce.cpp*
#include "bruteforce.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// BruteForceAccel Method Definitions
BruteForceAccel::BruteForceAccel(const vector<Primitive* > &p) {
	for (u_int i = 0; i < p.size(); ++i)
		p[i]->FullyRefine(prims);
	// Compute bounds
	for (u_int i = 0; i < prims.size(); ++i)
		bounds = Union(bounds, prims[i]->WorldBound());
}

BruteForceAccel::~BruteForceAccel() {
}

BBox BruteForceAccel::WorldBound() const {
	return bounds;
}

bool BruteForceAccel::Intersect(const Ray &ray,
                          Intersection *isect) const {
	bool hitSomething = false;

	if (!bounds.IntersectP(ray))
		return false;

	for (u_int i = 0; i < prims.size(); ++i) {
		hitSomething |= prims[i]->Intersect(ray, isect);
	}

	return hitSomething;
}

bool BruteForceAccel::IntersectP(const Ray &ray) const {
	if (!bounds.IntersectP(ray))
		return false;

	for (u_int i = 0; i < prims.size(); ++i) {
		if(prims[i]->IntersectP(ray))
			return true;
	}

	return false;
}

Primitive* BruteForceAccel::CreateAccelerator(const vector<Primitive* > &prims,
		const ParamSet &ps) {
	return new BruteForceAccel(prims);
}

static DynamicLoader::RegisterAccelerator<BruteForceAccel> r("none");
