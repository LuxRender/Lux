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

/*
* lenscomponent for realistic camera, based on:
* Lens Component plugin Copyright(c) 2004 Nico Galoppo von Borries
*/

// lenscomponent.cpp*
#include "lenscomponent.h"
#include "paramset.h"
#include "dynload.h"
#include "luxrays/utils/mc.h"

using namespace luxrays;
using namespace lux;

// LensComponent Method Definitions
LensComponent::LensComponent(const Transform &o2w, bool ro, const string &name, 
               float rad, float z0, float z1, float pm, float ap)
               : Shape(o2w, ro, name) {
                   radius = rad;
                   zmin = Clamp(min(z0, z1), -radius, radius);
                   zmax = Clamp(max(z0, z1), -radius, radius);
                   thetaMin = acosf(Clamp(zmin/radius, -1.f, 1.f));
                   thetaMax = acosf(Clamp(zmax/radius, -1.f, 1.f));
                   phiMax = Radians(Clamp(pm, 0.0f, 360.0f));
                   apRadius = ap / 2.0f;
}
Point LensComponent::Sample(float u1, float u2, float u3, Normal *n) const
{
	float lensU, lensV;
	ConcentricSampleDisk(u1, u2, &lensU, &lensV);
	lensU *= radius;
	lensV *= radius;
	return ObjectToWorld * (Point(0.f, 0.f, 0.f) + Vector(lensU, lensV, 0.f));
}
BBox LensComponent::ObjectBound() const
{
	return BBox(Point(-radius, -radius, zmin), Point(radius, radius, zmax));
}
bool LensComponent::Intersect(const Ray &r, float *tHit,
	DifferentialGeometry *dg) const
{
	float phi;
	Point phit;
	// Transform _Ray_ to object space
	Ray ray(Inverse(ObjectToWorld) * r);
	// Compute quadratic LensComponent coefficients
	float A = Dot(ray.d, ray.d);
	float B = 2.f * (ray.d.x * ray.o.x + ray.d.y * ray.o.y + ray.d.z * ray.o.z);
	float C = ray.o.x * ray.o.x + ray.o.y * ray.o.y + ray.o.z * ray.o.z - radius * radius;
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
		if (thit > ray.maxt)
			return false;
	}
	// Compute LensComponent hit position and $\phi$
	phit = ray(thit);
	phi = atan2f(phit.y, phit.x);
	if (phi < 0.f)
		phi += 2.f*M_PI;
	// Test LensComponent aperture
	float dist2 = phit.x * phit.x + phit.y * phit.y;
	if (dist2 > apRadius * apRadius)
		return false;
	// Find parametric representation of LensComponent hit
	float u = phi / phiMax;
	float theta = acosf(phit.z / radius);
	float v = (theta - thetaMin) / (thetaMax - thetaMin);
	// Compute LensComponent \dpdu and \dpdv
	float zradius = sqrtf(phit.x*phit.x + phit.y*phit.y);
	float invzradius = 1.f / zradius;
	float cosphi = phit.x * invzradius;
	float sinphi = phit.y * invzradius;
	Vector dpdu(-phiMax * phit.y, phiMax * phit.x, 0);
	Vector dpdv = (thetaMax-thetaMin) *
		Vector(phit.z * cosphi, phit.z * sinphi, -radius * sinf(theta));
	//// Compute LensComponent \dndu and \dndv
	//Vector d2Pduu = -phiMax * phiMax * Vector(phit.x,phit.y,0);
	//Vector d2Pduv = (thetaMax - thetaMin) *
	//    phit.z * phiMax *
	//    Vector(-sinphi, cosphi, 0.);
	//Vector d2Pdvv = -(thetaMax - thetaMin) *
	//    (thetaMax - thetaMin) *
	//    Vector(phit.x, phit.y, phit.z);
	//// Compute coefficients for fundamental forms
	//float E = Dot(dpdu, dpdu);
	//float F = Dot(dpdu, dpdv);
	//float G = Dot(dpdv, dpdv);
	//Vector N = Normalize(Cross(dpdu, dpdv));
	//float e = Dot(N, d2Pduu);
	//float f = Dot(N, d2Pduv);
	//float g = Dot(N, d2Pdvv);
	//// Compute \dndu and \dndv from fundamental form coefficients
	//float invEGF2 = 1.f / (E*G - F*F);
	//Vector dndu = (f*F - e*G) * invEGF2 * dpdu +
	//    (e*F - f*E) * invEGF2 * dpdv;
	//Vector dndv = (g*F - f*G) * invEGF2 * dpdu +
	//    (f*F - g*E) * invEGF2 * dpdv;
	// Initialize _DifferentialGeometry_ from parametric information
	*dg = DifferentialGeometry(ObjectToWorld * phit,
		ObjectToWorld * dpdu, ObjectToWorld * dpdv,
		ObjectToWorld * Normal(), ObjectToWorld * Normal(),
		u, v, this);
	// Update _tHit_ for quadric intersection
	*tHit = thit;
	return true;
}
bool LensComponent::IntersectP(const Ray &r) const
{
	//float phi;
	Point phit;
	// Transform _Ray_ to object space
	Ray ray(Inverse(ObjectToWorld) * r);
	// Compute quadratic LensComponent coefficients
	float A = Dot(ray.d, ray.d);
	float B = 2.f * (ray.d.x * ray.o.x + ray.d.y * ray.o.y + ray.d.z * ray.o.z);
	float C = ray.o.x * ray.o.x + ray.o.y * ray.o.y + ray.o.z * ray.o.z - radius * radius;
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
		if (thit > ray.maxt)
			return false;
	}
	// Compute LensComponent hit position and $\phi$
	phit = ray(thit);
	//phi = atan2f(phit.y, phit.x);
	//if (phi < 0.) phi += 2.f*M_PI;
	// Test LensComponent aperture
	float dist2 = phit.x * phit.x + phit.y * phit.y;
	if (dist2 > apRadius * apRadius)
		return false;
	// Test LensComponent intersection against clipping parameters
	//if (phit.z < zmin || phit.z > zmax || phi > phiMax) {
	//    if (thit == t1) return false;
	//    if (t1 > ray.maxt) return false;
	//    thit = t1;
	//    // Compute LensComponent hit position and $\phi$
	//    phit = ray(thit);
	//    phi = atan2f(phit.y, phit.x);
	//    if (phi < 0.) phi += 2.f*M_PI;
	//    if (phit.z < zmin || phit.z > zmax || phi > phiMax)
	//        return false;
	//}
	return true;
}
float LensComponent::Area() const
{
	return phiMax * radius * (zmax-zmin);
}

Shape* LensComponent::CreateShape(const Transform &o2w,
                   bool reverseOrientation,
                   const ParamSet &params) {
	string name = params.FindOneString("name", "'lenscomponent'");
    float radius = params.FindOneFloat("radius", 1.f);
    float zmin = params.FindOneFloat("zmin", -radius);
    float zmax = params.FindOneFloat("zmax", radius);
    float phimax = params.FindOneFloat("phimax", 360.f);
    float aperture = params.FindOneFloat("aperture", 1.f);
    return new LensComponent(o2w, reverseOrientation, name, radius,
        zmin, zmax, phimax, aperture);
}

static DynamicLoader::RegisterShape<LensComponent> r("lenscomponent");

