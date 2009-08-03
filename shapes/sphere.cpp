/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

// sphere.cpp*
#include "sphere.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Sphere Method Definitions
Sphere::Sphere(const Transform &o2w, bool ro, float rad,
               float z0, float z1, float pm)
	: Shape(o2w, ro) {
	radius = rad;
	zmin = Clamp(min(z0, z1), -radius, radius);
	zmax = Clamp(max(z0, z1), -radius, radius);
	thetaMin = acosf(Clamp(zmin/radius, -1.f, 1.f));
	thetaMax = acosf(Clamp(zmax/radius, -1.f, 1.f));
	phiMax = Radians(Clamp(pm, 0.0f, 360.0f));
}
BBox Sphere::ObjectBound() const {
	return BBox(Point(-radius, -radius, zmin),
		Point(radius,  radius, zmax));
}
bool Sphere::Intersect(const Ray &r, float *tHit,
		DifferentialGeometry *dg) const {
	float phi;
	Point phit;
	// Transform _Ray_ to object space
	Ray ray;
	WorldToObject(r, &ray);
	// Compute quadratic sphere coefficients
	float A = ray.d.x*ray.d.x + ray.d.y*ray.d.y +
	          ray.d.z*ray.d.z;
	float B = 2 * (ray.d.x*ray.o.x + ray.d.y*ray.o.y +
				   ray.d.z*ray.o.z);
	float C = ray.o.x*ray.o.x + ray.o.y*ray.o.y +
			  ray.o.z*ray.o.z - radius*radius;
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
	// Compute sphere hit position and $\phi$
	phit = ray(thit);
	phi = atan2f(phit.y, phit.x);
	if (phi < 0.) phi += 2.f*M_PI;
	// Test sphere intersection against clipping parameters
	if (phit.z < zmin || phit.z > zmax || phi > phiMax) {
		if (thit == t1) return false;
		if (t1 > ray.maxt) return false;
		thit = t1;
		// Compute sphere hit position and $\phi$
		phit = ray(thit);
		phi = atan2f(phit.y, phit.x);
		if (phi < 0.) phi += 2.f*M_PI;
		if (phit.z < zmin || phit.z > zmax || phi > phiMax)
			return false;
	}
	// Find parametric representation of sphere hit
	float u = phi / phiMax;
	float theta = acosf(phit.z / radius);
	float v = (theta - thetaMin) / (thetaMax - thetaMin);
	// Compute sphere \dpdu and \dpdv
	float cosphi, sinphi;
	Vector dpdu, dpdv;
	float zradius = sqrtf(phit.x*phit.x + phit.y*phit.y);
	if (zradius == 0)
	{
		// Handle hit at degenerate parameterization point
		cosphi = 0;
		sinphi = 1;
		dpdv = (thetaMax-thetaMin) *
			Vector(phit.z * cosphi, phit.z * sinphi,
				-radius * sinf(theta));
		Vector norm = Vector(phit);
		dpdu = Cross(dpdv, norm);
	}
	else
	{
		float invzradius = 1.f / zradius;
		cosphi = phit.x * invzradius;
		sinphi = phit.y * invzradius;
		dpdu = Vector(-phiMax * phit.y, phiMax * phit.x, 0);
		dpdv = (thetaMax-thetaMin) *
			Vector(phit.z * cosphi, phit.z * sinphi,
				-radius * sinf(theta));
	}
	// Compute sphere \dndu and \dndv
	Vector d2Pduu = -phiMax * phiMax * Vector(phit.x,phit.y,0);
	Vector d2Pduv = (thetaMax - thetaMin) *
	                 phit.z * phiMax *
	                 Vector(-sinphi, cosphi, 0.);
	Vector d2Pdvv = -(thetaMax - thetaMin) *
	                 (thetaMax - thetaMin) *
	                 Vector(phit.x, phit.y, phit.z);
	// Compute coefficients for fundamental forms
	float E = Dot(dpdu, dpdu);
	float F = Dot(dpdu, dpdv);
	float G = Dot(dpdv, dpdv);
	Vector N = Normalize(Cross(dpdu, dpdv));
	float e = Dot(N, d2Pduu);
	float f = Dot(N, d2Pduv);
	float g = Dot(N, d2Pdvv);
	// Compute \dndu and \dndv from fundamental form coefficients
	float invEGF2 = 1.f / (E*G - F*F);
	Normal dndu((f*F - e*G) * invEGF2 * dpdu +
		(e*F - f*E) * invEGF2 * dpdv);
	Normal dndv((g*F - f*G) * invEGF2 * dpdu +
		(f*F - g*E) * invEGF2 * dpdv);
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
bool Sphere::IntersectP(const Ray &r) const {
	float phi;
	Point phit;
	// Transform _Ray_ to object space
	Ray ray;
	WorldToObject(r, &ray);
	// Compute quadratic sphere coefficients
	float A = ray.d.x*ray.d.x + ray.d.y*ray.d.y +
	          ray.d.z*ray.d.z;
	float B = 2 * (ray.d.x*ray.o.x + ray.d.y*ray.o.y +
				   ray.d.z*ray.o.z);
	float C = ray.o.x*ray.o.x + ray.o.y*ray.o.y +
			  ray.o.z*ray.o.z - radius*radius;
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
	// Compute sphere hit position and $\phi$
	phit = ray(thit);
	phi = atan2f(phit.y, phit.x);
	if (phi < 0.) phi += 2.f*M_PI;
	// Test sphere intersection against clipping parameters
	if (phit.z < zmin || phit.z > zmax || phi > phiMax) {
		if (thit == t1) return false;
		if (t1 > ray.maxt) return false;
		thit = t1;
		// Compute sphere hit position and $\phi$
		phit = ray(thit);
		phi = atan2f(phit.y, phit.x);
		if (phi < 0.) phi += 2.f*M_PI;
		if (phit.z < zmin || phit.z > zmax || phi > phiMax)
			return false;
	}
	return true;
}
float Sphere::Area() const {
	return phiMax * radius * (zmax-zmin);
}
Shape* Sphere::CreateShape(const Transform &o2w,
					   bool reverseOrientation,
					   const ParamSet &params) {
	float radius = params.FindOneFloat("radius", 1.f);
	float zmin = params.FindOneFloat("zmin", -radius);
	float zmax = params.FindOneFloat("zmax", radius);
	float phimax = params.FindOneFloat("phimax", 360.f);
	return new Sphere(o2w, reverseOrientation, radius,
		zmin, zmax, phimax);
}

static DynamicLoader::RegisterShape<Sphere> r("sphere");
