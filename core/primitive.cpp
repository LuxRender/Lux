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

// primitive.cpp*
#include "primitive.h"
#include "light.h"
#include "material.h"
#include "motionsystem.h"

using namespace lux;

// Primitive Method Definitions
Primitive::~Primitive() { }

void Primitive::Refine(vector<boost::shared_ptr<Primitive> > &refined,
		const PrimitiveRefinementHints& refineHints, boost::shared_ptr<Primitive> thisPtr)
{
	luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Primitive::Refine method called!");
}


bool Primitive::Intersect(const Ray &r, Intersection *in) const {
	luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Primitive::Intersect method called!");
	return false;
}
bool Primitive::IntersectP(const Ray &r) const {
	luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Primitive::IntersectP method called!");
	return false;
}


void Primitive::GetShadingGeometry(const Transform &obj2world,
			const DifferentialGeometry &dg, DifferentialGeometry *dgShading) const
{
	luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Primitive::GetShadingGeometry method called!");
}


float Primitive::Area() const {
	luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Primitive::Area method called!");
	return 0.f;
}
Point Primitive::Sample(float u1, float u2, float u3, Normal *Ns) const {
	luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Primitive::Sample method called!");
	return Point();
}
float Primitive::Pdf(const Point &p) const {
	return 1.f / Area();
}
Point Primitive::Sample(const Point &p,
		float u1, float u2, float u3, Normal *Ns) const
{
	return Sample(u1, u2, u3, Ns);
}
float Primitive::Pdf(const Point &p, const Vector &wi) const {
	// Intersect sample ray with area light geometry
	Intersection isect;
	Ray ray(p, wi);
	if (!Intersect(ray, &isect)) return 0.f;
	// Convert light sample weight to solid angle measure
	float pdf = DistanceSquared(p, ray(ray.maxt)) /
		(AbsDot(isect.dg.nn, -wi) * Area());
	if (AbsDot(isect.dg.nn, -wi) == 0.f) pdf = INFINITY;
	return pdf;
}

// DifferentialGeometry Method Definitions
DifferentialGeometry::DifferentialGeometry(const Point &P,
		const Vector &DPDU, const Vector &DPDV,
		const Vector &DNDU, const Vector &DNDV,
		float uu, float vv, const Primitive *pr)
	: p(P), dpdu(DPDU), dpdv(DPDV), dndu((Normal)DNDU), dndv((Normal)DNDV) {
	// Initialize _DifferentialGeometry_ from parameters
	nn = Normal(Normalize(Cross(dpdu, dpdv)));
	u = uu;
	v = vv;
	prim = pr;
	dudx = dvdx = dudy = dvdy = 0;
}
// Dade - added this costructor as a little optimization if the
// normalized normal is already available
DifferentialGeometry::DifferentialGeometry(const Point &P,
		const Normal &NN,
		const Vector &DPDU, const Vector &DPDV,
		const Vector &DNDU, const Vector &DNDV,
		float uu, float vv, const Primitive *pr)
	: p(P), nn(NN), dpdu(DPDU), dpdv(DPDV), dndu((Normal)DNDU), dndv((Normal)DNDV) {
	// Initialize _DifferentialGeometry_ from parameters
	u = uu;
	v = vv;
	prim = pr;
	dudx = dvdx = dudy = dvdy = 0;
}
void DifferentialGeometry::ComputeDifferentials(
		const RayDifferential &ray) const {
	if (ray.hasDifferentials) {
		// Estimate screen-space change in \pt and $(u,v)$
		// Compute auxiliary intersection points with plane
		float d = -Dot(nn, Vector(p.x, p.y, p.z));
		Vector rxv(ray.rx.o.x, ray.rx.o.y, ray.rx.o.z);
		float tx = -(Dot(nn, rxv) + d) / Dot(nn, ray.rx.d);
		Point px = ray.rx.o + tx * ray.rx.d;
		Vector ryv(ray.ry.o.x, ray.ry.o.y, ray.ry.o.z);
		float ty = -(Dot(nn, ryv) + d) / Dot(nn, ray.ry.d);
		Point py = ray.ry.o + ty * ray.ry.d;
		dpdx = px - p;
		dpdy = py - p;
		// Compute $(u,v)$ offsets at auxiliary points
		// Initialize _A_, _Bx_, and _By_ matrices for offset computation
		float A[2][2], Bx[2], By[2], x[2];
		int axes[2];
		if (fabsf(nn.x) > fabsf(nn.y) && fabsf(nn.x) > fabsf(nn.z)) {
			axes[0] = 1; axes[1] = 2;
		}
		else if (fabsf(nn.y) > fabsf(nn.z)) {
			axes[0] = 0; axes[1] = 2;
		}
		else {
			axes[0] = 0; axes[1] = 1;
		}
		// Initialize matrices for chosen projection plane
		A[0][0] = dpdu[axes[0]];
		A[0][1] = dpdv[axes[0]];
		A[1][0] = dpdu[axes[1]];
		A[1][1] = dpdv[axes[1]];
		Bx[0] = px[axes[0]] - p[axes[0]];
		Bx[1] = px[axes[1]] - p[axes[1]];
		By[0] = py[axes[0]] - p[axes[0]];
		By[1] = py[axes[1]] - p[axes[1]];
		if (SolveLinearSystem2x2(A, Bx, x)) {
			dudx = x[0]; dvdx = x[1];
		}
		else  {
			dudx = 1.; dvdx = 0.;
		}
		if (SolveLinearSystem2x2(A, By, x)) {
			dudy = x[0]; dvdy = x[1];
		}
		else {
			dudy = 0.; dvdy = 1.;
		}
	}
	else {
		dudx = dvdx = 0.;
		dudy = dvdy = 0.;
		dpdx = dpdy = Vector(0,0,0);
	}
}

