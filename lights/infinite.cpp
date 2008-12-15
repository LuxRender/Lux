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
	void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const
	{
		Vector w(wi.x * X.x + wi.y * Y.x + wi.z * Z.x, wi.x * X.y + wi.y * Y.y + wi.z * Z.y, wi.x * X.z + wi.y * Y.z + wi.z * Z.z);
		*f += light.Le(tspack, RayDifferential(Point(0.f), -w));
	}
private:
	const InfiniteAreaLight &light;
	const Transform &WorldToLight;
	Vector X, Y, Z;
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
	worldRadius *= 1.01f;
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
	(*bsdf)->Add(BSDF_ALLOC(tspack, InfiniteBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns)));
	*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	if (!havePortalShape)
		*pdfDirect = AbsDot(r.d, n) * INV_TWOPI * AbsDot(r.d, ns) / DistanceSquared(r.o, ps);
	else {
		*pdfDirect = 0.f;
		for (int i = 0; i < nrPortalShapes; ++i) {
			Intersection isect;
			RayDifferential ray(r);
			ray.maxt = distance;
			if (PortalShapes[i]->Intersect(ray, &isect) && Dot(r.d, isect.dg.nn) < .0f)
				*pdfDirect += PortalShapes[i]->Pdf(r.o, r.d);
		}
		*pdfDirect *= AbsDot(r.d, ns) / (nrPortalShapes * distance * distance);
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
		Normal ns;
		Point ps;
		bool found = false;
		for (int i = 0; i < nrPortalShapes; ++i) {
			ps = PortalShapes[shapeidx]->Sample(p, u1, u2, u3, &ns);
			*wi = Normalize(ps - p);
			if (Dot(*wi, ns) < 0.f) {
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
		Normal ns;
		Point ps;
		bool found = false;
		for (int i = 0; i < nrPortalShapes; ++i) {
			ps = PortalShapes[shapeidx]->Sample(p, u1, u2, u3, &ns);
			*wi = Normalize(ps - p);
			if (Dot(*wi, ns) < 0.f) {
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

		Normal ns;
		ray->o = PortalShapes[shapeidx]->Sample(u1, u2, tspack->rng->floatValue(), &ns); // TODO - REFACT - add passed value from sample
		ray->d = UniformSampleSphere(u3, u4);
		if (Dot(ray->d, ns) < 0.) ray->d *= -1;

		*pdf = PortalShapes[shapeidx]->Pdf(ray->o) * INV_TWOPI / nrPortalShapes;
	}

	return Le(tspack, RayDifferential(ray->o, -ray->d));
}
bool InfiniteAreaLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const
{
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	worldRadius *= 1.01f;
	Point ps = worldCenter + worldRadius * UniformSampleSphere(u1, u2);
	Normal ns = Normal(Normalize(worldCenter - ps));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Vector(0, 0, 0), Vector (0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(tspack, BSDF)(dg, ns);
	(*bsdf)->Add(BSDF_ALLOC(tspack, InfiniteBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns)));
	*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	*Le = SWCSpectrum(1.f);
	return true;
}
bool InfiniteAreaLight::Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility, SWCSpectrum *Le) const
{
	Vector wi;
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
		wi = Vector(v1.x * wi.x + v2.x * wi.y + n.x * wi.z,
			 v1.y * wi.x + v2.y * wi.y + n.y * wi.z,
			 v1.z * wi.x + v2.z * wi.y + n.z * wi.z);
	} else {
		// Sample a random Portal
		int shapeIndex = 0;
		if(nrPortalShapes > 1) {
			shapeIndex = Floor2Int(u3 * nrPortalShapes);
			u3 *= nrPortalShapes;
			u3 -= shapeIndex;
		}
		Normal ns;
		Point ps = PortalShapes[shapeIndex]->Sample(p, u1, u2, u3, &ns);
		wi = Normalize(ps - p);
		if (Dot(wi, ns) < 0.f)
			*pdfDirect = PortalShapes[shapeIndex]->Pdf(p, wi) / nrPortalShapes;
		else {
			*Le = 0.f;
			return false;
		}
	}
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	worldRadius *= 1.01f;
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
	(*bsdf)->Add(BSDF_ALLOC(tspack, InfiniteBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns)));
	*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	*pdfDirect *= AbsDot(wi, ns) / DistanceSquared(p, ps);
	visibility->SetSegment(p, ps, tspack->time);
	*Le = SWCSpectrum(1.f);
	return true;
}

Light* InfiniteAreaLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
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

