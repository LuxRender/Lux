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
#include "mc.h"
#include "spectrumwavelengths.h"
#include "paramset.h"
#include "regular.h"
#include "reflection/bxdf.h"
#include "dynload.h"

#include "data/skychroma_spect.h"

using namespace lux;

class SkyBxDF : public BxDF
{
public:
	SkyBxDF(const SkyLight &sky, const Transform &WL,
		const Vector &x, const Vector &y, const Vector &z) :
		BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), skyLight(sky),
		WorldToLight(WL), X(x), Y(y), Z(z) { }
	virtual ~SkyBxDF() { }
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi,
		SWCSpectrum *const f) const {
		Vector w(wi.x * X + wi.y * Y + wi.z * Z);
		w = Normalize(WorldToLight(-w));
		const float phi = SphericalPhi(w);
		const float theta = SphericalTheta(w);
		SWCSpectrum L;
		skyLight.GetSkySpectralRadiance(tspack, theta, phi, &L);
		*f += L;
	}
private:
	const SkyLight &skyLight;
	const Transform &WorldToLight;
	Vector X, Y, Z;
};

class SkyPortalBxDF : public BxDF
{
public:
	SkyPortalBxDF(const SkyLight &sky, const Transform &WL,
		const Vector &x, const Vector &y, const Vector &z,
		const Point &p,
		const vector<boost::shared_ptr<Primitive> > &portalList,
		u_int portal, float u) :
		BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), skyLight(sky),
		WorldToLight(WL), X(x), Y(y), Z(z), ps(p),
		PortalShapes(portalList), shapeIndex(portal), u3(u) { }
	virtual ~SkyPortalBxDF() { }
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const {
		if (shapeIndex == ~0U || reverse)
			return false;
		DifferentialGeometry dg;
		dg.time = tspack->time;
		PortalShapes[shapeIndex]->Sample(tspack, ps, u1, u2, u3, &dg);
		Vector wiW = Normalize(dg.p - ps);
		Vector w = Normalize(WorldToLight(-wiW));
		const float phi = SphericalPhi(w);
		const float theta = SphericalTheta(w);
		skyLight.GetSkySpectralRadiance(tspack, theta, phi, f);
		wi->x = Dot(wiW, X);
		wi->y = Dot(wiW, Y);
		wi->z = Dot(wiW, Z);
		*wi = Normalize(*wi);
		*pdf = PortalShapes[shapeIndex]->Pdf(ps, dg.p) *
			DistanceSquared(ps, dg.p) / AbsDot(wiW, dg.nn);
		for (u_int i = 0; i < PortalShapes.size(); ++i) {
			if (i == shapeIndex)
				continue;
			Intersection isect;
			RayDifferential ray(ps, wiW);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect) &&
				Dot(wiW, isect.dg.nn) > 0.f)
				*pdf += PortalShapes[i]->Pdf(ps, isect.dg.p) *
					DistanceSquared(ps, isect.dg.p) /
					AbsDot(wiW, isect.dg.nn);
		}
		*pdf /= PortalShapes.size();
		if (pdfBack)
			*pdfBack = 0.f;
		return true;
	}
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi,
		SWCSpectrum *const f) const {
		const Vector w(wi.x * X + wi.y * Y + wi.z * Z);
		const Vector wh(Normalize(WorldToLight(-w)));
		const float phi = SphericalPhi(wh);
		const float theta = SphericalTheta(wh);
		SWCSpectrum L;
		skyLight.GetSkySpectralRadiance(tspack, theta, phi, &L);
		*f += L;
	}
	virtual float Pdf(const TsPack *tspack, const Vector &wi,
		const Vector &wo) const {
		const Vector w(wo.x * X + wo.y * Y + wo.z * Z);
		float pdf = 0.f;
		for (u_int i = 0; i < PortalShapes.size(); ++i) {
			Intersection isect;
			RayDifferential ray(ps, w);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect) &&
				Dot(w, isect.dg.nn) > 0.f)
				pdf += PortalShapes[i]->Pdf(ps, isect.dg.p) *
					DistanceSquared(ps, isect.dg.p) /
					AbsDot(w, isect.dg.nn);
		}
		return pdf / PortalShapes.size();
	}
private:
	const SkyLight &skyLight;
	const Transform &WorldToLight;
	Vector X, Y, Z;
	Point ps;
	const vector<boost::shared_ptr<Primitive> > &PortalShapes;
	u_int shapeIndex;
	float u3;
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

