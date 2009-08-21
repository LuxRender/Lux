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
void Primitive::Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const {
	luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Primitive::Sample method called!");
}
float Primitive::Pdf(const Point &p) const {
	return 1.f / Area();
}
void Primitive::Sample(const Point &p,
		float u1, float u2, float u3, DifferentialGeometry *dg) const
{
	Sample(u1, u2, u3, dg);
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
float Primitive::Pdf(const Point &p, const Point &po) const {
	return 1.f / Area();
}

// Intersection Method Definitions
BSDF *Intersection::GetBSDF(const TsPack *tspack, const RayDifferential &ray)
		const {
	// radiance - disabled for threading // static StatsCounter pointsShaded("Shading", "Number of points shaded"); // NOBOOK
	// radiance - disabled for threading // ++pointsShaded; // NOBOOK
	dg.ComputeDifferentials(ray);
	DifferentialGeometry dgShading;
	primitive->GetShadingGeometry(WorldToObject.GetInverse(), dg, &dgShading);
	return material->GetBSDF(tspack, dg, dgShading);
}
SWCSpectrum Intersection::Le(const TsPack *tspack, const Vector &w) const {
	return arealight ? arealight->L(tspack, dg, w) : SWCSpectrum(0.);
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
	isect->dg.handle = isect->primitive;
	isect->primitive = this;
	if(material)
		isect->material = material.get();
	return true;
}
bool InstancePrimitive::IntersectP(const Ray &r) const {
	return instance->IntersectP(WorldToInstance(r));
}
void InstancePrimitive::GetShadingGeometry(const Transform &obj2world,
	const DifferentialGeometry &dg, DifferentialGeometry *dgShading) const
{
	Transform o2w(WorldToInstance * obj2world);
	// Transform instance's differential geometry to world space
	DifferentialGeometry dgl(WorldToInstance(dg.p),
		Normalize(WorldToInstance(dg.nn)),
		WorldToInstance(dg.dpdu), WorldToInstance(dg.dpdv),
		WorldToInstance(dg.dndu), WorldToInstance(dg.dndv),
		dg.u, dg.v, dg.handle);
	memcpy(dgl.triangleBaryCoords, dg.triangleBaryCoords, 3 * sizeof(float));
	reinterpret_cast<const Primitive *>(dg.handle)->GetShadingGeometry(o2w, dgl, dgShading);
	dgShading->p = InstanceToWorld(dgShading->p);
	dgShading->nn = Normalize(InstanceToWorld(dgShading->nn));
	dgShading->dpdu = InstanceToWorld(dgShading->dpdu);
	dgShading->dpdv = InstanceToWorld(dgShading->dpdv);
	dgShading->dndu = InstanceToWorld(dgShading->dndu);
	dgShading->dndv = InstanceToWorld(dgShading->dndv);
	dgShading->dpdx = InstanceToWorld(dgShading->dpdx);
	dgShading->dpdy = InstanceToWorld(dgShading->dpdy);
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
	isect->dg.handle = isect->primitive;
	isect->primitive = this;
	isect->dg.time = r.time;
	return true;
}

bool MotionPrimitive::IntersectP(const Ray &r) const {
	Transform InstanceToWorld = motionSystem->Sample(r.time);
	Transform WorldToInstance = InstanceToWorld.GetInverse();

	return instance->IntersectP(WorldToInstance(r));
}
void MotionPrimitive::GetShadingGeometry(const Transform &obj2world,
	const DifferentialGeometry &dg, DifferentialGeometry *dgShading) const
{
	Transform InstanceToWorld = motionSystem->Sample(dg.time);
	Transform WorldToInstance = InstanceToWorld.GetInverse();
	Transform o2w(WorldToInstance * obj2world);
	// Transform instance's differential geometry to world space
	DifferentialGeometry dgl(WorldToInstance(dg.p),
		Normalize(WorldToInstance(dg.nn)),
		WorldToInstance(dg.dpdu), WorldToInstance(dg.dpdv),
		WorldToInstance(dg.dndu), WorldToInstance(dg.dndv),
		dg.u, dg.v, dg.handle);
	memcpy(dgl.triangleBaryCoords, dg.triangleBaryCoords, 3 * sizeof(float));
	reinterpret_cast<const Primitive *>(dg.handle)->GetShadingGeometry(o2w, dgl, dgShading);
	dgShading->p = InstanceToWorld(dgShading->p);
	dgShading->nn = Normalize(InstanceToWorld(dgShading->nn));
	dgShading->dpdu = InstanceToWorld(dgShading->dpdu);
	dgShading->dpdv = InstanceToWorld(dgShading->dpdv);
	dgShading->dndu = InstanceToWorld(dgShading->dndu);
	dgShading->dndv = InstanceToWorld(dgShading->dndv);
	dgShading->dpdx = InstanceToWorld(dgShading->dpdx);
	dgShading->dpdy = InstanceToWorld(dgShading->dpdy);
}

BBox MotionPrimitive::WorldBound() const  {
	return motionSystem->Bound(instance->WorldBound());
}

