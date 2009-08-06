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

// sun.cpp*
#include "sun.h"
#include "spd.h"
#include "regular.h"
#include "irregular.h"
#include "mc.h"
#include "paramset.h"
#include "reflection/bxdf.h"
#include "dynload.h"

#include "data/sun_spect.h"

using namespace lux;

class SunBxDF : public BxDF
{
public:
	SunBxDF(float sin2Max, float radius) : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), sin2ThetaMax(sin2Max), cosThetaMax(sqrtf(1.f - sin2Max)), worldRadius(radius) {}
	virtual ~SunBxDF() { }
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const {
		if (wo.z < 0.f || wi.z < 0.f || (wo.x * wo.x + wo.y * wo.y) > sin2ThetaMax || (wi.x * wi.x + wi.y * wi.y) > sin2ThetaMax)
			return;
		*f += SWCSpectrum(1.f);
	}
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi, float u1, float u2, SWCSpectrum *const f,float *pdf, float *pdfBack = NULL, bool reverse = false) const
	{
		*wi = UniformSampleCone(u1, u2, cosThetaMax);
		*pdf = UniformConePdf(cosThetaMax);
		if (pdfBack)
			*pdfBack = Pdf(*wi, wo);
		*f = SWCSpectrum(1.f);
		return true;
	}
	virtual float Pdf(const Vector &wi, const Vector &wo) const
	{
		if (wo.z < 0.f || wi.z < 0.f || (wo.x * wo.x + wo.y * wo.y) > sin2ThetaMax || (wi.x * wi.x + wi.y * wi.y) > sin2ThetaMax)
			return 0.f;
		else
			return UniformConePdf(cosThetaMax);
	}
private:
	float sin2ThetaMax, cosThetaMax, worldRadius;
};

// SunLight Method Definitions
SunLight::SunLight(const Transform &light2world,
		const float sunscale, const Vector &dir, float turb , float relSize, int ns)
	: Light(light2world, ns) {
	sundir = Normalize(LightToWorld(dir));
	turbidity = turb;

	CoordinateSystem(sundir, &x, &y);

	// Values from NASA Solar System Exploration page
	// http://solarsystem.nasa.gov/planets/profile.cfm?Object=Sun&Display=Facts&System=Metric
	const float sunRadius = 695500.f;
	const float sunMeanDistance = 149600000.f;
	if (relSize * sunRadius <= sunMeanDistance) {
		sin2ThetaMax = relSize * sunRadius / sunMeanDistance;
		sin2ThetaMax *= sin2ThetaMax;
		cosThetaMax = sqrtf(1.f - sin2ThetaMax);
	} else {
		std::stringstream ss;
		ss << "Reducing relative sun size to " << sunMeanDistance / sunRadius;
		luxError(LUX_LIMIT, LUX_WARNING, ss.str().c_str());
		cosThetaMax = 0.f;
		sin2ThetaMax = 1.f;
	}

	Vector wh = Normalize(sundir);
	phiS = SphericalPhi(wh);
	thetaS = SphericalTheta(wh);

    // NOTE - lordcrc - sun_k_oWavelengths contains 64 elements, while sun_k_oAmplitudes contains 65?!?
	SPD *k_oCurve  = new IrregularSPD(sun_k_oWavelengths,sun_k_oAmplitudes,  64);
	SPD *k_gCurve  = new IrregularSPD(sun_k_gWavelengths, sun_k_gAmplitudes, 4);
	SPD *k_waCurve = new IrregularSPD(sun_k_waWavelengths,sun_k_waAmplitudes,  13);

	SPD *solCurve = new RegularSPD(sun_solAmplitudes, 380, 750, 38);  // every 5 nm

	float beta = 0.04608365822050f * turbidity - 0.04586025928522f;
	float tauR, tauA, tauO, tauG, tauWA;

	float m = 1.f / (cos(thetaS) + 0.00094f * powf(1.6386f - thetaS, -1.253f));  // Relative Optical Mass

	int i;
	float lambda;
    // NOTE - lordcrc - SPD stores data internally, no need for Ldata to stick around
    float Ldata[91];
	for(i = 0, lambda = 350.f; i < 91; ++i, lambda += 5.f) {
			// Rayleigh Scattering
		tauR = expf( -m * 0.008735f * powf(lambda / 1000.f, -4.08f));
			// Aerosol (water + dust) attenuation
			// beta - amount of aerosols present
			// alpha - ratio of small to large particle sizes. (0:4,usually 1.3)
		const float alpha = 1.3f;
		tauA = expf(-m * beta * pow(lambda / 1000.f, -alpha));  // lambda should be in um
			// Attenuation due to ozone absorption
			// lOzone - amount of ozone in cm(NTP)
		const float lOzone = .35f;
		tauO = expf(-m * k_oCurve->sample(lambda) * lOzone);
			// Attenuation due to mixed gases absorption
		tauG = expf(-1.41f * k_gCurve->sample(lambda) * m / powf(1.f + 118.93f * k_gCurve->sample(lambda) * m, 0.45f));
			// Attenuation due to water vapor absorbtion
			// w - precipitable water vapor in centimeters (standard = 2)
		const float w = 2.0f;
		tauWA = expf(-0.2385f * k_waCurve->sample(lambda) * w * m /
		powf(1.f + 20.07f * k_waCurve->sample(lambda) * w * m, 0.45f));

		Ldata[i] = (solCurve->sample(lambda) * tauR * tauA * tauO * tauG * tauWA);
	}
	LSPD = new RegularSPD(Ldata, 350,800,91);
	LSPD->Scale(sunscale);

    delete k_oCurve;
    delete k_gCurve;
    delete k_waCurve;
    delete solCurve;
}

