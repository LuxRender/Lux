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

// sky.cpp*
#include "sky.h"
#include "memory.h"
#include "mc.h"
#include "spectrumwavelengths.h"
#include "paramset.h"
#include "regular.h"
#include "bxdf.h"
#include "singlebsdf.h"
#include "primitive.h"
#include "sampling.h"
#include "scene.h"
#include "dynload.h"

#include "data/skychroma_spect.h"

using namespace lux;

class  SkyBSDF : public BSDF  {
public:
	// SkyBSDF Public Methods
	SkyBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior,
		const SkyLight &l, const Transform &WL) :
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
		*f_ = SWCSpectrum(M_PI);
		light.GetSkySpectralRadiance(sw, Normalize(-wi), f_);
		return true;
	}
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1 &&
			Dot(wiW, ng) > 0.f && Dot(wiW, nn) > 0.f)
			return AbsDot(wiW, nn) * INV_PI;
		return 0.f;
	}
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse, BxDFType flags = BSDF_ALL) const {
		const float cosi = Dot(wiW, ng);
		if (NumComponents(flags) == 1 && cosi > 0.f) {
			const Vector w(Normalize(WorldToLight(-wiW)));
			SWCSpectrum L(cosi);
			light.GetSkySpectralRadiance(sw, w, &L);
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
	// SkyBSDF Private Methods
	virtual ~SkyBSDF() { }
	const SkyLight &light;
	const Transform &WorldToLight;
};
class  SkyPortalBSDF : public BSDF  {
public:
	// SkyPortalBSDF Public Methods
	SkyPortalBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior,
		const SkyLight &l, const Transform &WL,
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
		*wiW = Normalize(dg.p - ps);
		const float cosi = Dot(*wiW, ng);
		if (!(cosi > 0.f))
			return false;
		const Vector w(Normalize(WorldToLight(-(*wiW))));
		*f_ = SWCSpectrum(cosi);
		light.GetSkySpectralRadiance(sw, w, f_);
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
		if (NumComponents(flags) == 0 && !(Dot(wiW, nn) > 0.f))
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
			const Vector w(Normalize(WorldToLight(-wiW)));
			SWCSpectrum L(cosi);
			light.GetSkySpectralRadiance(sw, w, &L);
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
	// SkyPortalBSDF Private Methods
	virtual ~SkyPortalBSDF() { }
	const SkyLight &light;
	const Transform &WorldToLight;
	Point ps;
	const vector<boost::shared_ptr<Primitive> > &PortalShapes;
	u_int shapeIndex;
};

static float PerezBase(const float lam[6], float theta, float gamma)
{
	return (1.f + lam[1] * expf(lam[2] / cosf(theta))) *
		(1.f + lam[3] * expf(lam[4] * gamma)  + lam[5] * cosf(gamma) * cosf(gamma));
}

/* All angles in radians, theta angles measured from normal */
inline float RiAngleBetween(float thetav, float phiv, float theta, float phi)
{
	const float cospsi = sinf(thetav) * sinf(theta) * cosf(phi - phiv) + cosf(thetav) * cosf(theta);
	if (cospsi >= 1.f)
		return 0.f;
	if (cospsi <= -1.f)
		return M_PI;
	return acosf(cospsi);
}
inline float RiAngleBetween(const Vector &w1, const Vector &w2)
{
	const float cospsi = Dot(w1, w2);
	if (cospsi >= 1.f)
		return 0.f;
	if (cospsi <= -1.f)
		return M_PI;
	return acosf(cospsi);
}

static const RegularSPD S0(S0Amplitudes, 300.f, 830.f, 54);
static const RegularSPD S1(S1Amplitudes, 300.f, 830.f, 54);
static const RegularSPD S2(S2Amplitudes, 300.f, 830.f, 54);
static const float S0Y = S0.Y();
static const float S1Y = S1.Y();
static const float S2Y = S2.Y();

// SkyLight Method Definitions
SkyLight::~SkyLight()
{
}

SkyLight::SkyLight(const Transform &light2world, float skyscale, u_int ns,
	Vector sd, float turb,
	float aconst, float bconst, float cconst, float dconst, float econst, bool sup)
	: Light(light2world, ns) {
	skyScale = skyscale;
	sundir = sd;
	turbidity = turb;

	InitSunThetaPhi();

	float theta2 = thetaS*thetaS;
	float theta3 = theta2*thetaS;
	float T = turb;
	float T2 = turb*turb;

	float chi = (4.f / 9.f - T / 120.f) * (M_PI - 2 * thetaS);
	zenith_Y = (4.0453 * T - 4.9710) * tan(chi) - .2155 * T + 2.4192;
	zenith_Y *= 1000;  // conversion from kcd/m^2 to cd/m^2

	zenith_x =
	(0.00166f * theta3 - 0.00375f * theta2 + 0.00209f * thetaS) * T2 +
	(-0.02903f * theta3 + 0.06377f * theta2 - 0.03202f * thetaS + 0.00394f) * T +
	(0.11693f * theta3 - 0.21196f * theta2 + 0.06052f * thetaS + 0.25886f);

	zenith_y =
	(0.00275f * theta3 - 0.00610f * theta2 + 0.00317f * thetaS) * T2 +
	(-0.04214f * theta3 + 0.08970f * theta2 - 0.04153f * thetaS  + 0.00516f) * T +
	(0.15346f * theta3 - 0.26756f * theta2 + 0.06670f * thetaS  + 0.26688f);

	perez_Y[1] = (0.1787f * T  - 1.4630f) * aconst;
	perez_Y[2] = (-0.3554f * T  + 0.4275f) * bconst;
	perez_Y[3] = (-0.0227f * T  + 5.3251f) * cconst;
	perez_Y[4] = (0.1206f * T  - 2.5771f) * dconst;
	perez_Y[5] = (-0.0670f * T  + 0.3703f) * econst;

	perez_x[1] = (-0.0193f * T  - 0.2592f) * aconst;
	perez_x[2] = (-0.0665f * T  + 0.0008f) * bconst;
	perez_x[3] = (-0.0004f * T  + 0.2125f) * cconst;
	perez_x[4] = (-0.0641f * T  - 0.8989f) * dconst;
	perez_x[5] = (-0.0033f * T  + 0.0452f) * econst;

	perez_y[1] = (-0.0167f * T  - 0.2608f) * aconst;
	perez_y[2] = (-0.0950f * T  + 0.0092f) * bconst;
	perez_y[3] = (-0.0079f * T  + 0.2102f) * cconst;
	perez_y[4] = (-0.0441f * T  - 1.6537f) * dconst;
	perez_y[5] = (-0.0109f * T  + 0.0529f) * econst;

	zenith_Y /= PerezBase(perez_Y, 0, thetaS);
	zenith_x /= PerezBase(perez_x, 0, thetaS);
	zenith_y /= PerezBase(perez_y, 0, thetaS);
}

float SkyLight::Power(const Scene &scene) const
{
	Point worldCenter;
	float worldRadius;
	scene.WorldBound().BoundingSphere(&worldCenter, &worldRadius);

	const u_int steps = 100;
	const float deltaStep = 2.f / steps;
	float phi = 0.f, power = 0.f;
	for (u_int i = 0; i < steps; ++i) {
		float cosTheta = -1.f;
		for (u_int j = 0; j < steps; ++j) {
			float theta = acosf(cosTheta);
			float gamma = RiAngleBetween(theta, phi, thetaS, phiS);
			theta = min(theta, M_PI * .5f - .001f);
			power += zenith_Y * PerezBase(perez_Y, theta, gamma);
			cosTheta += deltaStep;
		}

		phi += deltaStep * M_PI;
	}
	power /= steps * steps;

	return power * (havePortalShape ? PortalArea : 4.f * M_PI * worldRadius * worldRadius) * 2.f * M_PI;
}

bool SkyLight::Le(const Scene &scene, const Sample &sample, const Ray &r,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *L) const
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
		*bsdf = ARENA_ALLOC(sample.arena, SkyBSDF)(dg, ns,
			v, v, *this, WorldToLight);
		if (pdf)
			*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
		if (pdfDirect)
			*pdfDirect = AbsDot(r.d, ns) /
			(4.f * M_PI * DistanceSquared(r.o, ps));
	} else {
		*bsdf = ARENA_ALLOC(sample.arena, SkyPortalBSDF)(dg, ns,
			v, v, *this, WorldToLight, ps, PortalShapes, ~0U);
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
	const Vector wh(Normalize(WorldToLight(r.d)));
	GetSkySpectralRadiance(sample.swl, wh, L);
	*L *= skyScale;
	return true;
}

