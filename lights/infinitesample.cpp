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
 
// TODO - Port SPD interfaces

// infinitesample.cpp*
#include "infinitesample.h"
#include "imagereader.h"
#include "mcdistribution.h"
#include "paramset.h"
#include "reflection/bxdf.h"
#include "dynload.h"

using namespace lux;

//FIXME - do proper sampling
class  InfiniteISBSDF : public BSDF  {
public:
	// InfiniteISBSDF Public Methods
	InfiniteISBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior,
		const InfiniteAreaLightIS &l, const Transform &WL) :
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
	// InfiniteISBSDF Private Methods
	virtual ~InfiniteISBSDF() { }
	const InfiniteAreaLightIS &light;
	const Transform &WorldToLight;
};

// InfiniteAreaLightIS Method Definitions
InfiniteAreaLightIS::~InfiniteAreaLightIS() {
	delete uvDistrib;
	delete radianceMap;
	delete mapping;
}
InfiniteAreaLightIS::InfiniteAreaLightIS(const Transform &light2world,
	const RGBColor &l, u_int ns, const string &texmap,
	EnvironmentMapping *m, float gain, float gamma)
	: Light(light2world, ns), SPDbase(l)
{
	// Base illuminant SPD
	SPDbase.Scale(gain);

	mapping = m;
	radianceMap = NULL;
	uvDistrib = NULL;
	u_int nu = 0, nv = 0;
	if (texmap != "") {
		auto_ptr<ImageData> imgdata(ReadImage(texmap));
		if (imgdata.get() != NULL) {
			nu = imgdata->getWidth();
			nv = imgdata->getHeight();
			radianceMap = imgdata->createMIPMap(BILINEAR, 8.f,
				TEXTURE_REPEAT, 1.f, gamma);
		} else
			radianceMap = NULL;
	}
	if (radianceMap == NULL) {
		// Set default sampling array size
		nu = 2;
		nv = 128;
	}
	// Initialize sampling PDFs for infinite area light
	float filter = 1.f / max(nu, nv);
	float *img = new float[nu * nv];
	for (u_int y = 0; y < nv; ++y) {
		float yp = (y + .5f) / nv;
		for (u_int x = 0; x < nu; ++x) {
			float xp = (x + .5f) / nu;
			Vector dummy;
			float pdf;
			mapping->Map(xp, yp, &dummy, &pdf);
			if (!(pdf > 0.f))
				img[x + y * nu] = 0.f;
			else if (radianceMap)
				img[x + y * nu] = radianceMap->LookupFloat(CHANNEL_WMEAN,
					xp, yp, filter) / pdf;
			else
				img[x + y * nu] = 1.f / pdf;
		}
	}
	uvDistrib = new Distribution2D(img, nu, nv);
	delete[] img;
}
SWCSpectrum InfiniteAreaLightIS::Le(const TsPack *tspack,
	const RayDifferential &r) const
{
	Vector w = r.d;
	// Compute infinite light radiance for direction
	if (radianceMap != NULL) {
		Vector wh = Normalize(WorldToLight(w));
		float s = SphericalPhi(wh) * INV_TWOPI;
		float t = SphericalTheta(wh) * INV_PI;
		return SWCSpectrum(tspack, SPDbase) *
			radianceMap->LookupSpectrum(tspack, s, t);
	}
	return SWCSpectrum(tspack, SPDbase);
}

SWCSpectrum InfiniteAreaLightIS::Le(const TsPack *tspack, const Scene *scene,
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
	*bsdf = ARENA_ALLOC(tspack->arena, InfiniteISBSDF)(dg, ns,
		NULL, NULL, *this, WorldToLight);
	*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	if (radianceMap != NULL) {
		const Vector wh = Normalize(WorldToLight(r.d));
		float s, t, pdfMap;
		mapping->Map(wh, &s, &t, &pdfMap);
		*pdfDirect = uvDistrib->Pdf(s, t) * pdfMap *
			AbsDot(r.d, ns) / DistanceSquared(r.o, ps);
		return SWCSpectrum(tspack, SPDbase) *
			radianceMap->LookupSpectrum(tspack, s, t);
	} else {
		*pdfDirect = AbsDot(r.d, n) * INV_TWOPI *
			AbsDot(r.d, ns) / DistanceSquared(r.o, ps);
		return SWCSpectrum(tspack, SPDbase);
	}
}

SWCSpectrum InfiniteAreaLightIS::Sample_L(const TsPack *tspack, const Point &p, float u1,
		float u2, float u3, Vector *wi, float *pdf,
		VisibilityTester *visibility) const {
	// Find floating-point $(u,v)$ sample coordinates
	float uv[2];
	uvDistrib->SampleContinuous(u1, u2, uv, pdf);
	// Convert sample point to direction on the unit sphere
	const float theta = uv[1] * M_PI;
	const float phi = uv[0] * 2.f * M_PI;
	const float costheta = cosf(theta), sintheta = sinf(theta);
	*wi = LightToWorld(SphericalDirection(sintheta, costheta, phi));
	// Compute PDF for sampled direction
	// FIXME - use mapping
	*pdf /= (2.f * M_PI * M_PI * sintheta);
	// Return radiance value for direction
	visibility->SetRay(p, *wi, tspack->time);
	if (radianceMap)
		return SWCSpectrum(tspack, SPDbase) *
			radianceMap->LookupSpectrum(tspack, uv[0], uv[1]);
	else
		return SWCSpectrum(tspack, SPDbase);
}
float InfiniteAreaLightIS::Pdf(const TsPack *tspack, const Point &,
		const Vector &w) const {
	Vector wi = WorldToLight(w);
	float theta = SphericalTheta(wi), phi = SphericalPhi(wi);
	// FIXME - use pdf from mapping
	return uvDistrib->Pdf(phi * INV_TWOPI, theta * INV_PI) /
		(2.f * M_PI * M_PI * sin(theta));
}

