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

// infinite.cpp*
#include "infinite.h"
#include "imagereader.h"
#include "mc.h"
#include "paramset.h"
#include "blackbodyspd.h"
#include "reflection/bxdf.h"
#include "dynload.h"

using namespace lux;

class InfiniteBxDF : public BxDF
{
public:
	InfiniteBxDF(const InfiniteAreaLight &l, const Transform &WL, const Vector &x, const Vector &y, const Vector &z) : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), light(l), WorldToLight(WL), X(x), Y(y), Z(z) {}
	virtual ~InfiniteBxDF() { }
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const
	{
		Vector w(wi.x * X + wi.y * Y + wi.z * Z);
		*f += light.Le(tspack, RayDifferential(Point(0.f), -w));
	}
private:
	const InfiniteAreaLight &light;
	const Transform &WorldToLight;
	Vector X, Y, Z;
};

class InfinitePortalBxDF : public BxDF
{
public:
	InfinitePortalBxDF(const InfiniteAreaLight &l, const Transform &WL, const Vector &x, const Vector &y, const Vector &z, const Point &p, const vector<boost::shared_ptr<Primitive> > &portalList, u_int portal, float u) : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), light(l), WorldToLight(WL), X(x), Y(y), Z(z), ps(p), PortalShapes(portalList), shapeIndex(portal), u3(u) {}
	virtual ~InfinitePortalBxDF() { }
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi, float u1, float u2, SWCSpectrum *const f,float *pdf, float *pdfBack = NULL, bool reverse = false) const
	{
		if (shapeIndex == ~0U || reverse)
			return false;
		DifferentialGeometry dg;
		PortalShapes[shapeIndex]->Sample(ps, u1, u2, u3, &dg);
		Vector wiW = Normalize(dg.p - ps);
		*f = light.Le(tspack, RayDifferential(Point(0.f), -wiW));
		wi->x = Dot(wiW, X);
		wi->y = Dot(wiW, Y);
		wi->z = Dot(wiW, Z);
		*wi = Normalize(*wi);
		*pdf = PortalShapes[shapeIndex]->Pdf(ps, dg.p) * DistanceSquared(ps, dg.p) / AbsDot(wiW, dg.nn);
		for (u_int i = 0; i < PortalShapes.size(); ++i) {
			if (i != shapeIndex) {
				Intersection isect;
				RayDifferential ray(ps, wiW);
				ray.mint = -INFINITY;
				if (PortalShapes[i]->Intersect(ray, &isect) && Dot(wiW, isect.dg.nn) > 0.f)
					*pdf += PortalShapes[i]->Pdf(ps, isect.dg.p) * DistanceSquared(ps, isect.dg.p) / AbsDot(wiW, isect.dg.nn);
			}
		}
		*pdf /= PortalShapes.size();
		if (pdfBack)
			*pdfBack = 0.f;
		return true;
	}
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const
	{
		Vector w(wi.x * X + wi.y * Y + wi.z * Z);
		*f += light.Le(tspack, RayDifferential(Point(0.f), -w));
	}
	virtual float Pdf(const Vector &wi, const Vector &wo) const
	{
		Vector w(wo.x * X + wo.y * Y + wo.z * Z);
		float pdf = 0.f;
		for (u_int i = 0; i < PortalShapes.size(); ++i) {
			Intersection isect;
			RayDifferential ray(ps, w);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect) && Dot(w, isect.dg.nn) > 0.f)
				pdf += PortalShapes[i]->Pdf(ps, isect.dg.p) * DistanceSquared(ps, isect.dg.p) / AbsDot(w, isect.dg.nn);
		}
		return pdf / PortalShapes.size();
	}
private:
	const InfiniteAreaLight &light;
	const Transform &WorldToLight;
	Vector X, Y, Z;
	Point ps;
	const vector<boost::shared_ptr<Primitive> > &PortalShapes;
	u_int shapeIndex;
	float u3;
};

// InfiniteAreaLight Method Definitions
InfiniteAreaLight::~InfiniteAreaLight() {
	delete radianceMap;
	delete SPDbase;
	delete mapping;
}
InfiniteAreaLight
	::InfiniteAreaLight(const Transform &light2world, const RGBColor &l, int ns, const string &texmap, EnvironmentMapping *m, float gain, float gamma)
	: Light(light2world, ns) {
	radianceMap = NULL;
	if (texmap != "") {
		auto_ptr<ImageData> imgdata(ReadImage(texmap));
		if(imgdata.get()!=NULL)
		{
			radianceMap = imgdata->createMIPMap<RGBColor>(BILINEAR, 8.f, 
				TEXTURE_REPEAT, 1.f, gamma);
		}
		else
			radianceMap=NULL;
	}

	mapping = m;

	// Base illuminant SPD
	SPDbase = new BlackbodySPD();
	SPDbase->Normalize();
	SPDbase->Scale(gain);

	// Base RGB RGBColor
	Lbase = l;
}

