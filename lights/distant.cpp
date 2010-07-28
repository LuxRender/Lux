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
#include "memory.h"
#include "color.h"
#include "mc.h"
#include "bxdf.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

class DistantBxDF : public BxDF
{
public:
	DistantBxDF(float sin2Max, float cosMax) : BxDF(BxDFType(BSDF_REFLECTION |
		BSDF_GLOSSY)), sin2ThetaMax(sin2Max), cosThetaMax(cosMax),
		conePdf(UniformConePdf(cosMax)) { }
	virtual ~DistantBxDF() { }
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi,
		SWCSpectrum *const f) const {
		if (wi.z <= 0.f || (wi.x * wi.x + wi.y * wi.y) > sin2ThetaMax)
			return;
		*f += SWCSpectrum(1.f / conePdf);
	}
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,float *pdf,
		float *pdfBack = NULL, bool reverse = false) const {
		*wi = UniformSampleCone(u1, u2, cosThetaMax);
		*pdf = conePdf;
		if (pdfBack)
			*pdfBack = 0.f;
		*f = SWCSpectrum(1.f / conePdf);
		return true;
	}
	virtual float Pdf(const TsPack *tspack, const Vector &wi,
		const Vector &wo) const {
		if (wo.z <= 0.f || (wo.x * wo.x + wo.y * wo.y) > sin2ThetaMax)
			return 0.f;
		else
			return conePdf;
	}
private:
	float sin2ThetaMax, cosThetaMax, conePdf;
};

// DistantLight Method Definitions
DistantLight::DistantLight(const Transform &light2world,
	const boost::shared_ptr<Texture<SWCSpectrum> > &L, 
	float g, float theta, const Vector &dir)
	: Light(light2world), Lbase(L) {
	lightDir = Normalize(LightToWorld(dir));
	CoordinateSystem(lightDir, &x, &y);
	Lbase->SetIlluminant();
	gain = g;
	if (theta == 0.f) {
		sin2ThetaMax = 2.f * MachineEpsilon::E(1.f);
		cosThetaMax = 1.f - MachineEpsilon::E(1.f);
	} else {
		sin2ThetaMax = sinf(theta) * sinf(theta);
		cosThetaMax = cosf(theta);
	}
	bxdf = new DistantBxDF(sin2ThetaMax, cosThetaMax);
}

DistantLight::~DistantLight()
{
	delete bxdf;
}

bool DistantLight::Le(const TsPack *tspack, const Scene *scene, const Ray &r,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *L) const
{
	const float xD = Dot(r.d, x);
	const float yD = Dot(r.d, y);
	const float cosRay = Dot(r.d, lightDir);
	if (cosRay <= 0.f || (xD * xD + yD * yD) > sin2ThetaMax)
		return false;
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	Vector toCenter(worldCenter - r.o);
	float approach = Dot(toCenter, lightDir);
	float distance = (approach + worldRadius) / cosRay;
	Point ps(r.o + distance * r.d);
	Normal ns(-lightDir);
	DifferentialGeometry dg(ps, ns, -x, y, Normal(0, 0, 0), Normal(0, 0, 0),
		0, 0, NULL);
	*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns, bxdf, NULL, NULL);
	if (pdf) {
		if (!havePortalShape)
			*pdf = 1.f / (M_PI * worldRadius * worldRadius);
		else {
			*pdf = 0.f;
			for (u_int i = 0; i < nrPortalShapes; ++i) {
				Intersection isect;
				RayDifferential ray(ps, lightDir);
				ray.mint = -INFINITY;
				if (PortalShapes[i]->Intersect(ray, &isect)) {
					float cosPortal = -Dot(lightDir, isect.dg.nn);
					if (cosPortal > 0.f)
						*pdf += PortalShapes[i]->Pdf(isect.dg.p) / cosPortal;
				}
			}
			*pdf /= nrPortalShapes;
		}
	}
	if (pdfDirect)
		*pdfDirect = UniformConePdf(cosThetaMax) * fabsf(cosRay) /
		(distance * distance);
	*L *= Lbase->Evaluate(tspack, dg) * gain * UniformConePdf(cosThetaMax);
	return true;
}

float DistantLight::Pdf(const TsPack *tspack, const Point &p,
	const Point &po, const Normal &ns) const
{
	const Vector w(p - po);
	const float d2 = w.LengthSquared();
	const float cosRay = AbsDot(w, ns) / sqrtf(d2);
	if (cosRay < cosThetaMax)
		return 0.f;
	else
		return UniformConePdf(cosThetaMax) * cosRay / d2;
}

bool DistantLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1,
	float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const
{
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
	*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns, bxdf, NULL, NULL);

	*Le = Lbase->Evaluate(tspack, dg) * gain * UniformConePdf(cosThetaMax);
	return true;
}

bool DistantLight::Sample_L(const TsPack *tspack, const Scene *scene,
	const Point &p, float u1, float u2, float u3,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *Le) const
{
	const Vector wi(UniformSampleCone(u1, u2, cosThetaMax, x, y, lightDir));

	const float cosRay = Dot(wi, lightDir);
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	Vector toCenter(worldCenter - p);
	float approach = Dot(toCenter, lightDir);
	float distance = (approach + worldRadius) / cosRay;
	Point ps(p + distance * wi);
	Normal ns(-lightDir);

	DifferentialGeometry dg(ps, ns, -x, y, Normal(0, 0, 0), Normal(0, 0, 0),
		0, 0, NULL);
	*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns, bxdf, NULL, NULL);
	if (pdf) {
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
	}
	if (pdfDirect)
		*pdfDirect = UniformConePdf(cosThetaMax) * cosRay /
			(distance * distance);

	*Le = Lbase->Evaluate(tspack, dg) * gain * UniformConePdf(cosThetaMax);
	return true;
}

Light* DistantLight::CreateLight(const Transform &light2world,
	const ParamSet &paramSet)
{
	boost::shared_ptr<Texture<SWCSpectrum> > L(paramSet.GetSWCSpectrumTexture("L", RGBColor(1.f)));
	float g = paramSet.FindOneFloat("gain", 1.f);
	float theta = Radians(paramSet.FindOneFloat("theta", 0.f));
	Point from = paramSet.FindOnePoint("from", Point(0, 0, 0));
	Point to = paramSet.FindOnePoint("to", Point(0, 0, 1));
	Vector dir = from - to;
	DistantLight *l = new DistantLight(light2world, L, g, theta, dir);
	l->hints.InitParam(paramSet);
	return l;
}

static DynamicLoader::RegisterLight<DistantLight> r("distant");