float InfiniteAreaLightIS::Pdf(const TsPack *tspack, const Point &p,
	const Normal &n, const Point &po, const Normal &ns) const
{
	const Vector d(Normalize(po - p));
	if (radianceMap != NULL) {
		const Vector wh = Normalize(WorldToLight(d));
		float s, t, pdf;
		mapping->Map(wh, &s, &t, &pdf);
		return uvDistrib->Pdf(s, t) * pdf *
			AbsDot(d, ns) / DistanceSquared(p, po);
	} else {
		return AbsDot(d, n) * INV_TWOPI *
			AbsDot(d, ns) / DistanceSquared(p, po);
	}
}

SWCSpectrum InfiniteAreaLightIS::Sample_L(const TsPack *tspack, const Scene *scene,
		float u1, float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
	// Choose two points _p1_ and _p2_ on scene bounding sphere
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter,
		&worldRadius);
	worldRadius *= 1.01f;
	Point p1 = worldCenter + worldRadius *
		UniformSampleSphere(u1, u2);
	Point p2 = worldCenter + worldRadius *
		UniformSampleSphere(u3, u4);
	// Construct ray between _p1_ and _p2_
	ray->o = p1;
	ray->d = Normalize(p2-p1);
	// Compute _InfiniteAreaLightIS_ ray weight
	Vector to_center = Normalize(worldCenter - p1);
	float costheta = AbsDot(to_center,ray->d);
	*pdf =
		costheta / ((4.f * M_PI * worldRadius * worldRadius));
	return Le(tspack, RayDifferential(ray->o, -ray->d));
}

bool InfiniteAreaLightIS::Sample_L(const TsPack *tspack, const Scene *scene,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	SWCSpectrum *Le) const
{
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	const Point ps = worldCenter +
		worldRadius * UniformSampleSphere(u1, u2);
	const Normal ns = Normal(Normalize(worldCenter - ps));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Normal(0, 0, 0),
		Normal (0, 0, 0), 0, 0, NULL);
	*bsdf = ARENA_ALLOC(tspack->arena, InfiniteISBSDF)(dg, ns,
		NULL, NULL, *this, WorldToLight);
	*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	*Le = SWCSpectrum(tspack, SPDbase) * M_PI;
	return true;
}

bool InfiniteAreaLightIS::Sample_L(const TsPack *tspack, const Scene *scene,
	const Point &p, const Normal &n, float u1, float u2, float u3,
	BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility, SWCSpectrum *Le) const
{
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	// Find floating-point $(u,v)$ sample coordinates
	float uv[2];
	uvDistrib->SampleContinuous(u1, u2, uv, pdfDirect);
	// Convert sample point to direction on the unit sphere
	Vector wi;
	float pdfMap;
	mapping->Map(uv[0], uv[1], &wi, &pdfMap);
	if (!(pdfMap > 0.f))
		return false;
	// Compute PDF for sampled direction
	*pdfDirect *= pdfMap;
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
	*bsdf = ARENA_ALLOC(tspack->arena, InfiniteISBSDF)(dg, ns,
		NULL, NULL, *this, WorldToLight);
	*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	*pdfDirect *= AbsDot(wi, ns) / (distance * distance);
	visibility->SetSegment(p, ps, tspack->time);
	*Le = SWCSpectrum(tspack, SPDbase) * M_PI;
	return true;
}

Light* InfiniteAreaLightIS::CreateLight(const Transform &light2world,
	const ParamSet &paramSet)
{
	RGBColor L = paramSet.FindOneRGBColor("L", RGBColor(1.f));
	string texmap = paramSet.FindOneString("mapname", "");
	int nSamples = paramSet.FindOneInt("nsamples", 1);

	EnvironmentMapping *map = NULL;
	string type = paramSet.FindOneString("mapping", "");
	if (type == "" || type == "latlong")
		map = new LatLongMapping();
	else if (type == "angular")
		map = new AngularMapping();
	else if (type == "vcross")
		map = new VerticalCrossMapping();

	// Initialize _ImageTexture_ parameters
	float gain = paramSet.FindOneFloat("gain", 1.0f);
	float gamma = paramSet.FindOneFloat("gamma", 1.0f);

	InfiniteAreaLightIS *l = new InfiniteAreaLightIS(light2world, L, nSamples, texmap, map, gain, gamma);
	l->hints.InitParam(paramSet);
	return l;
}

static DynamicLoader::RegisterLight<InfiniteAreaLightIS> r("infinitesample");

