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

// infinite.cpp*
#include "infinite.h"
#include "randomgen.h"
#include "imagereader.h"
#include "mc.h"
#include "paramset.h"
#include "reflection/bxdf.h"
#include "dynload.h"

using namespace lux;

class  InfiniteBSDF : public BSDF  {
public:
	// InfiniteBSDF Public Methods
	InfiniteBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior,
		const InfiniteAreaLight &l, const Transform &WL) :
		BSDF(dgs, ngeom, exterior, interior), light(l),
		WorldToLight(WL) { }
	virtual inline u_int NumComponents() const { return 1; }
	virtual inline u_int NumComponents(BxDFType flags) const {
		return (flags & (BSDF_REFLECTION | BSDF_DIFFUSE)) ==
			(BSDF_REFLECTION | BSDF_DIFFUSE) ? 1U : 0U;
	}
	virtual bool Sample_f(const TsPack *tspack, const Vector &woW, Vector *wiW,
		float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
		BxDFType flags = BSDF_ALL, BxDFType *sampledType = NULL,
		float *pdfBack = NULL, bool reverse = false) const {
		if (reverse || NumComponents(flags) == 0)
			return false;
		*wiW = Normalize(LocalToWorld(CosineSampleHemisphere(u1, u2)));
		if (!(Dot(*wiW, ng) > 0.f))
			return false;
		if (!(Dot(*wiW, nn) > 0.f))
			return false;
		if (sampledType)
			*sampledType = BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE);
		*pdf = AbsDot(*wiW, nn) * INV_PI;
		if (pdfBack)
			*pdfBack = 0.f;
		if (light.radianceMap == NULL) {
			*f_ = SWCSpectrum(INV_PI);
			return true;
		}
		const Vector wh = Normalize(WorldToLight(-(*wiW)));
		float s, t, dummy;
		light.mapping->Map(wh, &s, &t, &dummy);
		*f_ = light.radianceMap->LookupSpectrum(tspack, s, t) *
			INV_PI;
		return true;
	}
	virtual float Pdf(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1 &&
			Dot(wiW, ng) > 0.f && Dot(wiW, nn) > 0.f)
			return AbsDot(wiW, nn) * INV_PI;
		return 0.f;
	}
	virtual SWCSpectrum f(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1 && Dot(wiW, ng) > 0.f) {
			if (light.radianceMap == NULL) {
				return SWCSpectrum(INV_PI);
			}
			const Vector wh = Normalize(WorldToLight(-wiW));
			float s, t, dummy;
			light.mapping->Map(wh, &s, &t, &dummy);
			return light.radianceMap->LookupSpectrum(tspack, s, t) *
				INV_PI;
		}
		return SWCSpectrum(0.f);
	}
	virtual SWCSpectrum rho(const TsPack *tspack,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }
	virtual SWCSpectrum rho(const TsPack *tspack, const Vector &woW,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }

