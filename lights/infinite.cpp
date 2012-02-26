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
#include "imagereader.h"
#include "mc.h"
#include "paramset.h"
#include "bxdf.h"
#include "singlebsdf.h"
#include "primitive.h"
#include "sampling.h"
#include "scene.h"
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
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &woW,
		Vector *wiW, float u1, float u2, float u3,
		SWCSpectrum *const f_, float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL, float *pdfBack = NULL,
		bool reverse = false) const {
		if (reverse || NumComponents(flags) == 0)
			return false;
		const Vector w(CosineSampleHemisphere(u1, u2));
		const float cosi = w.z;
		const Vector wi(w.x * dgShading.dpdu + w.y * dgShading.dpdv +
			w.z * Vector(dgShading.nn));
		*wiW = Normalize(WorldToLight.GetInverse()(wi));
		if (sampledType)
			*sampledType = BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE);
		*pdf = cosi * INV_PI;
		if (pdfBack)
			*pdfBack = 0.f;
		if (light.radianceMap == NULL) {
			*f_ = SWCSpectrum(1.f);
			return true;
		}
		float s, t, dummy;
		light.mapping->Map(Normalize(-wi), &s, &t, &dummy);
		*f_ = light.radianceMap->LookupSpectrum(sw, s, t);
		return true;
	}
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1 &&
			Dot(wiW, ng) > 0.f && Dot(wiW, dgShading.nn) > 0.f)
			return AbsDot(wiW, dgShading.nn) * INV_PI;
		return 0.f;
	}
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse, BxDFType flags = BSDF_ALL) const {
		const float cosi = Dot(wiW, ng);
		if (NumComponents(flags) == 1 && cosi > 0.f) {
			if (light.radianceMap == NULL) {
				return SWCSpectrum(reverse ? INV_PI : INV_PI * cosi);
			}
			const Vector wh = Normalize(WorldToLight(-wiW));
			float s, t, dummy;
			light.mapping->Map(wh, &s, &t, &dummy);
			return light.radianceMap->LookupSpectrum(sw, s, t) *
				(reverse ? INV_PI : INV_PI * cosi);
		}
		return SWCSpectrum(0.f);
	}
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		const Vector &woW, BxDFType flags = BSDF_ALL) const {
		return SWCSpectrum(1.f);
	}

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
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &woW,
		Vector *wiW, float u1, float u2, float u3,
		SWCSpectrum *const f_, float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL, float *pdfBack = NULL,
		bool reverse = false) const {
		if (shapeIndex == ~0U || reverse || NumComponents(flags) == 0)
			return false;
		DifferentialGeometry dg;
		dg.time = dgShading.time;
		*pdf = PortalShapes[shapeIndex]->Sample(ps, u1, u2, u3, &dg);
		if (!(*pdf > 0.f))
			return false;
		*wiW = Normalize(dg.p - ps);
		const float cosi = Dot(*wiW, ng);
		if (!(cosi > 0.f))
			return false;
		*pdf *= DistanceSquared(ps, dg.p) / AbsDot(*wiW, dg.nn);
		for (u_int i = 0; i < PortalShapes.size(); ++i) {
			if (i == shapeIndex)
				continue;
			Intersection isect;
			Ray ray(ps, *wiW);
			ray.mint = -INFINITY;
			ray.time = dgShading.time;
			if (PortalShapes[i]->Intersect(ray, &isect) &&
				Dot(*wiW, isect.dg.nn) > 0.f)
				*pdf += PortalShapes[i]->Pdf(ps, isect.dg) *
					DistanceSquared(ps, isect.dg.p) /
					AbsDot(*wiW, isect.dg.nn);
		}
		*pdf /= PortalShapes.size();
		if (pdfBack)
			*pdfBack = 0.f;
		if (light.radianceMap != NULL) {
			const Vector wh = Normalize(WorldToLight(-(*wiW)));
			float s, t, dummy;
			light.mapping->Map(wh, &s, &t, &dummy);
			*f_ = light.radianceMap->LookupSpectrum(sw, s, t) *
				(INV_PI * cosi / *pdf);
		} else
			*f_ = SWCSpectrum(INV_PI * cosi / *pdf);
		return true;
	}
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 0 && !(Dot(wiW, dgShading.nn) > 0.f))
			return 0.f;
		float pdf = 0.f;
		for (u_int i = 0; i < PortalShapes.size(); ++i) {
			Intersection isect;
			Ray ray(ps, wiW);
			ray.mint = -INFINITY;
			ray.time = dgShading.time;
			if (PortalShapes[i]->Intersect(ray, &isect) &&
				Dot(wiW, isect.dg.nn) > 0.f)
				pdf += PortalShapes[i]->Pdf(ps, isect.dg) *
					DistanceSquared(ps, isect.dg.p) /
					AbsDot(wiW, isect.dg.nn);
		}
		return pdf / PortalShapes.size();
	}
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse, BxDFType flags = BSDF_ALL) const {
		const float cosi = Dot(wiW, ng);
		if (NumComponents(flags) == 1 && Dot(wiW, ng) > 0.f) {
			if (light.radianceMap == NULL) {
				return SWCSpectrum(reverse ? INV_PI : INV_PI * cosi);
			}
			const Vector wh = Normalize(WorldToLight(-wiW));
			float s, t, dummy;
			light.mapping->Map(wh, &s, &t, &dummy);
			return light.radianceMap->LookupSpectrum(sw, s, t) *
			(reverse ? INV_PI : INV_PI * cosi);
		}
		return SWCSpectrum(0.f);
	}
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		const Vector &woW, BxDFType flags = BSDF_ALL) const {
		return SWCSpectrum(1.f);
	}

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
		std::auto_ptr<ImageData> imgdata(ReadImage(texmap));
		if (imgdata.get() != NULL)
			radianceMap = imgdata->createMIPMap(BILINEAR, 8.f,
				TEXTURE_REPEAT, 1.f, gamma);
	}
}

