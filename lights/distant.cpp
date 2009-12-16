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

// distant.cpp*
#include "distant.h"
#include "mc.h"
#include "specularreflection.h"
#include "fresnelnoop.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// DistantLight Method Definitions
DistantLight::DistantLight(const Transform &light2world,
						   const boost::shared_ptr< Texture<SWCSpectrum> > L, 
						   float g, const Vector &dir)
	: Light(light2world) {
	lightDir = Normalize(LightToWorld(dir));
	Lbase = L;
	Lbase->SetIlluminant();
	gain = g;
}
DistantLight::~DistantLight()
{
}
SWCSpectrum DistantLight::Le(const TsPack *tspack, const Scene *scene, const Ray &r,
	const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const
{
	*bsdf = NULL;
	*pdf = 0.f;
	*pdfDirect = 0.f;
	return SWCSpectrum(0.f);
}
SWCSpectrum DistantLight::Sample_L(const TsPack *tspack, const Point &p, float u1, float u2, float u3,
		Vector *wi, float *pdf, VisibilityTester *visibility) const {
	*pdf = 1.f;
	*wi = lightDir;
	visibility->SetRay(p, *wi, tspack->time);
	return Lbase->Evaluate(tspack, dummydg) * gain;
}
float DistantLight::Pdf(const TsPack *tspack, const Point &, const Vector &) const {
	return 0.f;
}
float DistantLight::Pdf(const TsPack *tspack, const Point &p, const Normal &n,
	const Point &po, const Normal &ns) const
{
	return 0.f;
}
SWCSpectrum DistantLight::Sample_L(const TsPack *tspack, const Scene *scene,
		float u1, float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
	// Choose point on disk oriented toward infinite light direction
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter,
	                                   &worldRadius);
	Vector v1, v2;
	CoordinateSystem(lightDir, &v1, &v2);
	float d1, d2;
	ConcentricSampleDisk(u1, u2, &d1, &d2);
	Point Pdisk =
		worldCenter + worldRadius * (d1 * v1 + d2 * v2);
	// Set ray origin and direction for infinite light ray
	ray->o = Pdisk + worldRadius * lightDir;
	ray->d = -lightDir;
	*pdf = 1.f / (M_PI * worldRadius * worldRadius);
	return Lbase->Evaluate(tspack, dummydg) * gain;
}

bool DistantLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const
{
	Vector x, y;
	CoordinateSystem(lightDir, &x, &y);
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);

	Point ps;
	Normal ns(-lightDir);
	if (!havePortalShape) {
		float d1, d2;
		ConcentricSampleDisk(u1, u2, &d1, &d2);
		ps = worldCenter + worldRadius * (lightDir + d1 * x + d2 * y);
		*pdf = 1.f / (M_PI * worldRadius * worldRadius);
	} else  {
		// Choose a random portal
		u_int shapeIndex = 0;
		if (nrPortalShapes > 1) {
			u3 *= nrPortalShapes;
			shapeIndex = min(nrPortalShapes - 1, Floor2UInt(u3));
			u3 -= shapeIndex;
		}

		DifferentialGeometry dg;
		dg.time = tspack->time;
		PortalShapes[shapeIndex]->Sample(u1, u2, u3, &dg);
		ps = dg.p;
		const float cosPortal = Dot(ns, dg.nn);
		if (cosPortal <= 0.f) {
			*Le = SWCSpectrum(0.f);
			return false;
		}

		*pdf = PortalShapes[shapeIndex]->Pdf(ps) / cosPortal;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			if (i == shapeIndex)
				continue;
			Intersection isect;
			RayDifferential ray(ps, lightDir);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect)) {
				float cosP = Dot(ns, isect.dg.nn);
				if (cosP > 0.f)
					*pdf += PortalShapes[i]->Pdf(isect.dg.p) / cosP;
			}
		}
		*pdf /= nrPortalShapes;
		if (!(*pdf > 0.f)) {
			*Le = SWCSpectrum(0.f);
			return false;
		}

		ps += (worldRadius + Dot(worldCenter - ps, lightDir)) * lightDir;
	}

	DifferentialGeometry dg(ps, ns, -x, y, Normal(0, 0, 0), Normal(0, 0, 0),
		0, 0, NULL);
	*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns,
		ARENA_ALLOC(tspack->arena, SpecularReflection)(SWCSpectrum(1.f),
		ARENA_ALLOC(tspack->arena, FresnelNoOp)(), 0.f, 0.f));

	*Le = Lbase->Evaluate(tspack, dg) * gain;
	return true;
}

bool DistantLight::Sample_L(const TsPack *tspack, const Scene *scene,
	const Point &p, const Normal &n, float u1, float u2, float u3,
	BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility, SWCSpectrum *Le) const
{
	Vector x, y;
	CoordinateSystem(lightDir, &x, &y);
	*pdfDirect = 1.f;

	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	Vector toCenter(worldCenter - p);
	float approach = Dot(toCenter, lightDir);
	float distance = approach + worldRadius;
	Point ps(p + distance * lightDir);
	Normal ns(-lightDir);

	DifferentialGeometry dg(ps, ns, -x, y, Normal(0, 0, 0), Normal(0, 0, 0),
		0, 0, NULL);
	*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns,
		ARENA_ALLOC(tspack->arena, SpecularReflection)(SWCSpectrum(1.f),
		ARENA_ALLOC(tspack->arena, FresnelNoOp)(), 0.f, 0.f));
	if (!havePortalShape)
		*pdf = 1.f / (M_PI * worldRadius * worldRadius);
	else {
		*pdf = 0.f;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			Intersection isect;
			RayDifferential ray(ps, lightDir);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect)) {
				float cosPortal = Dot(ns, isect.dg.nn);
				if (cosPortal > 0.f)
					*pdf += PortalShapes[i]->Pdf(isect.dg.p) / cosPortal;
			}
		}
		*pdf /= nrPortalShapes;
	}
	visibility->SetSegment(p, ps, tspack->time);

	*Le = Lbase->Evaluate(tspack, dg) * gain;
	return true;
}

Light* DistantLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet, const TextureParams &tp) {
	boost::shared_ptr<Texture<SWCSpectrum> > L = tp.GetSWCSpectrumTexture("L", RGBColor(1.f));
	float g = paramSet.FindOneFloat("gain", 1.f);
	Point from = paramSet.FindOnePoint("from", Point(0,0,0));
	Point to = paramSet.FindOnePoint("to", Point(0,0,1));
	Vector dir = from-to;
	DistantLight *l = new DistantLight(light2world, L, g, dir);
	l->hints.InitParam(paramSet);
	return l;
}

static DynamicLoader::RegisterLight<DistantLight> r("distant");

