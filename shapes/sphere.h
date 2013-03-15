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

// sphere.cpp*
#include "shape.h"
#include "mc.h"

namespace lux
{

// Sphere Declarations
class Sphere: public Shape {
public:
	// Sphere Public Methods
	Sphere(const Transform &o2w, bool ro, const string &name, 
	       float rad, float zmin, float zmax, float phiMax);
	virtual ~Sphere() { }
	virtual BBox ObjectBound() const;
	virtual bool Intersect(const Ray &ray, Intersection *isect, bool null_shp_isect = false) const;
	virtual bool IntersectP(const Ray &ray, bool null_shp_isect = false) const;
	virtual float Area() const;
	virtual float Sample(float u1, float u2, float u3,
		DifferentialGeometry *dg) const {
		// Uniformly sample partial sphere
		const float Z = zMax - zMin;
		const float z = Z * u1 + zMin;
		const float radius2 = radius * radius;
		const float r = sqrtf(max(0.f, radius2 - z * z));
		const float phi = phiMax * u2;
		const Point p(r * cosf(phi), r * sinf(phi), z);
		const float factor = -Z * z / (r * r);
		const Vector dpdu(factor * p.x, factor * p.y, Z);
		const Vector dpdv(-phiMax * p.y, phiMax * p.x, 0.f);
		*dg = DifferentialGeometry(ObjectToWorld * p,
			Normalize(ObjectToWorld * Normal(p.x, p.y, p.z)),
			ObjectToWorld * dpdu, ObjectToWorld * dpdv,
			ObjectToWorld * Normal(dpdu / radius),
			ObjectToWorld * Normal(dpdv / radius),
			u1, u2, this);
		dg->AdjustNormal(reverseOrientation, transformSwapsHandedness);
		return fabsf(1.f / Dot(Cross(dg->dpdu, dg->dpdv), dg->nn));
	}
	virtual float Pdf(const PartialDifferentialGeometry &dg) const {
		return fabsf(1.f / Dot(Cross(dg.dpdu, dg.dpdv), dg.nn));
	}
	virtual float Sample(const Point &p, float u1, float u2, float u3,
		DifferentialGeometry *dg) const {
		return Sample(u1, u2, u3, dg);
// The following has been disabled because it is erroneous for partial spheres
/*		// Compute coordinate system for sphere sampling
		Point Pcenter = ObjectToWorld(Point(0,0,0));
		Vector wc = Pcenter - p;
		const float d2 = wc.LengthSquared();
		// Sample uniformly on sphere if \pt is inside it
		if (d2 - radius * radius < 1e-4f)
			return Sample(u1, u2, u3, dg);
		wc /= d2;
		Vector wcX, wcY;
		CoordinateSystem(wc, &wcX, &wcY);
		// Sample sphere uniformly inside subtended cone
		float cosThetaMax = sqrtf(max(0.f, 1.f - radius * radius /
			DistanceSquared(p, Pcenter)));
		DifferentialGeometry dgSphere;
		float thit;
		Point ps;
		Ray r(p, UniformSampleCone(u1, u2, cosThetaMax, wcX, wcY, wc));
		if (!Intersect(r, &thit, &dgSphere))
			ps = Pcenter - radius * wc;
		else
			ps = r(thit);
		*ns = Normal(Normalize(ps - Pcenter));
		if (reverseOrientation)
			*ns *= -1.f;
		return ps;*/
	}
	virtual float Pdf(const Point &p, const PartialDifferentialGeometry &dg) const {
		return Pdf(dg);
// The following has been disabled because it is erroneous for partial spheres
/*		Point Pcenter = ObjectToWorld(Point(0,0,0));
		// Return uniform weight if point inside sphere
		const float dc2 = DistanceSquared(p, Pcenter);
		const float r2 = radius * radius;
		if (dc2 - r2 < 1e-4f)
			return 1.f / Area();
		// Compute general sphere weight
		const float cosThetaMax = sqrtf(max(0.f, 1.f - r2 / dc2));
		const Vector w(p - dg.p);
		const float d2 = w.LengthSquared();
		if (d2 > dc2 + r2)
			return 0.f;
		return UniformConePdf(cosThetaMax) * AbsDot(w, dg.p - Pcenter) /
			(d2 * sqrtf(d2) * radius);*/
	}
	
	static Shape* CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);
private:
	// Sphere Private Data
	float radius;
	float phiMax;
	float zMin, zMax;
	float thetaMin, thetaMax;
};

}//namespace lux