static const RegularSPD S0(S0Amplitudes, 300.f, 830.f, 54);
static const RegularSPD S1(S1Amplitudes, 300.f, 830.f, 54);
static const RegularSPD S2(S2Amplitudes, 300.f, 830.f, 54);
static const float S0Y = S0.Y();
static const float S1Y = S1.Y();
static const float S2Y = S2.Y();

// SkyLight Method Definitions
SkyLight::~SkyLight() {
}

SkyLight::SkyLight(const Transform &light2world,
		                const float skyscale, u_int ns, Vector sd, float turb,
										float aconst, float bconst, float cconst, float dconst, float econst)
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

float SkyLight::Power(const Scene *scene) const
{
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	float cosTheta = -1.f, phi = 0.f, power = 0.f;
	for (u_int i = 0; i < 100; ++i) {
		for (u_int j = 0; j < 100; ++j) {
			float theta = acosf(cosTheta);
			float gamma = RiAngleBetween(theta, phi, thetaS, phiS);
			theta = min(theta, M_PI * .5f - .001f);
			power += zenith_Y * PerezBase(perez_Y, theta, gamma);
			cosTheta += .02f;
			phi += .02f * M_PI;
		}
	}
	return power * (havePortalShape ? PortalArea : 4.f * M_PI * worldRadius * worldRadius) * 2.f * M_PI;
}

SWCSpectrum SkyLight::Le(const TsPack *tspack, const RayDifferential &r) const {
	Vector w = r.d;
	// Compute sky light radiance for direction

	Vector wh = Normalize(WorldToLight(w));
	const float phi = SphericalPhi(wh);
	const float theta = SphericalTheta(wh);

	SWCSpectrum L;
	GetSkySpectralRadiance(tspack, theta, phi, &L);
	L *= skyScale;

	return L;
}
SWCSpectrum SkyLight::Le(const TsPack *tspack, const Scene *scene, const Ray &r,
	const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const
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
		*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns,
			ARENA_ALLOC(tspack->arena, SkyBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns)));
		*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
		*pdfDirect = AbsDot(r.d, n) * INV_TWOPI * AbsDot(r.d, ns) /
			DistanceSquared(r.o, ps);
	} else {
		*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns,
			ARENA_ALLOC(tspack->arena, SkyPortalBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns), ps, PortalShapes, ~0U, 0.f));
		*pdf = 0.f;
		*pdfDirect = 0.f;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			PortalShapes[i]->Sample(.5f, .5f, .5f, &dg);
			const Vector w(dg.p - ps);
			if (Dot(w, dg.nn) > 0.f) {
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
					isect.dg.p) / AbsDot(r.d, isect.dg.nn);
		}
		*pdf *= INV_TWOPI / nrPortalShapes;
		*pdfDirect *= AbsDot(r.d, ns) /
			(DistanceSquared(r.o, ps) * nrPortalShapes);
	}
	const Vector wh = Normalize(WorldToLight(r.d));
	const float phi = SphericalPhi(wh);
	const float theta = SphericalTheta(wh);
	SWCSpectrum L;
	GetSkySpectralRadiance(tspack, theta, phi, &L);
	L *= skyScale;
	return L;
}