protected:
	// InfiniteBSDF Private Methods
	virtual ~InfiniteBSDF() { }
	const InfiniteAreaLight &light;
	const Transform &WorldToLight;
};
class  InfinitePortalBSDF : public BSDF  {
public:
	// InfinitePortalBSDF Public Methods
	InfinitePortalBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior,
		const InfiniteAreaLight &l, const Transform &WL,
		const Point &p,
		const vector<boost::shared_ptr<Primitive> > &portalList,
		u_int portal) :
		BSDF(dgs, ngeom, exterior, interior), light(l),
		WorldToLight(WL), ps(p), PortalShapes(portalList),
		shapeIndex(portal) { }
	virtual inline u_int NumComponents() const { return 1; }
	virtual inline u_int NumComponents(BxDFType flags) const {
		return (flags & (BSDF_REFLECTION | BSDF_DIFFUSE)) ==
			(BSDF_REFLECTION | BSDF_DIFFUSE) ? 1U : 0U;
	}
	virtual bool Sample_f(const TsPack *tspack, const Vector &woW, Vector *wiW,
		float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
		BxDFType flags = BSDF_ALL, BxDFType *sampledType = NULL,
		float *pdfBack = NULL, bool reverse = false) const {
		if (shapeIndex == ~0U || reverse || NumComponents(flags) == 0)
			return false;
		DifferentialGeometry dg;
		dg.time = tspack->time;
		PortalShapes[shapeIndex]->Sample(tspack, ps, u1, u2, u3, &dg);
		*wiW = Normalize(dg.p - ps);
		if (!(Dot(*wiW, nn) > 0.f))
			return false;
		if (light.radianceMap != NULL) {
			const Vector wh = Normalize(WorldToLight(-(*wiW)));
			float s, t, dummy;
			light.mapping->Map(wh, &s, &t, &dummy);
			*f_ = light.radianceMap->LookupSpectrum(tspack, s, t) *
				INV_PI;
		} else
			*f_ = SWCSpectrum(INV_PI);
		*pdf = PortalShapes[shapeIndex]->Pdf(ps, dg.p) *
			DistanceSquared(ps, dg.p) / AbsDot(*wiW, dg.nn);
		for (u_int i = 0; i < PortalShapes.size(); ++i) {
			if (i == shapeIndex)
				continue;
			Intersection isect;
			RayDifferential ray(ps, *wiW);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect) &&
				Dot(*wiW, isect.dg.nn) > 0.f)
				*pdf += PortalShapes[i]->Pdf(ps, isect.dg.p) *
					DistanceSquared(ps, isect.dg.p) /
					AbsDot(*wiW, isect.dg.nn);
		}
		*pdf /= PortalShapes.size();
		if (pdfBack)
			*pdfBack = 0.f;
		return true;
	}
	virtual float Pdf(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 0 && !(Dot(wiW, nn) > 0.f))
			return 0.f;
		float pdf = 0.f;
		for (u_int i = 0; i < PortalShapes.size(); ++i) {
			Intersection isect;
			RayDifferential ray(ps, wiW);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect) &&
				Dot(wiW, isect.dg.nn) > 0.f)
				pdf += PortalShapes[i]->Pdf(ps, isect.dg.p) *
					DistanceSquared(ps, isect.dg.p) /
					AbsDot(wiW, isect.dg.nn);
		}
		return pdf / PortalShapes.size();
	}
	virtual SWCSpectrum f(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1 && Dot(wiW, ng) > 0.f) {
			if (light.radianceMap == NULL) {
				return SWCSpectrum(INV_PI);
			}
			const Vector wh = Normalize(WorldToLight(-wiW));
			float s, t, dummy;
			light.mapping->Map(wh, &s, &t, &dummy);
			return light.radianceMap->LookupSpectrum(tspack, s, t) *
			INV_PI;
		}
		return SWCSpectrum(0.f);
	}
	virtual SWCSpectrum rho(const TsPack *tspack,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }
	virtual SWCSpectrum rho(const TsPack *tspack, const Vector &woW,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }

protected:
	// InfinitePortalBSDF Private Methods
	virtual ~InfinitePortalBSDF() { }
	const InfiniteAreaLight &light;
	const Transform &WorldToLight;
	Point ps;
	const vector<boost::shared_ptr<Primitive> > &PortalShapes;
	u_int shapeIndex;
};

// InfiniteAreaLight Method Definitions
InfiniteAreaLight::~InfiniteAreaLight()
{
	delete radianceMap;
	delete mapping;
}

InfiniteAreaLight::InfiniteAreaLight(const Transform &light2world,
	const RGBColor &l, u_int ns, const string &texmap,
	EnvironmentMapping *m, float gain, float gamma)
	: Light(light2world, ns), SPDbase(l)
{
	// Base illuminant SPD
	SPDbase.Scale(gain);

	mapping = m;
	radianceMap = NULL;
	if (texmap != "") {
		auto_ptr<ImageData> imgdata(ReadImage(texmap));
		if (imgdata.get() != NULL) {
			radianceMap = imgdata->createMIPMap(BILINEAR, 8.f,
				TEXTURE_REPEAT, 1.f, gamma);
		} else
			radianceMap = NULL;
	}
}

float InfiniteAreaLight::Power(const Scene *scene) const
{
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	float power = SPDbase.Y() * M_PI * 4.f * M_PI * worldRadius * worldRadius;
	if (radianceMap != NULL)
		power *= radianceMap->LookupFloat(CHANNEL_MEAN, .5f, .5f, .5f);
	return power;
}