SWCSpectrum
	InfiniteAreaLight::Le(const TsPack *tspack, const RayDifferential &r) const {
	// Compute infinite light radiance for direction
	
	RGBColor L = Lbase;
	if (radianceMap != NULL) {
		Vector wh = Normalize(WorldToLight(r.d));

		float s, t;

		mapping->Map(wh, &s, &t);

		L *= radianceMap->Lookup(s, t);
	}

	return SWCSpectrum(tspack, SPDbase) * SWCSpectrum(tspack, L);
}
SWCSpectrum InfiniteAreaLight::Le(const TsPack *tspack, const Scene *scene, const Ray &r,
	const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const
{
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	Vector toCenter(worldCenter - r.o);
	float centerDistance = Dot(toCenter, toCenter);
	float approach = Dot(toCenter, r.d);
	float distance = approach + sqrtf(worldRadius * worldRadius - centerDistance + approach * approach);
	Point ps(r.o + distance * r.d);
	Normal ns(Normalize(worldCenter - ps));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Vector(0, 0, 0), Vector (0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(tspack, BSDF)(dg, ns);
	if (!havePortalShape) {
		(*bsdf)->Add(BSDF_ALLOC(tspack, InfiniteBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns)));
		*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
		*pdfDirect = AbsDot(r.d, n) * INV_TWOPI * AbsDot(r.d, ns) / DistanceSquared(r.o, ps);
	} else {
		float u3 = tspack->rng->floatValue();//FIXME
		(*bsdf)->Add(BSDF_ALLOC(tspack, InfinitePortalBxDF)(*this,
			WorldToLight, dpdu, dpdv, Vector(ns), ps, PortalShapes,
			~0U, u3));
		*pdf = 0.f;
		*pdfDirect = 0.f;
		for (int i = 0; i < nrPortalShapes; ++i) {
			PortalShapes[i]->Sample(.5f, .5f, u3, &dg);
			Vector w(dg.p - ps);
			if (Dot(w, dg.nn) > 0.f) {
				float distance = w.LengthSquared();
				*pdf += AbsDot(ns, w) / (sqrtf(distance) * distance);
			}
			Intersection isect;
			RayDifferential ray(r);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect) && Dot(r.d, isect.dg.nn) < 0.f)
				*pdfDirect += PortalShapes[i]->Pdf(r.o, isect.dg.p) * DistanceSquared(r.o, isect.dg.p) / DistanceSquared(r.o, ps) * AbsDot(r.d, ns) / AbsDot(r.d, isect.dg.nn);
		}
		*pdf *= INV_TWOPI / nrPortalShapes;
		*pdfDirect /= nrPortalShapes;
	}
	return Le(tspack, RayDifferential(r));
}
SWCSpectrum InfiniteAreaLight::Sample_L(const TsPack *tspack, const Point &p,
		const Normal &n, float u1, float u2, float u3,
		Vector *wi, float *pdf,
		VisibilityTester *visibility) const {
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
		*wi = Vector(v1.x * wi->x + v2.x * wi->y + n.x * wi->z,
					 v1.y * wi->x + v2.y * wi->y + n.y * wi->z,
					 v1.z * wi->x + v2.z * wi->y + n.z * wi->z);
	} else {
	    // Sample Portal
		int shapeidx = 0;
		if(nrPortalShapes > 1) 
			shapeidx = min<float>(nrPortalShapes - 1, u3 * nrPortalShapes);
		DifferentialGeometry dg;
		Point ps;
		bool found = false;
		for (int i = 0; i < nrPortalShapes; ++i) {
			PortalShapes[shapeidx]->Sample(p, u1, u2, u3, &dg);
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
float InfiniteAreaLight::Pdf(const Point &, const Normal &n,
		const Vector &wi) const {
	return AbsDot(n, wi) * INV_TWOPI;
}
float InfiniteAreaLight::Pdf(const Point &p, const Normal &n,
	const Point &po, const Normal &ns) const
{
	const Vector wi(po - p);
	if (!havePortalShape) {
		const float d2 = wi.LengthSquared();
		return AbsDot(n, wi) * INV_TWOPI * AbsDot(wi, ns) / (d2 * d2);
	} else {
		float pdf = 0.f;
		for (int i = 0; i < nrPortalShapes; ++i) {
			Intersection isect;
			RayDifferential ray(p, wi);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect) &&
				Dot(wi, isect.dg.nn) < 0.f)
				pdf += PortalShapes[i]->Pdf(p, isect.dg.p) * DistanceSquared(p, isect.dg.p) / DistanceSquared(p, po) * AbsDot(wi, ns) / AbsDot(wi, isect.dg.nn);
		}
		pdf /= nrPortalShapes;
		return pdf;
	}
}
SWCSpectrum InfiniteAreaLight::Sample_L(const TsPack *tspack, const Point &p,
		float u1, float u2, float u3, Vector *wi, float *pdf,
		VisibilityTester *visibility) const {
	if(!havePortalShape) {
		*wi = UniformSampleSphere(u1, u2);
		*pdf = UniformSpherePdf();
	} else {
	    // Sample a random Portal
		int shapeidx = 0;
		if(nrPortalShapes > 1) 
			shapeidx = min<float>(nrPortalShapes - 1, u3 * nrPortalShapes);
		DifferentialGeometry dg;
		Point ps;
		bool found = false;
		for (int i = 0; i < nrPortalShapes; ++i) {
			PortalShapes[shapeidx]->Sample(p, u1, u2, u3, &dg);
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
float InfiniteAreaLight::Pdf(const Point &, const Vector &) const {
	return 1.f / (4.f * M_PI);
}
SWCSpectrum InfiniteAreaLight::Sample_L(const TsPack *tspack, const Scene *scene,
		float u1, float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
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
		*pdf =
			costheta / ((4.f * M_PI * worldRadius * worldRadius));		
	} else {
		// Dade - choose a random portal. This strategy is quite bad if there
		// is more than one portal.
		int shapeidx = 0;
		if(nrPortalShapes > 1) 
			shapeidx = min<float>(nrPortalShapes - 1,
					Floor2Int(tspack->rng->floatValue() * nrPortalShapes));  // TODO - REFACT - add passed value from sample

		DifferentialGeometry dg;
		PortalShapes[shapeidx]->Sample(u1, u2, tspack->rng->floatValue(), &dg); // TODO - REFACT - add passed value from sample
		ray->o = dg.p;
		ray->d = UniformSampleSphere(u3, u4);
		if (Dot(ray->d, dg.nn) < 0.) ray->d *= -1;

		*pdf = PortalShapes[shapeidx]->Pdf(ray->o) * INV_TWOPI / nrPortalShapes;
	}

	return Le(tspack, RayDifferential(ray->o, -ray->d));
}
bool InfiniteAreaLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const
{
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	if (!havePortalShape) {
		Point ps = worldCenter + worldRadius * UniformSampleSphere(u1, u2);
		Normal ns = Normal(Normalize(worldCenter - ps));
		Vector dpdu, dpdv;
		CoordinateSystem(Vector(ns), &dpdu, &dpdv);
		DifferentialGeometry dg(ps, ns, dpdu, dpdv, Vector(0, 0, 0), Vector (0, 0, 0), 0, 0, NULL);
		*bsdf = BSDF_ALLOC(tspack, BSDF)(dg, ns);
		(*bsdf)->Add(BSDF_ALLOC(tspack, InfiniteBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns)));
		*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	} else {
		// Sample a random Portal
		int shapeIndex = 0;
		if (nrPortalShapes > 1) {
			shapeIndex = min(nrPortalShapes - 1, Floor2Int(u3 * nrPortalShapes));
			u3 *= nrPortalShapes;
			u3 -= shapeIndex;
		}
		DifferentialGeometry dgs;
		PortalShapes[shapeIndex]->Sample(.5f, .5f, u3, &dgs);
		Vector wi(UniformSampleHemisphere(u1, u2));
		wi = Normalize(wi.x * Normalize(dgs.dpdu) + wi.y * Normalize(dgs.dpdv) - wi.z * Vector(dgs.nn));
		Vector toCenter(worldCenter - dgs.p);
		float centerDistance = Dot(toCenter, toCenter);
		float approach = Dot(toCenter, wi);
		float distance = approach + sqrtf(worldRadius * worldRadius - centerDistance + approach * approach);
		Point ps(dgs.p + distance * wi);
		Normal ns(Normalize(worldCenter - ps));
		Vector dpdu, dpdv;
		CoordinateSystem(Vector(ns), &dpdu, &dpdv);
		DifferentialGeometry dg(ps, ns, dpdu, dpdv, Vector(0, 0, 0), Vector(0, 0, 0), 0, 0, NULL);
		*bsdf = BSDF_ALLOC(tspack, BSDF)(dg, ns);
		(*bsdf)->Add(BSDF_ALLOC(tspack, InfinitePortalBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns), ps, PortalShapes, shapeIndex, u3));
		*pdf = AbsDot(ns, wi) / (distance * distance);
		for (int i = 0; i < nrPortalShapes; ++i) {
			if (i != shapeIndex) {
				PortalShapes[i]->Sample(.5f, .5f, u3, &dgs);
				wi = ps - dgs.p;
				if (Dot(wi, dg.nn) < 0.f) {
					distance = wi.LengthSquared();
					*pdf += AbsDot(ns, wi) / (sqrtf(distance) * distance);
				}
			}
		}
		*pdf *= INV_TWOPI / nrPortalShapes;
	}
	*Le = SWCSpectrum(1.f);
	return true;
}
bool InfiniteAreaLight::Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility, SWCSpectrum *Le) const
{
	Vector wi;
	int shapeIndex = 0;
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	if(!havePortalShape) {
		// Sample cosine-weighted direction on unit sphere
		float x, y, z;
		ConcentricSampleDisk(u1, u2, &x, &y);
		z = sqrtf(max(0.f, 1.f - x*x - y*y));
		if (u3 < .5)
			z *= -1;
		wi = Vector(x, y, z);
		// Compute _pdf_ for cosine-weighted infinite light direction
		*pdfDirect = fabsf(wi.z) * INV_TWOPI;
		// Transform direction to world space
		Vector v1, v2;
		CoordinateSystem(Normalize(Vector(n)), &v1, &v2);
		wi = Vector(wi.x * v1 + wi.y * v2 + wi.z * Vector(n));
	} else {
		// Sample a random Portal
		if(nrPortalShapes > 1) {
			shapeIndex = min(nrPortalShapes - 1, Floor2Int(u3 * nrPortalShapes));
			u3 *= nrPortalShapes;
			u3 -= shapeIndex;
		}
		DifferentialGeometry dg;
		PortalShapes[shapeIndex]->Sample(p, u1, u2, u3, &dg);
		Point ps = dg.p;
		wi = Normalize(ps - p);
		if (Dot(wi, dg.nn) < 0.f) {
			*pdfDirect = PortalShapes[shapeIndex]->Pdf(p, ps) / nrPortalShapes;
			*pdfDirect *= DistanceSquared(p, dg.p) / AbsDot(wi, dg.nn);
		} else {
			*Le = 0.f;
			return false;
		}
	}
	Vector toCenter(worldCenter - p);
	float centerDistance = Dot(toCenter, toCenter);
	float approach = Dot(toCenter, wi);
	float distance = approach + sqrtf(worldRadius * worldRadius - centerDistance + approach * approach);
	Point ps(p + distance * wi);
	Normal ns(Normalize(worldCenter - ps));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Vector(0, 0, 0), Vector (0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(tspack, BSDF)(dg, ns);
	if (!havePortalShape) {
		(*bsdf)->Add(BSDF_ALLOC(tspack, InfiniteBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns)));
		*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	} else {
		(*bsdf)->Add(BSDF_ALLOC(tspack, InfinitePortalBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns), ps, PortalShapes, shapeIndex, u3));
		*pdf = 0.f;
		DifferentialGeometry dgs;
		for (int i = 0; i < nrPortalShapes; ++i) {
			PortalShapes[i]->Sample(.5f, .5f, u3, &dgs);
			Vector w(ps - dgs.p);
			if (Dot(wi, dg.nn) < 0.f) {
				float distance = w.LengthSquared();
				*pdf += AbsDot(ns, w) / (sqrtf(distance) * distance);
			}
		}
		*pdf *= INV_TWOPI / nrPortalShapes;
	}
	*pdfDirect *= AbsDot(wi, ns) / (distance * distance);
	visibility->SetSegment(p, ps, tspack->time);
	*Le = SWCSpectrum(1.f);
	return true;
}

Light* InfiniteAreaLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet, const TextureParams &tp) {
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

	return new InfiniteAreaLight(light2world, L, nSamples, texmap, map, gain, gamma);
}

static DynamicLoader::RegisterLight<InfiniteAreaLight> r("infinite");