SWCSpectrum SkyLight::Sample_L(const TsPack *tspack, const Point &p,
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
		// Sample a random Portal
		u_int shapeIndex = 0;
		if(nrPortalShapes > 1) {
			shapeIndex = min(nrPortalShapes - 1,
					Floor2UInt(u3 * nrPortalShapes));
			u3 *= nrPortalShapes;
			u3 -= shapeIndex;
		}
		DifferentialGeometry dg;
		dg.time = tspack->time;
		PortalShapes[shapeIndex]->Sample(tspack, p, u1, u2, u3, &dg);
		Point ps = dg.p;
		*wi = Normalize(ps - p);
		if (Dot(*wi, dg.nn) < 0.f)
			*pdf = PortalShapes[shapeIndex]->Pdf(p, *wi) / nrPortalShapes;
		else {
			*pdf = 0.f;
			return 0.f;
		}
	}
	visibility->SetRay(p, *wi, tspack->time);
	return Le(tspack, RayDifferential(p, *wi));
}
float SkyLight::Pdf(const TsPack *tspack, const Point &p, const Normal &n,
		const Vector &wi) const {
	if (!havePortalShape)
		return AbsDot(n, wi) * INV_TWOPI;
	else {
		float pdf = 0.f;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
			Intersection isect;
			RayDifferential ray(p, wi);
			if (PortalShapes[i]->Intersect(ray, &isect) && Dot(wi, isect.dg.nn) < .0f)
				pdf += PortalShapes[i]->Pdf(p, wi);
		}
		pdf /= nrPortalShapes;
		return pdf;
	}
}
float SkyLight::Pdf(const TsPack *tspack, const Point &p, const Normal &n,
	const Point &po, const Normal &ns) const
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
					AbsDot(wi, isect.dg.nn);
		}
		pdf *= AbsDot(wi, ns) /
			(DistanceSquared(p, po) * nrPortalShapes);
		return pdf;
	}
}
SWCSpectrum SkyLight::Sample_L(const TsPack *tspack, const Point &p,
		float u1, float u2, float u3, Vector *wi, float *pdf,
		VisibilityTester *visibility) const {
	if(!havePortalShape) {
		*wi = UniformSampleSphere(u1, u2);
		*pdf = UniformSpherePdf();
	} else {
		// Sample a random Portal
		u_int shapeIndex = 0;
		if(nrPortalShapes > 1) {
			shapeIndex = min(nrPortalShapes - 1,
					Floor2UInt(u3 * nrPortalShapes));
			u3 *= nrPortalShapes;
			u3 -= shapeIndex;
		}
		DifferentialGeometry dg;
		dg.time = tspack->time;
		PortalShapes[shapeIndex]->Sample(tspack, p, u1, u2, u3, &dg);
		Point ps = dg.p;
		*wi = Normalize(ps - p);
		if (Dot(*wi, dg.nn) < 0.f)
			*pdf = PortalShapes[shapeIndex]->Pdf( p, *wi) / nrPortalShapes;
		else {
			*pdf = 0.f;
			return 0.f;
		}
	}
	visibility->SetRay(p, *wi, tspack->time);
	return Le(tspack, RayDifferential(p, *wi));
}
float SkyLight::Pdf(const TsPack *tspack, const Point &, const Vector &) const {
	return 1.f / (4.f * M_PI);
}
SWCSpectrum SkyLight::Sample_L(const TsPack *tspack, const Scene *scene,
		float u1, float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
	if (!havePortalShape) {
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
		// Compute _SkyLight_ ray weight
		Vector to_center = Normalize(worldCenter - p1);
		float costheta = AbsDot(to_center,ray->d);
		*pdf = costheta / ((4.f * M_PI * worldRadius * worldRadius));
	} else {
		// Dade - choose a random portal. This strategy is quite bad if there
		// is more than one portal.
		u_int shapeidx = 0;
		if (nrPortalShapes > 1)
			shapeidx = min(nrPortalShapes - 1,
				Floor2UInt(tspack->rng->floatValue() * nrPortalShapes));  // TODO - REFACT - add passed value from sample

		DifferentialGeometry dg;
		dg.time = tspack->time;
		PortalShapes[shapeidx]->Sample(u1, u2, tspack->rng->floatValue(), &dg); // TODO - REFACT - add passed value from sample
		ray->o = dg.p;
		ray->d = UniformSampleSphere(u3, u4); //Jeanphi - FIXME this is wrong as it doesn't take the portal normal into account
		if (Dot(ray->d, dg.nn) < 0.) ray->d *= -1;

		*pdf = PortalShapes[shapeidx]->Pdf(ray->o) * INV_TWOPI / nrPortalShapes;
	}

	return Le(tspack, RayDifferential(ray->o, -ray->d));
}
bool SkyLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const
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
		*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns,
			ARENA_ALLOC(tspack->arena, SkyBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns)));
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
			Normal (0, 0, 0), 0, 0, NULL);
		*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns,
			ARENA_ALLOC(tspack->arena, SkyPortalBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns), ps, PortalShapes, shapeIndex, u3));
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
	*Le = SWCSpectrum(skyScale);
	return true;
}
bool SkyLight::Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility, SWCSpectrum *Le) const
{
	Vector wi;
	u_int shapeIndex = 0;
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	if(!havePortalShape) {
		// Sample cosine-weighted direction on unit sphere
		float x, y, z;
		ConcentricSampleDisk(u1, u2, &x, &y);
		z = sqrtf(max(0.f, 1.f - x*x - y*y));
		if (u3 < .5f)
			z = -z;
		wi = Vector(x, y, z);
		// Compute _pdf_ for cosine-weighted infinite light direction
		*pdfDirect = fabsf(wi.z) * INV_TWOPI;
		// Transform direction to world space
		Vector v1, v2;
		CoordinateSystem(Normalize(Vector(n)), &v1, &v2);
		wi = Vector(v1 * wi.x + v2 * wi.y + Vector(n) * wi.z);
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
			*pdfDirect = PortalShapes[shapeIndex]->Pdf(p, ps) *
				DistanceSquared(p, ps) / AbsDot(wi, dg.nn);
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
		Normal(0, 0, 0), 0, 0, NULL);
	if (!havePortalShape) {
		*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns,
			ARENA_ALLOC(tspack->arena, SkyBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns)));
		*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	} else {
		*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns,
			ARENA_ALLOC(tspack->arena, SkyPortalBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns), ps, PortalShapes, shapeIndex, u3));
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
			if (i == shapeIndex)
				continue;
			Intersection isect;
			RayDifferential ray(p, wi);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect) &&
				Dot(wi, isect.dg.nn) < 0.f)
				*pdfDirect += PortalShapes[i]->Pdf(p,
					isect.dg.p) * DistanceSquared(p,
					isect.dg.p) / AbsDot(wi, isect.dg.nn);
		}
		*pdf *= INV_TWOPI / nrPortalShapes;
		*pdfDirect /= nrPortalShapes;
	}
	*pdfDirect *= AbsDot(wi, ns) / (distance * distance);
	visibility->SetSegment(p, ps, tspack->time);
	*Le = SWCSpectrum(skyScale);
	return true;
}

