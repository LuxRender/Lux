/*
This source is published under the following 3-clause BSD license.

Copyright (c) 2012, Lukas Hosek and Alexander Wilkie
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * None of the names of the contributors may be used to endorse or promote 
      products derived from this software without specific prior written 
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* ============================================================================

This file is part of a sample implementation of the analytical skylight model
presented in the SIGGRAPH 2012 paper


           "An Analytic Model for Full Spectral Sky-Dome Radiance"

                                    by 

                       Lukas Hosek and Alexander Wilkie
                Charles University in Prague, Czech Republic


                        Version: 1.1, July 4th, 2012
                        
Version history:

1.1  The coefficients of the spectral model are now scaled so that the output 
     is given in physical units: W / (m^-2 * sr * nm). Also, the output of the   
     XYZ model is now no longer scaled to the range [0...1]. Instead, it is
     the result of a simple conversion from spectral data via the CIE 2 degree
     standard observer matching functions. Therefore, after multiplication 
     with 683 lm / W, the Y channel now corresponds to luminance in lm.
     
1.0  Initial release (May 11th, 2012).


Please visit http://cgg.mff.cuni.cz/projects/SkylightModelling/ to check if
an updated version of this code has been published!

============================================================================ */

/*

All instructions on how to use this code are to be found in the accompanying
header file.

*/

#include "luxrays/core/color/spectrumwavelengths.h"
#include "luxrays/core/color/spds/regular.h"
using luxrays::RegularSPD;

#include "sky2.h"
#include "memory.h"
#include "paramset.h"
#include "bxdf.h"
#include "singlebsdf.h"
#include "primitive.h"
#include "sampling.h"
#include "scene.h"
#include "dynload.h"

#include "data/ArHosekSkyModelData.h"
#include "luxrays/utils/mc.h"

using namespace lux;
using namespace luxrays;

// internal functions

static float RiCosBetween(const Vector &w1, const Vector &w2)
{
	return Clamp(Dot(w1, w2), -1.f, 1.f);
}
static float ComputeCoefficient(const float elevation[6],
	const float parameters[6])
{
	return elevation[0] * parameters[0] +
		5.f * elevation[1] * parameters[1] +
		10.f * elevation[2] * parameters[2] +
		10.f * elevation[3] * parameters[3] +
		5.f * elevation[4] * parameters[4] +
		elevation[5] * parameters[5];
}

static float ComputeInterpolatedCoefficient(u_int index, u_int wavelength,
	float turbidity, float albedo, const float elevation[6])
{
	const u_int lowTurbidity = Floor2UInt(Clamp(turbidity - 1.f, 0.f, 9.f));
	const u_int highTurbidity = min(lowTurbidity + 1U, 9U);
	const float turbidityLerp = Clamp(turbidity - highTurbidity, 0.f, 1.f);

	return Lerp(Clamp(albedo, 0.f, 1.f),
		Lerp(turbidityLerp,
		ComputeCoefficient(elevation, datasets[wavelength][0][lowTurbidity][index]),
		ComputeCoefficient(elevation, datasets[wavelength][0][highTurbidity][index])),
		Lerp(turbidityLerp,
		ComputeCoefficient(elevation, datasets[wavelength][1][lowTurbidity][index]),
		ComputeCoefficient(elevation, datasets[wavelength][1][highTurbidity][index])));
}

static void ComputeModel(float turbidity, float albedo[11], float elevation,
	RegularSPD *SkyModel[10])
{
	const float normalizedElevation = powf(elevation * 2.f * INV_PI, 1.f / 3.f);
	const float elevations[6] = {
		powf(1.f - normalizedElevation, 5.f),
		powf(1.f - normalizedElevation, 4.f) * normalizedElevation,
		powf(1.f - normalizedElevation, 3.f) * powf(normalizedElevation, 2.f),
		powf(1.f - normalizedElevation, 2.f) * powf(normalizedElevation, 3.f),
		(1.f - normalizedElevation) * powf(normalizedElevation, 4.f),
		powf(normalizedElevation, 5.f)
	};

	float values[11];
	for (u_int i = 0; i < 10; ++i) {
		for (u_int wl = 0; wl < 11; ++wl)
			values[wl] = ComputeInterpolatedCoefficient(i, wl, turbidity, albedo[wl], elevations);
		SkyModel[i] = new RegularSPD(values, 320.f, 720.f, 11);
	}
}