float InfiniteAreaLight::Power(const Scene &scene) const
{
	Point worldCenter;
	float worldRadius;
	scene.WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	float power = SPDbase.Y() * M_PI * 4.f * M_PI * worldRadius * worldRadius;
	if (radianceMap != NULL)
		power *= radianceMap->LookupFloat(CHANNEL_MEAN, .5f, .5f, .5f);
	return power;
}

bool InfiniteAreaLight::Le(const Scene &scene, const Sample &sample,
	const Ray &r, BSDF **bsdf, float *pdf, float *pdfDirect,
	SWCSpectrum *L) const
{
	Point worldCenter;
	float worldRadius;
	scene.WorldBound().BoundingSphere(&worldCenter, &worldRadius);
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
	dg.time = sample.realTime;
	const Volume *v = GetVolume();
	if (!havePortalShape) {
		*bsdf = ARENA_ALLOC(sample.arena, InfiniteBSDF)(dg, ns,
			v, v, *this, WorldToLight);
		if (pdf)
			*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
		if (pdfDirect)
			*pdfDirect = AbsDot(r.d, ns) /
				(4.f * M_PI * DistanceSquared(r.o, ps));
	} else {
		*bsdf = ARENA_ALLOC(sample.arena, InfinitePortalBSDF)(dg, ns,
			v, v, *this, WorldToLight, ps, PortalShapes,
			~0U);
		if (pdf)
			*pdf = 0.f;
		if (pdfDirect)
			*pdfDirect = 0.f;
		DifferentialGeometry dgs;
		dgs.time = sample.realTime;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			if (pdf) {
				PortalShapes[i]->Sample(.5f, .5f, .5f, &dgs);
				const Vector w(dgs.p - ps);
				if (Dot(w, dgs.nn) > 0.f) {
					const float distance = w.LengthSquared();
					*pdf += AbsDot(ns, w) /
						(sqrtf(distance) * distance);
				}
			}
			if (pdfDirect) {
				Intersection isect;
				Ray ray(r);
				ray.mint = -INFINITY;
				ray.time = sample.realTime;
				if (PortalShapes[i]->Intersect(ray, &isect) &&
					Dot(r.d, isect.dg.nn) < 0.f)
					*pdfDirect += PortalShapes[i]->Pdf(r.o,
						isect.dg) * DistanceSquared(r.o,
						isect.dg.p) / DistanceSquared(r.o, ps) *
						AbsDot(r.d, ns) / AbsDot(r.d,
						isect.dg.nn);
			}
		}
		if (pdf)
			*pdf *= INV_TWOPI / nrPortalShapes;
		if (pdfDirect)
			*pdfDirect /= nrPortalShapes;
	}
	*L *= SWCSpectrum(sample.swl, SPDbase);
	if (radianceMap != NULL) {
		const Vector wh = Normalize(WorldToLight(r.d));
		float s, t, dummy;
		mapping->Map(wh, &s, &t, &dummy);
		*L *= radianceMap->LookupSpectrum(sample.swl, s, t);
	}
	return true;
}