Light* SkyLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet, const TextureParams &tp) {
	float scale = paramSet.FindOneFloat("gain", 1.f);				// gain (aka scale) factor to apply to sun/skylight (0.005)
	int nSamples = paramSet.FindOneInt("nsamples", 1);
	Vector sundir = paramSet.FindOneVector("sundir", Vector(0,0,1));	// direction vector of the sun
	Normalize(sundir);
	float turb = paramSet.FindOneFloat("turbidity", 2.0f);			// [in] turb  Turbidity (1.0,30+) 2-6 are most useful for clear days.
	float aconst = paramSet.FindOneFloat("aconst", 1.0f);				// Perez function multiplicative constants
	float bconst = paramSet.FindOneFloat("bconst", 1.0f);
	float cconst = paramSet.FindOneFloat("cconst", 1.0f);
	float dconst = paramSet.FindOneFloat("dconst", 1.0f);
	float econst = paramSet.FindOneFloat("econst", 1.0f);

	SkyLight *l = new SkyLight(light2world, scale, nSamples, sundir, turb, aconst, bconst, cconst, dconst, econst);
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
void SkyLight::GetSkySpectralRadiance(const TsPack *tspack, const float theta, const float phi, SWCSpectrum * const dst_spect) const
{
	// add bottom half of hemisphere with horizon colour
	const float theta_fin = min(theta,(M_PI * 0.5f) - 0.001f);
	const float gamma = RiAngleBetween(theta,phi,thetaS,phiS);

	// Compute xyY values
	const float x = zenith_x * PerezBase(perez_x, theta_fin, gamma);
	const float y = zenith_y * PerezBase(perez_y, theta_fin, gamma);
	const float Y = zenith_Y * PerezBase(perez_Y, theta_fin, gamma);

	ChromaticityToSpectrum(tspack, x, y, dst_spect);
	*dst_spect *= Y;
}

// note - lyc - removed redundant computations and optimised
void SkyLight::ChromaticityToSpectrum(const TsPack *tspack, const float x, const float y, SWCSpectrum * const dst_spect) const
{
	const float den = 1.0f / (0.0241f + 0.2562f * x - 0.7341f * y);
	const float M1 = (-1.3515f - 1.7703f * x + 5.9114f * y) * den;
	const float M2 = (0.03f - 31.4424f * x + 30.0717f * y) * den;

	for (unsigned int j = 0; j < WAVELENGTH_SAMPLES; ++j) {
		const float w = tspack->swl->w[j];
		dst_spect->c[j] = S0.sample(w) + M1 * S1.sample(w) +
			M2 * S2.sample(w);
	}
	*dst_spect /= S0Y + M1 * S1Y + M2 * S2Y;
}

static DynamicLoader::RegisterLight<SkyLight> r("sky");