SWCSpectrum InfiniteAreaLight::Le(const TsPack *tspack,
	const RayDifferential &r) const
{
	// Compute infinite light radiance for direction
	if (radianceMap != NULL) {
		Vector wh = Normalize(WorldToLight(r.d));
		float s, t, dummy;
		mapping->Map(wh, &s, &t, &dummy);
		return SWCSpectrum(tspack, SPDbase) *
			radianceMap->LookupSpectrum(tspack, s, t);
	} else
		return SWCSpectrum(tspack, SPDbase);
}

SWCSpectrum InfiniteAreaLight::Le(const TsPack *tspack, const Scene *scene,
	const Ray &r, const Normal &n, BSDF **bsdf, float *pdf,
	float *pdfDirect) const
{
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	const Vector toCenter(worldCenter - r.o);
	const float centerDistance = Dot(toCenter, toCenter);
	const float approach = Dot(toCenter, r.d);
	const float distance = approach + sqrtf(worldRadius * worldRadius -
		centerDistance + approach * approach);
	const Point ps(r.o + distance * r.d);
	const Normal ns(Normalize(worldCenter - ps));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Normal(0, 0, 0),
		Normal(0, 0, 0), 0, 0, NULL);
	dg.time = tspack->time;
	if (!havePortalShape) {
		*bsdf = ARENA_ALLOC(tspack->arena, InfiniteBSDF)(dg, ns,
			NULL, NULL, *this, WorldToLight);
		*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
		*pdfDirect = AbsDot(r.d, n) * INV_TWOPI * AbsDot(r.d, ns) /
			DistanceSquared(r.o, ps);
	} else {
		*bsdf = ARENA_ALLOC(tspack->arena, InfinitePortalBSDF)(dg, ns,
			NULL, NULL, *this, WorldToLight, ps, PortalShapes,
			~0U);
		*pdf = 0.f;
		*pdfDirect = 0.f;
		DifferentialGeometry dgs;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			PortalShapes[i]->Sample(.5f, .5f, .5f, &dgs);
			const Vector w(dgs.p - ps);
			if (Dot(w, dgs.nn) > 0.f) {
				const float distance = w.LengthSquared();
				*pdf += AbsDot(ns, w) /
					(sqrtf(distance) * distance);
			}
			Intersection isect;
			RayDifferential ray(r);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect) &&
				Dot(r.d, isect.dg.nn) < 0.f)
				*pdfDirect += PortalShapes[i]->Pdf(r.o,
					isect.dg.p) * DistanceSquared(r.o,
					isect.dg.p) / DistanceSquared(r.o, ps) *
					AbsDot(r.d, ns) / AbsDot(r.d,
					isect.dg.nn);
		}
		*pdf *= INV_TWOPI / nrPortalShapes;
		*pdfDirect /= nrPortalShapes;
	}
	if (radianceMap != NULL) {
		const Vector wh = Normalize(WorldToLight(r.d));
		float s, t, dummy;
		mapping->Map(wh, &s, &t, &dummy);
		return SWCSpectrum(tspack, SPDbase) *
			radianceMap->LookupSpectrum(tspack, s, t);
	} else
		return SWCSpectrum(tspack, SPDbase);
}