// Intersection Method Definitions
BSDF *Intersection::GetBSDF(const TsPack *tspack, const RayDifferential &ray, float u)
		const {
	// radiance - disabled for threading // static StatsCounter pointsShaded("Shading", "Number of points shaded"); // NOBOOK
	// radiance - disabled for threading // ++pointsShaded; // NOBOOK
	dg.ComputeDifferentials(ray);
	DifferentialGeometry dgShading;
	primitive->GetShadingGeometry(WorldToObject.GetInverse(), dg, &dgShading);
	return material->GetBSDF(tspack, dg, dgShading, u);
}
SWCSpectrum Intersection::Le(const TsPack *tspack, const Vector &w) const {
	return arealight ? arealight->L(tspack, dg.p, dg.nn, w) : SWCSpectrum(0.);
}
SWCSpectrum Intersection::Le(const TsPack *tspack, const Ray &ray, const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const
{
	if (arealight)
		return arealight->L(tspack, ray, dg, n, bsdf, pdf, pdfDirect);
	*pdf = *pdfDirect = 0.f;
	*bsdf = NULL;
	return 0.f;
}

// AreaLightPrimitive Method Definitions
AreaLightPrimitive::AreaLightPrimitive(boost::shared_ptr<Primitive> aPrim,
		AreaLight* aAreaLight)
{
	this->prim = aPrim;
	this->areaLight = aAreaLight;
}
void AreaLightPrimitive::Refine(vector<boost::shared_ptr<Primitive> > &refined,
		const PrimitiveRefinementHints& refineHints,
		boost::shared_ptr<Primitive> thisPtr)
{
	// Refine the decorated primitive and add an arealight decorator to each result
	vector<boost::shared_ptr<Primitive> > tmpRefined;
	prim->Refine(tmpRefined, refineHints, prim);
	for(u_int i=0; i<tmpRefined.size(); i++) {
		boost::shared_ptr<Primitive> currPrim(
			new AreaLightPrimitive(tmpRefined[i], areaLight));
		refined.push_back(currPrim);
	}
}
bool AreaLightPrimitive::Intersect(const Ray &r, Intersection *in) const {
	if(!prim->Intersect(r, in))
		return false;
	in->arealight = areaLight; // set the intersected arealight
	return true;
}

// InstancePrimitive Method Definitions
bool InstancePrimitive::Intersect(const Ray &r,
								  Intersection *isect) const {
	Ray ray = WorldToInstance(r);
	if (!instance->Intersect(ray, isect))
		return false;
	r.maxt = ray.maxt;
	isect->WorldToObject = isect->WorldToObject * WorldToInstance;
	// Transform instance's differential geometry to world space
	isect->dg.p = InstanceToWorld(isect->dg.p);
	isect->dg.nn = Normalize(InstanceToWorld(isect->dg.nn));
	isect->dg.dpdu = InstanceToWorld(isect->dg.dpdu);
	isect->dg.dpdv = InstanceToWorld(isect->dg.dpdv);
	isect->dg.dndu = InstanceToWorld(isect->dg.dndu);
	isect->dg.dndv = InstanceToWorld(isect->dg.dndv);
	if(material)
		isect->material = material.get();
	return true;
}
bool InstancePrimitive::IntersectP(const Ray &r) const {
	return instance->IntersectP(WorldToInstance(r));
}

// MotionPrimitive Method Definitions
bool MotionPrimitive::Intersect(const Ray &r,
                                                                  Intersection *isect) const {
	Transform InstanceToWorld = motionSystem->Sample(r.time);
	Transform WorldToInstance = InstanceToWorld.GetInverse();

        Ray ray = WorldToInstance(r);
        if (!instance->Intersect(ray, isect))
                return false;
        r.maxt = ray.maxt;
        isect->WorldToObject = isect->WorldToObject * WorldToInstance;
        // Transform instance's differential geometry to world space
        isect->dg.p = InstanceToWorld(isect->dg.p);
        isect->dg.nn = Normalize(InstanceToWorld(isect->dg.nn));
        isect->dg.dpdu = InstanceToWorld(isect->dg.dpdu);
        isect->dg.dpdv = InstanceToWorld(isect->dg.dpdv);
        isect->dg.dndu = InstanceToWorld(isect->dg.dndu);
        isect->dg.dndv = InstanceToWorld(isect->dg.dndv);
        return true;
}
bool MotionPrimitive::IntersectP(const Ray &r) const {
        Transform InstanceToWorld = motionSystem->Sample(r.time);
        Transform WorldToInstance = InstanceToWorld.GetInverse();

        return instance->IntersectP(WorldToInstance(r));
}

BBox MotionPrimitive::WorldBound() const  {
	return motionSystem->Bound(instance->WorldBound());
}