float InfiniteAreaLight::Pdf(const Point &p, const PartialDifferentialGeometry &dg) const
{
	const Vector wi(dg.p - p);
	if (!havePortalShape) {
		const float d2 = wi.LengthSquared();
		return AbsDot(wi, dg.nn) / (4.f * M_PI * sqrtf(d2) * d2);
	} else {
		float pdf = 0.f;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			Intersection isect;
			Ray ray(p, wi);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect) &&
				Dot(wi, isect.dg.nn) < 0.f)
				pdf += PortalShapes[i]->Pdf(p, isect.dg) *
					DistanceSquared(p, isect.dg.p) /
					DistanceSquared(p, dg.p) *
					AbsDot(wi, dg.nn) /
					AbsDot(wi, isect.dg.nn);
		}
		pdf /= nrPortalShapes;
		return pdf;
	}
}

bool InfiniteAreaLight::SampleL(const Scene &scene, const Sample &sample,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	SWCSpectrum *Le) const
{
	Point worldCenter;
	float worldRadius;
	scene.WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	const Volume *v = GetVolume();
	if (!havePortalShape) {
		const Point ps = worldCenter +
			worldRadius * UniformSampleSphere(u1, u2);
		const Normal ns = Normal(Normalize(worldCenter - ps));
		Vector dpdu, dpdv;
		CoordinateSystem(Vector(ns), &dpdu, &dpdv);
		DifferentialGeometry dg(ps, ns, dpdu, dpdv, Normal(0, 0, 0),
			Normal (0, 0, 0), 0, 0, NULL);
		dg.time = sample.realTime;
		*bsdf = ARENA_ALLOC(sample.arena, InfiniteBSDF)(dg, ns,
			v, v, *this, WorldToLight);
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
		dgs.time = sample.realTime;
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
		dg.time = sample.realTime;
		*bsdf = ARENA_ALLOC(sample.arena, InfinitePortalBSDF)(dg, ns,
			v, v, *this, WorldToLight, ps, PortalShapes,
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
	*Le = SWCSpectrum(sample.swl, SPDbase) * (M_PI / *pdf);
	return true;
}
bool InfiniteAreaLight::SampleL(const Scene &scene, const Sample &sample,
	const Point &p, float u1, float u2, float u3,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *Le) const
{
	Vector wi;
	u_int shapeIndex = 0;
	Point worldCenter;
	float worldRadius;
	scene.WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	if(!havePortalShape) {
		// Sample uniform direction on unit sphere
		wi = UniformSampleSphere(u1, u2);
		// Compute _pdf_ for uniform infinite light direction
		*pdfDirect = .25f * INV_PI;
	} else {
		// Sample a random Portal
		if (nrPortalShapes > 1) {
			u3 *= nrPortalShapes;
			shapeIndex = min(nrPortalShapes - 1, Floor2UInt(u3));
			u3 -= shapeIndex;
		}
		DifferentialGeometry dg;
		dg.time = sample.realTime;
		*pdfDirect = PortalShapes[shapeIndex]->Sample(p, u1, u2, u3, &dg);
		if (!(*pdfDirect > 0.f)) {
			*Le = 0.f;
			return false;
		}
		Point ps = dg.p;
		wi = Normalize(ps - p);
		if (!(Dot(wi, dg.nn) < 0.f)) {
			*Le = 0.f;
			return false;
		}
		*pdfDirect *= DistanceSquared(p, dg.p) / AbsDot(wi, dg.nn);
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
	dg.time = sample.realTime;
	const Volume *v = GetVolume();
	if (!havePortalShape) {
		*bsdf = ARENA_ALLOC(sample.arena, InfiniteBSDF)(dg, ns,
			v, v, *this, WorldToLight);
		if (pdf)
			*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	} else {
		*bsdf = ARENA_ALLOC(sample.arena, InfinitePortalBSDF)(dg, ns,
			v, v, *this, WorldToLight, ps, PortalShapes,
			shapeIndex);
		if (pdf) {
			*pdf = 0.f;
			DifferentialGeometry dgs;
			dgs.time = sample.realTime;
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
	}
	if (havePortalShape) {
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			if (i == shapeIndex)
				continue;
			Intersection isect;
			Ray ray(p, wi);
			ray.mint = -INFINITY;
			ray.time = sample.realTime;
			if (PortalShapes[i]->Intersect(ray, &isect) &&
				Dot(wi, isect.dg.nn) < 0.f)
				*pdfDirect += PortalShapes[i]->Pdf(p,
					isect.dg) * DistanceSquared(p,
					isect.dg.p) / AbsDot(wi, isect.dg.nn);
		}
		*pdfDirect /= nrPortalShapes;
	}
	*pdfDirect *= AbsDot(wi, ns) / (distance * distance);
	*Le = SWCSpectrum(sample.swl, SPDbase) * (M_PI / *pdfDirect);
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

