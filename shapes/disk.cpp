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

// disk.cpp*
#include "disk.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Disk Method Definitions
Disk::Disk(const Transform &o2w, bool ro, float ht,
           float r, float ri, float tmax)
	: Shape(o2w, ro) {
	height = ht;
	radius = r;
	innerRadius = ri;
	phiMax = Radians(Clamp(tmax, 0.0f, 360.0f));
}
BBox Disk::ObjectBound() const {
	return BBox(Point(-radius, -radius, height),
				Point(radius, radius, height));
}
bool Disk::Intersect(const TsPack *tspack, const Ray &r, float *tHit,
		DifferentialGeometry *dg) const {
	// Transform _Ray_ to object space
	Ray ray;
	WorldToObject(r, &ray);
	// Compute plane intersection for disk
	if (fabsf(ray.d.z) < 1e-7) return false;
	float thit = (height - ray.o.z) / ray.d.z;
	if (thit < ray.mint || thit > ray.maxt)
		return false;
	// See if hit point is inside disk radii and \phimax
	Point phit = ray(thit);
	float dist2 = phit.x * phit.x + phit.y * phit.y;
	if (dist2 > radius * radius ||
	    dist2 < innerRadius*innerRadius)
		return false;
	// Test disk $\phi$ value against \phimax
	float phi = atan2f(phit.y, phit.x);
	if (phi < 0.f)
		phi += 2.f * M_PI;
	if (phi > phiMax)
		return false;
	// Find parametric representation of disk hit
	float u = phi / phiMax;
	float v = 1.f - ((sqrtf(dist2)-innerRadius) /
	                 (radius-innerRadius));
	Vector dpdu(-phiMax * phit.y, phiMax * phit.x, 0.);
	dpdu *= phiMax * INV_TWOPI;
	Vector dpdv(-phit.x / (1-v), -phit.y / (1-v), 0.);
	dpdv *= (radius - innerRadius) / radius;
	Normal dndu(0,0,0), dndv(0,0,0);
	// Initialize _DifferentialGeometry_ from parametric information
	*dg = DifferentialGeometry(ObjectToWorld(phit),
	                           ObjectToWorld(dpdu),
							   ObjectToWorld(dpdv),
	                           ObjectToWorld(dndu),
							   ObjectToWorld(dndv),
	                           u, v, this);
	// Update _tHit_ for quadric intersection
	*tHit = thit;
	return true;
}
bool Disk::IntersectP(const TsPack *tspack, const Ray &r) const {
	// Transform _Ray_ to object space
	Ray ray;
	WorldToObject(r, &ray);
	// Compute plane intersection for disk
	if (fabsf(ray.d.z) < 1e-7) return false;
	float thit = (height - ray.o.z) / ray.d.z;
	if (thit < ray.mint || thit > ray.maxt)
		return false;
	// See if hit point is inside disk radii and \phimax
	Point phit = ray(thit);
	float dist2 = phit.x * phit.x + phit.y * phit.y;
	if (dist2 > radius * radius ||
	    dist2 < innerRadius*innerRadius)
		return false;
	// Test disk $\phi$ value against \phimax
	float phi = atan2f(phit.y, phit.x);
	if (phi < 0.f)
		phi += 2.f * M_PI;
	if (phi > phiMax)
		return false;
	return true;
}
float Disk::Area() const {
	return phiMax * 0.5f *
            (radius * radius -innerRadius * innerRadius );
}
Shape* Disk::CreateShape(const Transform &o2w,
		bool reverseOrientation, const ParamSet &params) {
	float height = params.FindOneFloat( "height", 0. );
	float radius = params.FindOneFloat( "radius", 1 );
	float inner_radius = params.FindOneFloat( "innerradius", 0 );
	float phimax = params.FindOneFloat( "phimax", 360 );
	return new Disk(o2w, reverseOrientation, height, radius, inner_radius, phimax);
}

static DynamicLoader::RegisterShape<Disk> r("disk");
