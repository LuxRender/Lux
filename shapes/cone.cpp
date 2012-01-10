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

// cone.cpp*
#include "cone.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Cone Method Definitions
Cone::Cone(const Transform &o2w, bool ro, const string &name, 
           float ht, float rad, float rad2, float tm)
	: Shape(o2w, ro, name) {
	if (rad2 > rad)
		swap(rad, rad2);
	radius = rad;
	radius2 = rad2;

	if (rad2 > 0.f) {
		// It is a cone frustrum
		height2 = ht;
		// Calculate height of the cone:
		//  Tan(theta) = radius / height
		//  (radius - radius2) / height2 = radius / height
		//  height = (radius * height2) / (radius - radius2)
		height = (radius * height2) / (radius - radius2);
	} else {
		height = ht;
	}

	phiMax = Radians( Clamp( tm, 0.0f, 360.0f ) );
}

BBox Cone::ObjectBound() const {
	const Point p1 = Point(-radius, -radius, 0.f);
	Point p2;
	if (radius2 > 0.f)
		p2 = Point(radius,  radius, height2);
	else
		p2 = Point(radius,  radius, height);

	return BBox(p1, p2);
}

bool Cone::Intersect(const Ray &r, float *tHit,
		DifferentialGeometry *dg) const {
	float phi;
	Point phit;
	// Transform _Ray_ to object space
	Ray ray;
	WorldToObject(r, &ray);

	// Compute quadratic cone coefficients
	float k = radius / height;
	k = k*k;
	const float A = ray.d.x * ray.d.x + ray.d.y * ray.d.y -
		k * ray.d.z * ray.d.z;
	const float B = 2 * (ray.d.x * ray.o.x + ray.d.y * ray.o.y -
		k * ray.d.z * (ray.o.z-height) );
	const float C = ray.o.x * ray.o.x + ray.o.y * ray.o.y -
		k * (ray.o.z -height) * (ray.o.z-height);

	// Solve quadratic equation for _t_ values
	float t0, t1;
	if (!Quadratic(A, B, C, &t0, &t1))
		return false;

	// Compute intersection distance along ray
	if (t0 > ray.maxt || t1 < ray.mint)
		return false;
	float thit = t0;
	if (t0 < ray.mint) {
		thit = t1;
		if (thit > ray.maxt) return false;
	}

	// Compute cone inverse mapping
	phit = ray(thit);
	phi = atan2f(phit.y, phit.x);
	if (phi < 0.) phi += 2.f*M_PI;

	// Test cone intersection against clipping parameters
	if (phit.z < 0 || phit.z > height ||
			((radius2 > 0.f) && (phit.z > height2)) || phi > phiMax) {
		if (thit == t1) return false;
		thit = t1;
		if (t1 > ray.maxt) return false;

		// Compute cone inverse mapping
		phit = ray(thit);
		phi = atan2f(phit.y, phit.x);
		if (phi < 0.) phi += 2.f*M_PI;
		if (phit.z < 0 || phit.z > height  ||
				((radius2 > 0.f) && (phit.z > height2)) || phi > phiMax)
			return false;
	}

	// Find parametric representation of cone hit
	const float u = phi / phiMax;
	const float v = phit.z / height;

	// Compute cone \dpdu and \dpdv
	const Vector dpdu(-phiMax * phit.y, phiMax * phit.x, 0);
	const Vector dpdv(-phit.x / (1.f - v),
	            -phit.y / (1.f - v), height);

	// Compute cone \dndu and \dndv
	const Vector d2Pduu = -phiMax * phiMax *
	                Vector(phit.x, phit.y, 0.);
	const float vn = (radius2 > 0.f) ? (phit.z / height) : v;
	const Vector d2Pduv = phiMax / (1.f - vn) *
	                Vector(-phit.y, -phit.x, 0.);
	const Vector d2Pdvv(0, 0, 0);
	// Compute coefficients for fundamental forms
	const float E = Dot(dpdu, dpdu);
	const float F = Dot(dpdu, dpdv);
	const float G = Dot(dpdv, dpdv);
	const Vector N = Normalize(Cross(dpdu, dpdv));
	const float e = Dot(N, d2Pduu);
	const float f = Dot(N, d2Pduv);
	const float g = Dot(N, d2Pdvv);
	// Compute \dndu and \dndv from fundamental form coefficients
	const float invEGF2 = 1.f / (E*G - F*F);
	const Normal dndu((f*F - e*G) * invEGF2 * dpdu +
		(e*F - f*E) * invEGF2 * dpdv);
	const Normal dndv((g*F - f*G) * invEGF2 * dpdu +
		(f*F - g*E) * invEGF2 * dpdv);
	// Initialize _DifferentialGeometry_ from parametric information
	*dg = DifferentialGeometry(ObjectToWorld(phit),
	                           ObjectToWorld(dpdu),
							   ObjectToWorld(dpdv),
	                           ObjectToWorld(dndu),
							   ObjectToWorld(dndv),
	                           u, ((radius2 > 0.f) ? (phit.z / height2) : v), this);
	// Update _tHit_ for quadric intersection
	*tHit = thit;

	return true;
}