SWCSpectrum InfiniteAreaLight::Sample_L(const TsPack *tspack, const Point &p,
	const Normal &n, float u1, float u2, float u3, Vector *wi, float *pdf,
	VisibilityTester *visibility) const
{
	if(!havePortalShape) {
		// Sample cosine-weighted direction on unit sphere
		float x, y, z;
		ConcentricSampleDisk(u1, u2, &x, &y);
		z = sqrtf(max(0.f, 1.f - x*x - y*y));
		if (u3 < .5) z *= -1;
		*wi = Vector(x, y, z);
		// Compute _pdf_ for cosine-weighted infinite light direction
		*pdf = fabsf(wi->z) * INV_TWOPI;
		// Transform direction to world space
		Vector v1, v2;
		CoordinateSystem(Normalize(Vector(n)), &v1, &v2);
		*wi = v1 * wi->x + v2 * wi->y + Vector(n) * wi->z;
	} else {
		// Sample Portal
		u_int shapeidx = 0;
		if(nrPortalShapes > 1) 
			shapeidx = min(nrPortalShapes - 1U,
				Floor2UInt(u3 * nrPortalShapes));
		DifferentialGeometry dg;
		dg.time = tspack->time;
		Point ps;
		bool found = false;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			PortalShapes[shapeidx]->Sample(tspack, p, u1, u2, u3,
				&dg);
			ps = dg.p;
			*wi = Normalize(ps - p);
			if (Dot(*wi, dg.nn) < 0.f) {
				found = true;
				break;
			}

			if (++shapeidx >= nrPortalShapes)
				shapeidx = 0;
		}

		if (found)
			*pdf = PortalShapes[shapeidx]->Pdf(p, *wi);
		else {
			*pdf = 0.f;
			return SWCSpectrum(0.f);
		}
	}
	visibility->SetRay(p, *wi, tspack->time);
	return Le(tspack, RayDifferential(p, *wi));
}
float InfiniteAreaLight::Pdf(const TsPack *tspack, const Point &,
	const Normal &n, const Vector &wi) const
{
	return AbsDot(n, wi) * INV_TWOPI;
}
float InfiniteAreaLight::Pdf(const TsPack *tspack, const Point &p,
	const Normal &n, const Point &po, const Normal &ns) const
{
	const Vector wi(po - p);
	if (!havePortalShape) {
		const float d2 = wi.LengthSquared();
		return AbsDot(n, wi) * INV_TWOPI * AbsDot(wi, ns) / (d2 * d2);
	} else {
		float pdf = 0.f;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			Intersection isect;
			RayDifferential ray(p, wi);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect) &&
				Dot(wi, isect.dg.nn) < 0.f)
				pdf += PortalShapes[i]->Pdf(p, isect.dg.p) *
					DistanceSquared(p, isect.dg.p) /
					DistanceSquared(p, po) *
					AbsDot(wi, ns) /
					AbsDot(wi, isect.dg.nn);
		}
		pdf /= nrPortalShapes;
		return pdf;
	}
}
SWCSpectrum InfiniteAreaLight::Sample_L(const TsPack *tspack, const Point &p,
	float u1, float u2, float u3, Vector *wi, float *pdf,
	VisibilityTester *visibility) const
{
	if(!havePortalShape) {
		*wi = UniformSampleSphere(u1, u2);
		*pdf = UniformSpherePdf();
	} else {
	    // Sample a random Portal
		u_int shapeidx = 0;
		if(nrPortalShapes > 1) 
			shapeidx = min(nrPortalShapes - 1U,
				Floor2UInt(u3 * nrPortalShapes));
		DifferentialGeometry dg;
		dg.time = tspack->time;
		Point ps;
		bool found = false;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			PortalShapes[shapeidx]->Sample(tspack, p, u1, u2, u3,
				&dg);
			ps = dg.p;
			*wi = Normalize(ps - p);
			if (Dot(*wi, dg.nn) < 0.f) {
				found = true;
				break;
			}

			if (++shapeidx >= nrPortalShapes)
				shapeidx = 0;
		}

		if (found)
			*pdf = PortalShapes[shapeidx]->Pdf(p, *wi);
		else {
			*pdf = 0.f;
			return SWCSpectrum(0.f);
		}
	}
	visibility->SetRay(p, *wi, tspack->time);
	return Le(tspack, RayDifferential(p, *wi));
}

float InfiniteAreaLight::Pdf(const TsPack *tspack, const Point &,
	const Vector &) const
{
	return 1.f / (4.f * M_PI);
}