static void ComputeRadiance(const RegularSPD * const SkyModel[10], const Vector &sundir,
	const Vector &w, const SpectrumWavelengths &sw, SWCSpectrum *r)
{
	const float cosG = RiCosBetween(w, sundir);
	const float cosG2 = cosG * cosG;
	const float gamma = acosf(cosG);
	const float cosT = max(0.f, CosTheta(w));
	int bins[WAVELENGTH_SAMPLES];
	float offsets[WAVELENGTH_SAMPLES];
	SkyModel[0]->Offsets(WAVELENGTH_SAMPLES, sw.w, bins, offsets);
	SWCSpectrum a, b, c, d, e, f, g, h, i, radiance;
	SkyModel[0]->Sample(WAVELENGTH_SAMPLES, bins, offsets, a.c);
	SkyModel[1]->Sample(WAVELENGTH_SAMPLES, bins, offsets, b.c);
	SkyModel[2]->Sample(WAVELENGTH_SAMPLES, bins, offsets, c.c);
	SkyModel[3]->Sample(WAVELENGTH_SAMPLES, bins, offsets, d.c);
	SkyModel[4]->Sample(WAVELENGTH_SAMPLES, bins, offsets, e.c);
	SkyModel[5]->Sample(WAVELENGTH_SAMPLES, bins, offsets, f.c);
	SkyModel[6]->Sample(WAVELENGTH_SAMPLES, bins, offsets, g.c);
	SkyModel[7]->Sample(WAVELENGTH_SAMPLES, bins, offsets, h.c);
	SkyModel[8]->Sample(WAVELENGTH_SAMPLES, bins, offsets, i.c);
	SkyModel[9]->Sample(WAVELENGTH_SAMPLES, bins, offsets, radiance.c);

	const SWCSpectrum expTerm(d * Exp(e * gamma));
	const SWCSpectrum rayleighTerm(f * cosG2);
	const SWCSpectrum mieTerm(g * (1.f + cosG2) /
		Pow(SWCSpectrum(1.f) + i * (i - SWCSpectrum(2.f * cosG)), 1.5f));
	const SWCSpectrum zenithTerm(h * sqrtf(cosT));
	*r *= (SWCSpectrum(1.f) + a * Exp(b / (cosT + .01f))) *
		(c + expTerm + rayleighTerm + mieTerm + zenithTerm) * radiance;
}

static float ComputeY(const RegularSPD * const SkyModel[10], const Vector &sundir,
	const Vector &w)
{
	const float cosG = RiCosBetween(w, sundir);
	const float cosG2 = cosG * cosG;
	const float gamma = acosf(cosG);
	const float cosT = max(0.f, CosTheta(w));
	const float a = SkyModel[0]->Filter();
	const float b = SkyModel[1]->Filter();
	const float c = SkyModel[2]->Filter();
	const float d = SkyModel[3]->Filter();
	const float e = SkyModel[4]->Filter();
	const float f = SkyModel[5]->Filter();
	const float g = SkyModel[6]->Filter();
	const float h = SkyModel[7]->Filter();
	const float i = SkyModel[8]->Filter();
	const float radiance = SkyModel[9]->Y();

	const float expTerm = d * expf(e * gamma);
	const float rayleighTerm = f * cosG2;
	const float mieTerm = g * (1.f + cosG2) /
		powf(1.f + i * (i - 2.f * cosG), 1.5f);
	const float zenithTerm = h * sqrtf(cosT);
	return (1.f + a * expf(b / (cosT + .01f))) *
		(c + expTerm + rayleighTerm + mieTerm + zenithTerm) * radiance;
}