bool Cone::IntersectP(const Ray &r) const {
	float phi;
	Point phit;
	// Transform _Ray_ to object space
	Ray ray;
	WorldToObject(r, &ray);

	// Compute quadratic cone coefficients
	float k = radius / height;
	k = k*k;
	const float A = ray.d.x * ray.d.x + ray.d.y * ray.d.y -
		k * ray.d.z * ray.d.z;
	const float B = 2 * (ray.d.x * ray.o.x + ray.d.y * ray.o.y -
		k * ray.d.z * (ray.o.z-height) );
	const float C = ray.o.x * ray.o.x + ray.o.y * ray.o.y -
		k * (ray.o.z -height) * (ray.o.z-height);

	// Solve quadratic equation for _t_ values
	float t0, t1;
	if (!Quadratic(A, B, C, &t0, &t1))
		return false;

	// Compute intersection distance along ray
	if (t0 > ray.maxt || t1 < ray.mint)
		return false;
	float thit = t0;
	if (t0 < ray.mint) {
		thit = t1;
		if (thit > ray.maxt) return false;
	}

	// Compute cone inverse mapping
	phit = ray(thit);
	phi = atan2f(phit.y, phit.x);
	if (phi < 0.) phi += 2.f*M_PI;

	// Test cone intersection against clipping parameters
	if (phit.z < 0 || phit.z > height ||
			((radius2 > 0.f) && (phit.z > height2)) || phi > phiMax) {
		if (thit == t1) return false;
		thit = t1;
		if (t1 > ray.maxt) return false;
		// Compute cone inverse mapping
		phit = ray(thit);
		phi = atan2f(phit.y, phit.x);
		if (phi < 0.) phi += 2.f*M_PI;
		if (phit.z < 0 || phit.z > height ||
				((radius2 > 0.f) && (phit.z > height2)) || phi > phiMax)
			return false;
	}

	return true;
}

float Cone::Area() const {
	float area = phiMax * height * height *
				sqrtf((height * height) + (radius * radius)) / (2.0f * radius);

	if (radius2 > 0.f) {
		const float apexHeight = height - height2;
		area -= phiMax * apexHeight * apexHeight *
				sqrtf((apexHeight * apexHeight) + (radius2 * radius2)) / (2.0f * radius2);
	}

	return area;
}

Shape* Cone::CreateShape(const Transform &o2w,
		bool reverseOrientation, const ParamSet &params) {

	string name = params.FindOneString("name", "'cone'");
	float phimax = params.FindOneFloat("phimax", 360.f);

	float radius = params.FindOneFloat("radius", 1.f);
	float radius2 = params.FindOneFloat("radius2", 0.f);
	float height = params.FindOneFloat("height", 1.f);

	return new Cone(o2w, reverseOrientation, name, height, radius, radius2, phimax);
}

static DynamicLoader::RegisterShape<Cone> r("cone");