float SkyLight::Pdf(const Point &p, const DifferentialGeometry &dg) const
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

bool SkyLight::SampleL(const Scene &scene, const Sample &sample,
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
		*bsdf = ARENA_ALLOC(sample.arena, SkyBSDF)(dg, ns,
			v, v, *this, WorldToLight);
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
			sqrtf(worldRadius * worldRadius - centerDistance +
			approach * approach);
		const Point ps(dgs.p + distance * wi);
		const Normal ns(Normalize(worldCenter - ps));
		Vector dpdu, dpdv;
		CoordinateSystem(Vector(ns), &dpdu, &dpdv);
		DifferentialGeometry dg(ps, ns, dpdu, dpdv, Normal(0, 0, 0),
			Normal (0, 0, 0), 0, 0, NULL);
		dg.time = sample.realTime;
		*bsdf = ARENA_ALLOC(sample.arena, SkyPortalBSDF)(dg, ns,
			v, v, *this, WorldToLight, ps, PortalShapes, shapeIndex);
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
bool SkyLight::SampleL(const Scene &scene, const Sample &sample,
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
	const float distance = approach + sqrtf(worldRadius * worldRadius -
		centerDistance + approach * approach);
	const Point ps(p + distance * wi);
	const Normal ns(Normalize(worldCenter - ps));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Normal(0, 0, 0),
		Normal(0, 0, 0), 0, 0, NULL);
	dg.time = sample.realTime;
	const Volume *v = GetVolume();
	if (!havePortalShape) {
		*bsdf = ARENA_ALLOC(sample.arena, SkyBSDF)(dg, ns,
			v, v, *this, WorldToLight);
		if (pdf)
			*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	} else {
		*bsdf = ARENA_ALLOC(sample.arena, SkyPortalBSDF)(dg, ns,
			v, v, *this, WorldToLight, ps, PortalShapes, shapeIndex);
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

Light* SkyLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
	bool sup = paramSet.FindOneBool("support", false);
	float scale = paramSet.FindOneFloat("gain", 1.f);				// gain (aka scale) factor to apply to sun/skylight (0.005)
	int nSamples = paramSet.FindOneInt("nsamples", 1);
	Vector sundir = paramSet.FindOneVector("sundir", Vector(0,0,1));	// direction vector of the sun
	Normalize(sundir);
	float turb = paramSet.FindOneFloat("turbidity", 2.0f);			// [in] turb  Turbidity (1.0,30+) 2-6 are most useful for clear days.
	// Perez function multiplicative constants
	float aconst = paramSet.FindOneFloat("aconst",
		paramSet.FindOneFloat("horizonbrightness", 1.0f));
	float bconst = paramSet.FindOneFloat("bconst",
		paramSet.FindOneFloat("horizonsize", 1.0f));
	float cconst = paramSet.FindOneFloat("cconst",
		paramSet.FindOneFloat("sunhalobrightness", 1.0f));
	float dconst = paramSet.FindOneFloat("dconst",
		paramSet.FindOneFloat("sunhalosize", 1.0f));
	float econst = paramSet.FindOneFloat("econst",
		paramSet.FindOneFloat("backscattering", 1.0f));

	SkyLight *l = new SkyLight(light2world, scale, nSamples, sundir, turb, aconst, bconst, cconst, dconst, econst, sup);
	l->hints.InitParam(paramSet);
	return l;
}

/**********************************************************
// South = x,  East = y, up = z
// All times in decimal form (6.25 = 6:15 AM)
// All angles in Radians
// From IES Lighting Handbook pg 361
// ********************************************************/

void SkyLight::InitSunThetaPhi()
{
	Vector wh = Normalize(sundir);
	phiS = SphericalPhi(wh);
	thetaS = SphericalTheta(wh);
}

/**********************************************************
// Sky Radiance
//
// ********************************************************/


// note - lyc - optimised return call to not create temporaries, passed in scale factor
void SkyLight::GetSkySpectralRadiance(const SpectrumWavelengths &sw,
	const Vector &w, SWCSpectrum * const dst_spect) const
{
	// add bottom half of hemisphere with horizon colour
	const float theta = min(SphericalTheta(w), (M_PI * .5f) - .001f);
	const float gamma = RiAngleBetween(w, sundir);

	// Compute xyY values
	const float x = zenith_x * PerezBase(perez_x, theta, gamma);
	const float y = zenith_y * PerezBase(perez_y, theta, gamma);
	const float Y = zenith_Y * PerezBase(perez_Y, theta, gamma);

	ChromaticityToSpectrum(sw, x, y, dst_spect);
	*dst_spect *= Y;
}

// note - lyc - removed redundant computations and optimised
void SkyLight::ChromaticityToSpectrum(const SpectrumWavelengths &sw,
	const float x, const float y, SWCSpectrum * const dst_spect) const
{
	const float den = 1.0f / (0.0241f + 0.2562f * x - 0.7341f * y);
	const float M1 = (-1.3515f - 1.7703f * x + 5.9114f * y) * den;
	const float M2 = (0.03f - 31.4424f * x + 30.0717f * y) * den;

	SWCSpectrum s0, s1, s2;
	S0.Sample(WAVELENGTH_SAMPLES, sw.w, s0.c);
	S1.Sample(WAVELENGTH_SAMPLES, sw.w, s1.c);
	S2.Sample(WAVELENGTH_SAMPLES, sw.w, s2.c);
	*dst_spect *= (s0 + M1 * s1 + M2 * s2) / (S0Y + M1 * S1Y + M2 * S2Y);
}

static DynamicLoader::RegisterLight<SkyLight> r("sky");