class  Sky2BSDF : public BSDF  {
public:
	// Sky2BSDF Public Methods
	Sky2BSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior,
		const Sky2Light &l, const Transform &LW) :
		BSDF(dgs, ngeom, exterior, interior), light(l),
		LightToWorld(LW) { }
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
		*wiW = Normalize(LightToWorld * wi);
		if (sampledType)
			*sampledType = BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE);
		*pdf = cosi * INV_PI;
		if (pdfBack)
			*pdfBack = 0.f;
		*f_ = SWCSpectrum(M_PI);
		ComputeRadiance(light.model, light.sundir, Normalize(-wi), sw, f_);
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
			const Vector w(Normalize(Inverse(LightToWorld) * -wiW));
			SWCSpectrum L(cosi);
			ComputeRadiance(light.model, light.sundir, w, sw, &L);
			return L;
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
	// Sky2BSDF Private Methods
	virtual ~Sky2BSDF() { }
	const Sky2Light &light;
	const Transform &LightToWorld;
};
class  Sky2PortalBSDF : public BSDF  {
public:
	// Sky2PortalBSDF Public Methods
	Sky2PortalBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior,
		const Sky2Light &l, const Transform &LW,
		const Point &p,
		const vector<boost::shared_ptr<Primitive> > &portalList,
		u_int portal) :
		BSDF(dgs, ngeom, exterior, interior), light(l),
		LightToWorld(LW), ps(p), PortalShapes(portalList),
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
		*wiW = Normalize(dg.p - ps);
		const float cosi = Dot(*wiW, ng);
		if (!(cosi > 0.f))
			return false;
		const Vector w(Normalize(Inverse(LightToWorld) * -(*wiW)));
		*f_ = SWCSpectrum(cosi);
		ComputeRadiance(light.model, light.sundir, w, sw, f_);
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
		*f_ /= *pdf;
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
		if (NumComponents(flags) == 1 && cosi > 0.f) {
			const Vector w(Normalize(Inverse(LightToWorld) * -wiW));
			SWCSpectrum L(cosi);
			ComputeRadiance(light.model, light.sundir, w, sw, &L);
			return L;
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
	// Sky2PortalBSDF Private Methods
	virtual ~Sky2PortalBSDF() { }
	const Sky2Light &light;
	const Transform &LightToWorld;
	Point ps;
	const vector<boost::shared_ptr<Primitive> > &PortalShapes;
	u_int shapeIndex;
};

// Sky2Light Method Definitions
Sky2Light::~Sky2Light()
{
	for (u_int i = 0; i < 10; ++i)
		delete model[i];
}

Sky2Light::Sky2Light(const Transform &light2world, float skyscale, u_int ns,
	Vector sd, float turb)
	: Light("Sky2Light-" + boost::lexical_cast<string>(this), light2world, ns) {
	skyScale = skyscale;
	sundir = sd;
	turbidity = turb;
	float albedo[11] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
	for (u_int i = 0; i < 10; ++i)
		model[i] = NULL;

	ComputeModel(turbidity, albedo, M_PI * .5f - SphericalTheta(sd), model);

	AddFloatAttribute(*this, "dir.x", "Sky light direction X", &Sky2Light::GetDirectionX);
	AddFloatAttribute(*this, "dir.y", "Sky light direction Y", &Sky2Light::GetDirectionY);
	AddFloatAttribute(*this, "dir.z", "Sky light direction Z", &Sky2Light::GetDirectionZ);
	AddFloatAttribute(*this, "turbidity", "Sky light turbidity", &Sky2Light::turbidity);
	AddFloatAttribute(*this, "gain", "Sky light gain", &Sky2Light::skyScale);
}