SWCSpectrum InfiniteAreaLight::Sample_L(const TsPack *tspack,
	const Scene *scene, float u1, float u2, float u3, float u4,
	Ray *ray, float *pdf) const
{
	if(!havePortalShape) {
		// Choose two points _p1_ and _p2_ on scene bounding sphere
		Point worldCenter;
		float worldRadius;
		scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
		worldRadius *= 1.01f;
		Point p1 = worldCenter + worldRadius *
			UniformSampleSphere(u1, u2);
		Point p2 = worldCenter + worldRadius *
			UniformSampleSphere(u3, u4);
		// Construct ray between _p1_ and _p2_
		ray->o = p1;
		ray->d = Normalize(p2-p1);
		// Compute _InfiniteAreaLight_ ray weight
		Vector to_center = Normalize(worldCenter - p1);
		float costheta = AbsDot(to_center,ray->d);
		*pdf = costheta / ((4.f * M_PI * worldRadius * worldRadius));
	} else {
		// Dade - choose a random portal. This strategy is quite bad if there
		// is more than one portal.
		u_int shapeidx = 0;
		if (nrPortalShapes > 1) 
			shapeidx = min(nrPortalShapes - 1,
				Floor2UInt(tspack->rng->floatValue() *
				nrPortalShapes));  // TODO - REFACT - add passed value from sample

		DifferentialGeometry dg;
		dg.time = tspack->time;
		PortalShapes[shapeidx]->Sample(u1, u2,
			tspack->rng->floatValue(), &dg); // TODO - REFACT - add passed value from sample
		ray->o = dg.p;
		ray->d = UniformSampleSphere(u3, u4);
		if (Dot(ray->d, dg.nn) < 0.f)
			ray->d *= -1;

		*pdf = PortalShapes[shapeidx]->Pdf(ray->o) * INV_TWOPI /
			nrPortalShapes;
	}

	return Le(tspack, RayDifferential(ray->o, -ray->d));
}
bool InfiniteAreaLight::Sample_L(const TsPack *tspack, const Scene *scene,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	SWCSpectrum *Le) const
{
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	if (!havePortalShape) {
		const Point ps = worldCenter +
			worldRadius * UniformSampleSphere(u1, u2);
		const Normal ns = Normal(Normalize(worldCenter - ps));
		Vector dpdu, dpdv;
		CoordinateSystem(Vector(ns), &dpdu, &dpdv);
		DifferentialGeometry dg(ps, ns, dpdu, dpdv, Normal(0, 0, 0),
			Normal (0, 0, 0), 0, 0, NULL);
		*bsdf = ARENA_ALLOC(tspack->arena, InfiniteBSDF)(dg, ns,
			NULL, NULL, *this, WorldToLight);
		*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	} else {
		// Sample a random Portal
		u_int shapeIndex = 0;
		if (nrPortalShapes > 1) {
			u3 *= nrPortalShapes;
			shapeIndex = min(nrPortalShapes - 1U, Floor2UInt(u3));
			u3 -= shapeIndex;
		}
		DifferentialGeometry dgs;
		dgs.time = tspack->time;
		PortalShapes[shapeIndex]->Sample(.5f, .5f, .5f, &dgs);
		Vector wi(UniformSampleHemisphere(u1, u2));
		wi = Normalize(wi.x * Normalize(dgs.dpdu) +
			wi.y * Normalize(dgs.dpdv) - wi.z * Vector(dgs.nn));
		const Vector toCenter(worldCenter - dgs.p);
		const float centerDistance = Dot(toCenter, toCenter);
		const float approach = Dot(toCenter, wi);
		const float distance = approach +
			sqrtf(worldRadius * worldRadius - centerDistance +
			approach * approach);
		const Point ps(dgs.p + distance * wi);
		const Normal ns(Normalize(worldCenter - ps));
		Vector dpdu, dpdv;
		CoordinateSystem(Vector(ns), &dpdu, &dpdv);
		DifferentialGeometry dg(ps, ns, dpdu, dpdv, Normal(0, 0, 0),
			Normal(0, 0, 0), 0, 0, NULL);
		*bsdf = ARENA_ALLOC(tspack->arena, InfinitePortalBSDF)(dg, ns,
			NULL, NULL, *this, WorldToLight, ps, PortalShapes,
			shapeIndex);
		*pdf = AbsDot(ns, wi) / (distance * distance);
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			if (i == shapeIndex)
				continue;
			PortalShapes[i]->Sample(.5f, .5f, .5f, &dgs);
			wi = ps - dgs.p;
			if (Dot(wi, dgs.nn) < 0.f) {
				const float d2 = wi.LengthSquared();
				*pdf += AbsDot(ns, wi) /
					(sqrtf(d2) * d2);
			}
		}
		*pdf *= INV_TWOPI / nrPortalShapes;
	}
	*Le = SWCSpectrum(tspack, SPDbase) * M_PI;
	return true;
}
bool InfiniteAreaLight::Sample_L(const TsPack *tspack, const Scene *scene,
	const Point &p, const Normal &n, float u1, float u2, float u3,
	BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility, SWCSpectrum *Le) const
{
	Vector wi;
	u_int shapeIndex = 0;
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	if(!havePortalShape) {
		// Sample cosine-weighted direction on unit sphere
		wi = CosineSampleHemisphere(u1, u2);
		// Compute _pdf_ for cosine-weighted infinite light direction
		*pdfDirect = fabsf(wi.z) * INV_PI;
		// Transform direction to world space
		Vector v1, v2;
		CoordinateSystem(Normalize(Vector(n)), &v1, &v2);
		wi = Vector(wi.x * v1 + wi.y * v2 + wi.z * Vector(n));
	} else {
		// Sample a random Portal
		if (nrPortalShapes > 1) {
			u3 *= nrPortalShapes;
			shapeIndex = min(nrPortalShapes - 1, Floor2UInt(u3));
			u3 -= shapeIndex;
		}
		DifferentialGeometry dg;
		dg.time = tspack->time;
		PortalShapes[shapeIndex]->Sample(tspack, p, u1, u2, u3, &dg);
		Point ps = dg.p;
		wi = Normalize(ps - p);
		if (Dot(wi, dg.nn) < 0.f) {
			*pdfDirect = PortalShapes[shapeIndex]->Pdf(p, ps);
			*pdfDirect *= DistanceSquared(p, dg.p) /
				AbsDot(wi, dg.nn);
		} else {
			*Le = 0.f;
			return false;
		}
	}
	const Vector toCenter(worldCenter - p);
	const float centerDistance = Dot(toCenter, toCenter);
	const float approach = Dot(toCenter, wi);
	const float distance = approach + sqrtf(worldRadius * worldRadius -
		centerDistance + approach * approach);
	const Point ps(p + distance * wi);
	const Normal ns(Normalize(worldCenter - ps));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Normal(0, 0, 0),
		Normal (0, 0, 0), 0, 0, NULL);
	if (!havePortalShape) {
		*bsdf = ARENA_ALLOC(tspack->arena, InfiniteBSDF)(dg, ns,
			NULL, NULL, *this, WorldToLight);
		*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	} else {
		*bsdf = ARENA_ALLOC(tspack->arena, InfinitePortalBSDF)(dg, ns,
			NULL, NULL, *this, WorldToLight, ps, PortalShapes,
			shapeIndex);
		*pdf = 0.f;
		DifferentialGeometry dgs;
		dgs.time = tspack->time;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			PortalShapes[i]->Sample(.5f, .5f, .5f, &dgs);
			Vector w(ps - dgs.p);
			if (Dot(wi, dgs.nn) < 0.f) {
				float distance = w.LengthSquared();
				*pdf += AbsDot(ns, w) / (sqrtf(distance) * distance);
			}
		}
		*pdf *= INV_TWOPI / nrPortalShapes;
	}
	*pdfDirect *= AbsDot(wi, ns) / (distance * distance);
	if (havePortalShape) {
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			if (i == shapeIndex)
				continue;
			Intersection isect;
			RayDifferential ray(p, wi);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect) &&
				Dot(wi, isect.dg.nn) < 0.f)
				*pdfDirect += PortalShapes[i]->Pdf(p,
					isect.dg.p) * DistanceSquared(p,
					isect.dg.p) / DistanceSquared(p, ps) *
					AbsDot(wi, ns) / AbsDot(wi,
					isect.dg.nn);
		}
		*pdfDirect /= nrPortalShapes;
	}
	visibility->SetSegment(p, ps, tspack->time);
	*Le = SWCSpectrum(tspack, SPDbase) * M_PI;
	return true;
}

Light* InfiniteAreaLight::CreateLight(const Transform &light2world,
	const ParamSet &paramSet)
{
	RGBColor L = paramSet.FindOneRGBColor("L", RGBColor(1.0));
	string texmap = paramSet.FindOneString("mapname", "");
	int nSamples = paramSet.FindOneInt("nsamples", 1);

	EnvironmentMapping *map = NULL;
	string type = paramSet.FindOneString("mapping", "");
	if (type == "" || type == "latlong") {
		map = new LatLongMapping();
	}
	else if (type == "angular") map = new AngularMapping();
	else if (type == "vcross") map = new VerticalCrossMapping();

	// Initialize _ImageTexture_ parameters
	float gain = paramSet.FindOneFloat("gain", 1.0f);
	float gamma = paramSet.FindOneFloat("gamma", 1.0f);

	InfiniteAreaLight *l =  new InfiniteAreaLight(light2world, L, nSamples, texmap, map, gain, gamma);
	l->hints.InitParam(paramSet);
	return l;
}

static DynamicLoader::RegisterLight<InfiniteAreaLight> r("infinite");