SWCSpectrum SunLight::Le(const TsPack *tspack, const RayDifferential &r) const {
	Vector w = r.d;
	if(cosThetaMax < 1.f && Dot(w,sundir) > cosThetaMax)
		return SWCSpectrum(tspack, LSPD);
	else
		return SWCSpectrum(0.f);
}

SWCSpectrum SunLight::Le(const TsPack *tspack, const Scene *scene, const Ray &r,
	const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const
{
	const float xD = Dot(r.d, x);
	const float yD = Dot(r.d, y);
	if (cosThetaMax == 1.f || Dot(r.d, sundir) < 0.f || (xD * xD + yD * yD) > sin2ThetaMax) {
		*bsdf = NULL;
		*pdf = 0.f;
		*pdfDirect = 0.f;
		return SWCSpectrum(0.f);
	}
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	Vector toCenter(worldCenter - r.o);
	float approach = Dot(toCenter, sundir);
	float distance = (approach + worldRadius) / Dot(r.d, sundir);
	Point ps(r.o + distance * r.d);
	Normal ns(-sundir);
	DifferentialGeometry dg(ps, ns, -x, y, Normal(0, 0, 0), Normal (0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(tspack, SingleBSDF)(dg, ns,
		BSDF_ALLOC(tspack, SunBxDF)(sin2ThetaMax, worldRadius));
	if (!havePortalShape)
		*pdf = 1.f / (M_PI * worldRadius * worldRadius);
	else {
		*pdf = 0.f;
		for (int i = 0; i < nrPortalShapes; ++i) {
			Intersection isect;
			RayDifferential ray(ps, sundir);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect)) {
				float cosPortal = Dot(-sundir, isect.dg.nn);
				if (cosPortal > 0.f)
					*pdf += PortalShapes[i]->Pdf(isect.dg.p) / cosPortal;
			}
		}
		*pdf /= nrPortalShapes;
	}
	*pdfDirect = UniformConePdf(cosThetaMax) * AbsDot(r.d, ns) / DistanceSquared(r.o, ps);
	return SWCSpectrum(tspack, LSPD);
}

bool SunLight::checkPortals(Ray portalRay) const {
	if (!havePortalShape)
		return true;

	Ray isectRay(portalRay);
	Intersection isect;
	bool found = false;
	for (int i = 0; i < nrPortalShapes; ++i) {
		// Dade - I need to use Intersect instead of IntersectP
		// because of the normal
		if (PortalShapes[i]->Intersect(isectRay, &isect)) {
			// Dade - found a valid portal, check the orientation
			if (Dot(portalRay.d, isect.dg.nn) < 0.f) {
				found = true;
				break;
			}
		}
	}

	return found;
}

SWCSpectrum SunLight::Sample_L(const TsPack *tspack, const Point &p, float u1, float u2, float u3,
		Vector *wi, float *pdf, VisibilityTester *visibility) const {
	if(cosThetaMax == 1.f) {
		*wi = sundir;
		*pdf = 1.f;
	} else {
		*wi = UniformSampleCone(u1, u2, cosThetaMax, x, y, sundir);
		*pdf = UniformConePdf(cosThetaMax);
	}
	visibility->SetRay(p, *wi, tspack->time);

	// Dade - check if the portals are excluding this ray
/*	if (!checkPortals(Ray(p, *wi)))
		return SWCSpectrum(0.f);*/

	return SWCSpectrum(tspack, LSPD);
}
float SunLight::Pdf(const Point &, const Vector &) const {
	if(cosThetaMax == 1)
		return 0.f;
	else
		return UniformConePdf(cosThetaMax);
}
float SunLight::Pdf(const Point &p, const Normal &n,
	const Point &po, const Normal &ns) const
{
	const float cosTheta = AbsDot(Normalize(p - po), ns);
	if(cosTheta < cosThetaMax)
		return 0.f;
	else
		return UniformConePdf(cosThetaMax) * cosTheta / DistanceSquared(p, po);
}

SWCSpectrum SunLight::Sample_L(const TsPack *tspack, const Scene *scene,
		float u1, float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
	if (!havePortalShape) {
		// Choose point on disk oriented toward infinite light direction
		Point worldCenter;
		float worldRadius;
		scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
		float d1, d2;
		ConcentricSampleDisk(u1, u2, &d1, &d2);
		Point Pdisk =
			worldCenter + worldRadius * (d1 * x + d2 * y);
		// Set ray origin and direction for infinite light ray
		ray->o = Pdisk + worldRadius * sundir;
		ray->d = -UniformSampleCone(u3, u4, cosThetaMax, x, y, sundir);
		*pdf = UniformConePdf(cosThetaMax) / (M_PI * worldRadius * worldRadius);

		return SWCSpectrum(tspack, LSPD);
	} else {
		// Dade - choose a random portal. This strategy is quite bad if there
		// is more than one portal.
		int shapeidx = 0;
		if(nrPortalShapes > 1)
			shapeidx = min<float>(nrPortalShapes - 1,
					Floor2Int(tspack->rng->floatValue() * nrPortalShapes)); // TODO - REFACT - add passed value from sample

		DifferentialGeometry dg;
		dg.time = tspack->time;
		PortalShapes[shapeidx]->Sample(u1, u2, tspack->rng->floatValue(), &dg); // TODO - REFACT - add passed value from sample
		ray->o = dg.p;
		float pdfPortal = PortalShapes[shapeidx]->Pdf(ray->o) / nrPortalShapes;

		ray->d = -UniformSampleCone(u3, u4, cosThetaMax, x, y, sundir);
		float pdfSun = UniformConePdf(cosThetaMax);

		// Dade - extend the ray origin up to the "sun disk"
		Point worldCenter;
		float worldRadius;
		scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
		Vector centerSunDisk = Vector(worldCenter) + worldRadius;

		// Dade - intersect the ray with the plane of the sun disk
		// t = -(Pn· R0 + D) / (Pn · Rd)
		float PnRd = Dot(sundir, ray->d);
		if (PnRd == 0.f)
			return SWCSpectrum(0.f);

		float distanceSunDisk = centerSunDisk.Length();
		float t = - (Dot(sundir, Vector(ray->o)) + distanceSunDisk) / PnRd;

		// Dade - move the ray origin on the "sun disk"
		ray->o = ray->o - ray->d * t;

		*pdf = pdfSun * pdfPortal;

		if (Dot(ray->d, dg.nn) <= 0.f)
			return SWCSpectrum(0.f);
		else
			return SWCSpectrum(tspack, LSPD);
	}
}

bool SunLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const
{
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);

	Point ps;
	Normal ns(-sundir);
	if (!havePortalShape) {
		float d1, d2;
		ConcentricSampleDisk(u1, u2, &d1, &d2);
		ps = worldCenter + worldRadius * (sundir + d1 * x + d2 * y);
		*pdf = 1.f / (M_PI * worldRadius * worldRadius);
	} else  {
		// Choose a random portal
		int shapeIndex = 0;
		if(nrPortalShapes > 1) {
			shapeIndex = min(nrPortalShapes - 1,
				Floor2Int(u3 * nrPortalShapes));
			u3 *= nrPortalShapes;
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
		for (int i = 0; i < nrPortalShapes; ++i) {
			if (i == shapeIndex)
				continue;
			Intersection isect;
			RayDifferential ray(ps, sundir);
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

		ps += (worldRadius + Dot(worldCenter - ps, sundir)) * sundir;
	}

	DifferentialGeometry dg(ps, ns, -x, y, Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(tspack, SingleBSDF)(dg, ns,
		BSDF_ALLOC(tspack, SunBxDF)(sin2ThetaMax, worldRadius));

	*Le = SWCSpectrum(tspack, LSPD);
	return true;
}

bool SunLight::Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility, SWCSpectrum *Le) const
{
	Vector wi;
	if(cosThetaMax == 1.f) {
		wi = sundir;
		*pdfDirect = 1.f;

		// Dade - check if the portals are excluding this ray
/*		if (!checkPortals(Ray(p, wi))) {
			*bsdf = NULL;
			*pdf = 0.0f;
			return SWCSpectrum(0.0f);
		}*/
	} else {
		wi = UniformSampleCone(u1, u2, cosThetaMax, x, y, sundir);
		*pdfDirect = UniformConePdf(cosThetaMax);

		// Dade - check if the portals are excluding this ray
/*		if (!checkPortals(Ray(p, wi))) {
			*bsdf = NULL;
			*pdf = 0.0f;
			return SWCSpectrum(0.0f);
		}*/
	}

	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	Vector toCenter(worldCenter - p);
	float approach = Dot(toCenter, sundir);
	float distance = (approach + worldRadius) / Dot(wi, sundir);
	Point ps(p + distance * wi);
	Normal ns(-sundir);

	DifferentialGeometry dg(ps, ns, -x, y, Normal(0, 0, 0), Normal (0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(tspack, SingleBSDF)(dg, ns,
		BSDF_ALLOC(tspack, SunBxDF)(sin2ThetaMax, worldRadius));
	if (!havePortalShape)
		*pdf = 1.f / (M_PI * worldRadius * worldRadius);
	else {
		*pdf = 0.f;
		for (int i = 0; i < nrPortalShapes; ++i) {
			Intersection isect;
			RayDifferential ray(ps, sundir);
			ray.mint = -INFINITY;
			if (PortalShapes[i]->Intersect(ray, &isect)) {
				float cosPortal = Dot(ns, isect.dg.nn);
				if (cosPortal > 0.f)
					*pdf += PortalShapes[i]->Pdf(isect.dg.p) / cosPortal;
			}
		}
		*pdf /= nrPortalShapes;
	}
	if (cosThetaMax < 1.f)
		*pdfDirect *= AbsDot(wi, ns) / DistanceSquared(p, ps);
	visibility->SetSegment(p, ps, tspack->time);

	*Le = SWCSpectrum(tspack, LSPD);
	return true;
}

Light* SunLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet, const TextureParams &tp) {
	//NOTE - Ratow - Added relsize param and reactivated nsamples
	float scale = paramSet.FindOneFloat("gain", 1.f);				// gain (aka scale) factor to apply to sun/skylight (0.005)
	int nSamples = paramSet.FindOneInt("nsamples", 1);
	Vector sundir = paramSet.FindOneVector("sundir", Vector(0,0,-1));	// direction vector of the sun
	float turb = paramSet.FindOneFloat("turbidity", 2.0f);				// [in] turb  Turbidity (1.0,30+) 2-6 are most useful for clear days.
	float relSize = paramSet.FindOneFloat("relsize", 1.0f);				// relative size to the sun. Set to 0 for old behavior.
	return new SunLight(light2world, scale, sundir, turb, relSize, nSamples);
}

static DynamicLoader::RegisterLight<SunLight> r("sun");