float Sky2Light::Power(const Scene &scene) const
{
	Point worldCenter;
	float worldRadius;
	scene.WorldBound().BoundingSphere(&worldCenter, &worldRadius);

	const u_int steps = 100;
	const float deltaStep = 1.f / steps;
	float power = 0.f;
	for (u_int i = 0; i < steps; ++i) {
		for (u_int j = 0; j < steps; ++j) {
			power += ComputeY(model, sundir, UniformSampleSphere(i * deltaStep + deltaStep / 2.f, j * deltaStep + deltaStep / 2.f));
		}
	}
	power /= steps * steps;

	return power * (havePortalShape ? PortalArea : 4.f * M_PI * worldRadius * worldRadius) * 2.f * M_PI;
}

bool Sky2Light::Le(const Scene &scene, const Sample &sample, const Ray &r,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *L) const
{
	Point worldCenter;
	float worldRadius;
	scene.WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	const Vector toCenter(worldCenter - r.o);
	const float centerDistance = Dot(toCenter, toCenter);
	const float approach = Dot(toCenter, r.d);
	const float distance = approach + sqrtf(max(0.f, worldRadius * worldRadius -
		centerDistance + approach * approach));
	const Point ps(r.o + distance * r.d);
	const Normal ns(Normalize(worldCenter - ps));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Normal(0, 0, 0),
		Normal(0, 0, 0), 0, 0, NULL);
	dg.time = sample.realTime;
	const Volume *v = GetVolume();
	if (!havePortalShape) {
		*bsdf = ARENA_ALLOC(sample.arena, Sky2BSDF)(dg, ns,
			v, v, *this, LightToWorld);
		if (pdf)
			*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
		if (pdfDirect)
			*pdfDirect = AbsDot(r.d, ns) /
			(4.f * M_PI * DistanceSquared(r.o, ps));
	} else {
		*bsdf = ARENA_ALLOC(sample.arena, Sky2PortalBSDF)(dg, ns,
			v, v, *this, LightToWorld, ps, PortalShapes, ~0U);
		if (pdf)
			*pdf = 0.f;
		if (pdfDirect)
			*pdfDirect = 0.f;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			if (pdf) {
				PortalShapes[i]->Sample(.5f, .5f, .5f, &dg);
				const Vector w(dg.p - ps);
				if (Dot(w, dg.nn) > 0.f) {
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
						isect.dg.p) / AbsDot(r.d, isect.dg.nn);
			}
		}
		if (pdf)
			*pdf *= INV_TWOPI / nrPortalShapes;
		if (pdfDirect)
			*pdfDirect *= AbsDot(r.d, ns) /
				(DistanceSquared(r.o, ps) * nrPortalShapes);
	}
	const Vector wh(Normalize(Inverse(LightToWorld) * r.d));
	ComputeRadiance(model, sundir, wh, sample.swl, L);
	*L *= skyScale;
	return true;
}

float Sky2Light::Pdf(const Point &p, const PartialDifferentialGeometry &dg) const
{
	const Vector wi(dg.p - p);
	if (!havePortalShape) {
		const float d2 = wi.LengthSquared();
		return AbsDot(wi, dg.nn) / (4.f * M_PI * sqrtf(d2) * d2);
	} else {
		const float d2 = wi.LengthSquared();
		float pdf = 0.f;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			Intersection isect;
			Ray ray(p, wi);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect) &&
				Dot(wi, isect.dg.nn) < 0.f)
				pdf += PortalShapes[i]->Pdf(p, isect.dg) *
					DistanceSquared(p, isect.dg.p) /
					AbsDot(wi, isect.dg.nn);
		}
		pdf *= AbsDot(wi, dg.nn) /
			(d2 * d2 * nrPortalShapes);
		return pdf;
	}
}

