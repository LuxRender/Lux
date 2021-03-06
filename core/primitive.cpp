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

// primitive.cpp*
#include "primitive.h"
#include "light.h"
#include "material.h"

#include "luxrays/core/geometry/motionsystem.h"

using namespace luxrays;
using namespace lux;

// Primitive Method Definitions
void Primitive::Refine(vector<boost::shared_ptr<Primitive> > &refined,
	const PrimitiveRefinementHints& refineHints,
	const boost::shared_ptr<Primitive> &thisPtr)
{
	LOG( LUX_SEVERE,LUX_BUG)<< "Unimplemented Primitive::Refine method called!";
}

bool Primitive::Intersect(const Ray &r, Intersection *in) const
{
	LOG( LUX_SEVERE,LUX_BUG)<< "Unimplemented Primitive::Intersect method called!";
	return false;
}

bool Primitive::IntersectP(const Ray &r) const
{
	LOG( LUX_SEVERE,LUX_BUG)<< "Unimplemented Primitive::IntersectP method called!";
	return false;
}

void Primitive::GetShadingGeometry(const Transform &obj2world,
	const DifferentialGeometry &dg, DifferentialGeometry *dgShading) const
{
	LOG( LUX_SEVERE,LUX_BUG)<< "Unimplemented Primitive::GetShadingGeometry method called!";
}

void Primitive::GetShadingInformation(const DifferentialGeometry &dgShading,
		RGBColor *color, float *alpha) const {
	*color = RGBColor(1.f);
	*alpha = 1.f;
}

float Primitive::Area() const
{
	LOG( LUX_SEVERE,LUX_BUG)<< "Unimplemented Primitive::Area method called!";
	return 0.f;
}

float Primitive::Sample(float u1, float u2, float u3,
	DifferentialGeometry *dg) const
{
	LOG( LUX_SEVERE,LUX_BUG)<< "Unimplemented Primitive::Sample method called!";
	return 0.f;
}

// Intersection Method Definitions
BSDF *Intersection::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Ray &ray) const
{
	DifferentialGeometry dgShading;
	primitive->GetShadingGeometry(ObjectToWorld, dg,
		&dgShading);
	material->GetShadingGeometry(sw, dg.nn, &dgShading);
	return material->GetBSDF(arena, sw, *this, dgShading);
}

bool Intersection::Le(const Sample &sample, const Ray &ray,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *L) const
{
	if (arealight)
		return arealight->L(sample, ray, dg, bsdf, pdf, pdfDirect, L);
	return false;
}

// AreaLightPrimitive Method Definitions
void AreaLightPrimitive::Refine(vector<boost::shared_ptr<Primitive> > &refined,
	const PrimitiveRefinementHints& refineHints,
	const boost::shared_ptr<Primitive> &thisPtr)
{
	// Refine the decorated primitive and add an arealight decorator to each result
	vector<boost::shared_ptr<Primitive> > tmpRefined;
	prim->Refine(tmpRefined, refineHints, prim);
	for (u_int i = 0; i < tmpRefined.size(); ++i) {
		boost::shared_ptr<Primitive> currPrim(
			new AreaLightPrimitive(tmpRefined[i], areaLight));
		refined.push_back(currPrim);
	}
}

bool AreaLightPrimitive::Intersect(const Ray &r, Intersection *in) const
{
	if (!prim->Intersect(r, in))
		return false;
	in->arealight = areaLight.get(); // set the intersected arealight
	return true;
}

// InstancePrimitive Method Definitions
bool InstancePrimitive::Intersect(const Ray &r, Intersection *isect) const
{
	Ray ray(Inverse(InstanceToWorld) * r);
	if (!instance->Intersect(ray, isect))
		return false;
	r.maxt = ray.maxt;
	isect->ObjectToWorld = InstanceToWorld * isect->ObjectToWorld;
	// Transform instance's differential geometry to world space
	isect->dg *= InstanceToWorld;
	isect->dg.handle = this;
	isect->primitive = this;
	if (material)
		isect->material = material.get();
	if (exterior)
		isect->exterior = exterior.get();
	if (interior)
		isect->interior = interior.get();
	return true;
}

bool InstancePrimitive::IntersectP(const Ray &r) const {
	return instance->IntersectP(Inverse(InstanceToWorld) * r);
}

void InstancePrimitive::GetShadingGeometry(const Transform &obj2world,
	const DifferentialGeometry &dg, DifferentialGeometry *dgShading) const {
	// Transform instance's differential geometry to world space
	DifferentialGeometry dgl(Inverse(obj2world) * dg);

	dg.ihandle->GetShadingGeometry(obj2world, dgl, dgShading);
	*dgShading *= obj2world;
	dgShading->handle = this;
	dgShading->ihandle = dg.ihandle;
}

void InstancePrimitive::GetShadingInformation(const DifferentialGeometry &dgShading,
		RGBColor *color, float *alpha) const {
	dgShading.ihandle->GetShadingInformation(dgShading, color, alpha);
}

// MotionPrimitive Method Definitions
bool MotionPrimitive::Intersect(const Ray &r, Intersection *isect) const
{
	Transform InstanceToWorld = Transform(motionPath.Sample(r.time));

	Ray ray(Inverse(InstanceToWorld) * r);
	if (!instance->Intersect(ray, isect))
		return false;
	r.maxt = ray.maxt;
	isect->ObjectToWorld = InstanceToWorld * isect->ObjectToWorld;
	// Transform instance's differential geometry to world space
	isect->dg *= InstanceToWorld;
	isect->dg.handle = this;
	isect->primitive = this;
	if (material)
		isect->material = material.get();
	if (exterior)
		isect->exterior = exterior.get();
	if (interior)
		isect->interior = interior.get();
	return true;
}

bool MotionPrimitive::IntersectP(const Ray &r) const
{
	Transform InstanceToWorld =  Transform(motionPath.Sample(r.time));

	return instance->IntersectP(Inverse(InstanceToWorld) * r);
}

void MotionPrimitive::GetShadingGeometry(const Transform &obj2world,
	const DifferentialGeometry &dg, DifferentialGeometry *dgShading) const {
	// Transform instance's differential geometry to world space
	DifferentialGeometry dgl(Inverse(obj2world) * dg);

	dg.ihandle->GetShadingGeometry(obj2world, dgl, dgShading);
	*dgShading *= obj2world;
	dgShading->handle = this;
	dgShading->ihandle = dg.ihandle;
}

void MotionPrimitive::GetShadingInformation(const DifferentialGeometry &dgShading,
		RGBColor *color, float *alpha) const {
	dgShading.ihandle->GetShadingInformation(dgShading, color, alpha);
}

BBox MotionPrimitive::WorldBound() const
{
	return motionPath.Bound(instance->WorldBound(), false);
}
