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
 
// infinitesample.cpp*
#include "infinitesample.h"
#include "imagereader.h"
#include "mcdistribution.h"
#include "paramset.h"
#include "bxdf.h"
#include "singlebsdf.h"
#include "sampling.h"
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
	const RGBColor &l, u_int ns, const string &texmap, u_int immaxres,
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
		std::auto_ptr<ImageData> imgdata(ReadImage(texmap));
		if (imgdata.get() != NULL) {
			nu = imgdata->getWidth();
			nv = imgdata->getHeight();
			radianceMap = imgdata->createMIPMap(BILINEAR, 8.f,
				TEXTURE_REPEAT, 1.f, gamma);
		}
	}
	if (radianceMap == NULL) {
		// Set default sampling array size
		nu = 2;
		nv = 128;
	}
	// Initialize sampling PDFs for infinite area light
	// resample using supersampling if dimensions are large
	u_int dnu = nu;
	u_int dnv = nv;
	if (nu > immaxres || nv > immaxres) {
		dnu = Ceil2UInt(static_cast<float>(nu * immaxres) / max(nu, nv));
		dnv = Ceil2UInt(static_cast<float>(nv * immaxres) / max(nu, nv));
	}
	const float uscale = static_cast<float>(nu) / dnu;
	const float vscale = static_cast<float>(nv) / dnv;
	const u_int samples = Ceil2UInt(max(uscale, vscale));
	const float us = uscale / samples;
	const float vs = vscale / samples;

	const float filter = 1.f / max(nu, nv);
	vector<float> img(dnu * dnv);
	LOG(LUX_DEBUG, LUX_NOERROR) << "Computing importance sampling map";
	mean_y = 0.f;
	for (u_int y = 0; y < dnv*samples; ++y) {
		const float yp = (y * vs + .5f) / nv;
		u_int iy = y / samples;
		for (u_int x = 0; x < dnu*samples; ++x) {
			const float xp = (x * us + .5f) / nu;
			Vector dummy;
			float pdf;
			mapping->Map(xp, yp, &dummy, &pdf);
			u_int ix = x / samples;
			if (!(pdf > 0.f))
			//	img[ix + iy * dnu] += 0.f;
				continue;
			const float y = (radianceMap) ? 
				radianceMap->LookupFloat(CHANNEL_WMEAN, xp, yp, filter) : 1.f;
			img[ix + iy * dnu] += y / (samples*samples*pdf);
			mean_y += y;
		}
	}
	mean_y /= dnu*samples * dnv*samples;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Finished computing importance sampling map";
	uvDistrib = new Distribution2D(&img[0], dnu, dnv);
}

bool InfiniteAreaLightIS::Le(const Scene &scene, const Sample &sample,
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
	*bsdf = ARENA_ALLOC(sample.arena, InfiniteISBSDF)(dg, ns,
		v, v, *this, WorldToLight);
	*L *= SWCSpectrum(sample.swl, SPDbase);
	const Vector wh = Normalize(WorldToLight(r.d));
	float s, t, pdfMap;
	mapping->Map(wh, &s, &t, &pdfMap);
	if (radianceMap != NULL)
		*L *= radianceMap->LookupSpectrum(sample.swl, s, t);
	if (pdf)
		*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	if (pdfDirect)
		*pdfDirect = uvDistrib->Pdf(s, t) * pdfMap *
			AbsDot(r.d, ns) / DistanceSquared(r.o, ps);
	return true;
}

float InfiniteAreaLightIS::Pdf(const Point &p, const PartialDifferentialGeometry &dg) const
{
	const Vector d(Normalize(dg.p - p));
	const Vector wh = Normalize(WorldToLight(d));
	float s, t, pdf;
	mapping->Map(wh, &s, &t, &pdf);
	return uvDistrib->Pdf(s, t) * pdf * AbsDot(d, dg.nn) /
		DistanceSquared(p, dg.p);
}

bool InfiniteAreaLightIS::SampleL(const Scene &scene, const Sample &sample,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	SWCSpectrum *Le) const
{
	Point worldCenter;
	float worldRadius;
	scene.WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	const Point ps = worldCenter +
		worldRadius * UniformSampleSphere(u1, u2);
	const Normal ns = Normal(Normalize(worldCenter - ps));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Normal(0, 0, 0),
		Normal (0, 0, 0), 0, 0, NULL);
	dg.time = sample.realTime;
	const Volume *v = GetVolume();
	*bsdf = ARENA_ALLOC(sample.arena, InfiniteISBSDF)(dg, ns,
		v, v, *this, WorldToLight);
	*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	*Le = SWCSpectrum(sample.swl, SPDbase) * (M_PI / *pdf);
	return true;
}

bool InfiniteAreaLightIS::SampleL(const Scene &scene, const Sample &sample,
	const Point &p, float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	float *pdfDirect, SWCSpectrum *Le) const
{
	Point worldCenter;
	float worldRadius;
	scene.WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	// Find floating-point $(u,v)$ sample coordinates
	float uv[2];
	uvDistrib->SampleContinuous(u1, u2, uv, pdfDirect);
	// Convert sample point to direction on the unit sphere
	Vector wi;
	float pdfMap;
	mapping->Map(uv[0], uv[1], &wi, &pdfMap);
	wi = Normalize(LightToWorld(wi)); 
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
	dg.time = sample.realTime;
	const Volume *v = GetVolume();
	*bsdf = ARENA_ALLOC(sample.arena, InfiniteISBSDF)(dg, ns,
		v, v, *this, WorldToLight);
	if (pdf)
		*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	*pdfDirect *= AbsDot(wi, ns) / (distance * distance);
	*Le = SWCSpectrum(sample.swl, SPDbase) * (M_PI / *pdfDirect);
	return true;
}

Light* InfiniteAreaLightIS::CreateLight(const Transform &light2world,
	const ParamSet &paramSet)
{
	RGBColor L = paramSet.FindOneRGBColor("L", RGBColor(1.f));
	string texmap = paramSet.FindOneString("mapname", "");
	int nSamples = paramSet.FindOneInt("nsamples", 1);
	int imapmaxres = paramSet.FindOneInt("imapmaxresolution", 500);

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

	InfiniteAreaLightIS *l = new InfiniteAreaLightIS(light2world, L, nSamples, texmap, imapmaxres, map, gain, gamma);
	l->hints.InitParam(paramSet);
	return l;
}

static DynamicLoader::RegisterLight<InfiniteAreaLightIS> r("infinitesample");