bool Sky2Light::SampleL(const Scene &scene, const Sample &sample,
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
		*bsdf = ARENA_ALLOC(sample.arena, Sky2BSDF)(dg, ns,
			v, v, *this, LightToWorld);
		*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	} else {
		// Sample a random Portal
		u_int shapeIndex = 0;
		if (nrPortalShapes > 1) {
			u3 *= nrPortalShapes;
			shapeIndex = min(nrPortalShapes - 1, Floor2UInt(u3));
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
			sqrtf(max(0.f, worldRadius * worldRadius - centerDistance +
			approach * approach));
		const Point ps(dgs.p + distance * wi);
		const Normal ns(Normalize(worldCenter - ps));
		Vector dpdu, dpdv;
		CoordinateSystem(Vector(ns), &dpdu, &dpdv);
		DifferentialGeometry dg(ps, ns, dpdu, dpdv, Normal(0, 0, 0),
			Normal (0, 0, 0), 0, 0, NULL);
		dg.time = sample.realTime;
		*bsdf = ARENA_ALLOC(sample.arena, Sky2PortalBSDF)(dg, ns,
			v, v, *this, LightToWorld, ps, PortalShapes, shapeIndex);
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
	*Le = SWCSpectrum(skyScale / *pdf);
	return true;
}
bool Sky2Light::SampleL(const Scene &scene, const Sample &sample,
	const Point &p, float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	float *pdfDirect, SWCSpectrum *Le) const
{
	Vector wi;
	u_int shapeIndex = 0;
	Point worldCenter;
	float worldRadius;
	scene.WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	if (!havePortalShape) {
		// Sample uniform direction on unit sphere
		wi = UniformSampleSphere(u1, u2);
		// Compute _pdf_ for cosine-weighted infinite light direction
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
		if (!(*pdfDirect > 0.f))
			return false;
		Point ps = dg.p;
		wi = Normalize(ps - p);
		if (!(Dot(wi, dg.nn) < 0.f))
			return false;
		*pdfDirect *= DistanceSquared(p, ps) / AbsDot(wi, dg.nn);
	}
	const Vector toCenter(worldCenter - p);
	const float centerDistance = Dot(toCenter, toCenter);
	const float approach = Dot(toCenter, wi);
	const float distance = approach + sqrtf(max(0.f, worldRadius * worldRadius -
		centerDistance + approach * approach));
	const Point ps(p + distance * wi);
	const Normal ns(Normalize(worldCenter - ps));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Normal(0, 0, 0),
		Normal(0, 0, 0), 0, 0, NULL);
	dg.time = sample.realTime;
	const Volume *v = GetVolume();
	if (!havePortalShape) {
		*bsdf = ARENA_ALLOC(sample.arena, Sky2BSDF)(dg, ns,
			v, v, *this, LightToWorld);
		if (pdf)
			*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	} else {
		*bsdf = ARENA_ALLOC(sample.arena, Sky2PortalBSDF)(dg, ns,
			v, v, *this, LightToWorld, ps, PortalShapes, shapeIndex);
		if (pdf)
			*pdf = 0.f;
		DifferentialGeometry dgs;
		dgs.time = sample.realTime;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			if (pdf) {
				PortalShapes[i]->Sample(.5f, .5f, .5f, &dgs);
				Vector w(ps - dgs.p);
				if (Dot(wi, dgs.nn) < 0.f) {
					float distance = w.LengthSquared();
					*pdf += AbsDot(ns, w) / (sqrtf(distance) * distance);
				}
			}
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
		if (pdf)
			*pdf *= INV_TWOPI / nrPortalShapes;
		*pdfDirect /= nrPortalShapes;
	}
	*pdfDirect *= AbsDot(wi, ns) / (distance * distance);
	*Le = SWCSpectrum(skyScale / *pdfDirect);
	return true;
}

Light* Sky2Light::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
	float scale = paramSet.FindOneFloat("gain", 1.f);				// gain (aka scale) factor to apply to sun/skylight (0.005)
	int nSamples = paramSet.FindOneInt("nsamples", 1);
	Vector sundir = paramSet.FindOneVector("sundir", Vector(0,0,1));	// direction vector of the sun
	Normalize(sundir);
	float turb = paramSet.FindOneFloat("turbidity", 2.0f);			// [in] turb  Turbidity (1.0,10) 2-6 are most useful for clear days.

	Sky2Light *l = new Sky2Light(light2world, scale, nSamples, sundir, turb);
	l->hints.InitParam(paramSet);
	return l;
}

static DynamicLoader::RegisterLight<Sky2Light> r("sky2");
