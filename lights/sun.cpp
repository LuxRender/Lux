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

// sun.cpp*
#include "sun.h"
#include "memory.h"
#include "spd.h"
#include "regular.h"
#include "irregular.h"
#include "mc.h"
#include "paramset.h"
#include "reflection/bxdf.h"
#include "dynload.h"

#include "data/sun_spect.h"

using namespace lux;

class  SunBSDF : public BSDF  {
public:
	// SunBSDF Public Methods
	SunBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior,
		float sin2Max) :
		BSDF(dgs, ngeom, exterior, interior), sin2ThetaMax(sin2Max) { }
	virtual inline u_int NumComponents() const { return 1; }
	virtual inline u_int NumComponents(BxDFType flags) const {
		return (flags & (BSDF_REFLECTION | BSDF_GLOSSY)) ==
			(BSDF_REFLECTION | BSDF_GLOSSY) ? 1U : 0U;
	}
	virtual bool Sample_f(const TsPack *tspack, const Vector &woW, Vector *wiW,
		float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
		BxDFType flags = BSDF_ALL, BxDFType *sampledType = NULL,
		float *pdfBack = NULL, bool reverse = false) const {
		if (reverse || NumComponents(flags) == 0)
			return false;
		const float sin2Theta = Lerp(u1, 0.f, sin2ThetaMax);
		const float sinTheta = sqrtf(sin2Theta);
		const float cosTheta = sqrtf(1.f - sin2Theta);
		const float phi = 2.f * M_PI * u2;
		*wiW = cosf(phi) * sinTheta * sn + sinf(phi) * sinTheta * tn +
			cosTheta * Vector(nn);
		// Approximate pdf value accounting for the fact that theta max
		// is very small
		*pdf = INV_PI / sin2ThetaMax;
		if (sampledType)
			*sampledType = BxDFType(BSDF_REFLECTION | BSDF_GLOSSY);
		if (pdfBack)
			*pdfBack = 0.f;
		*f_ = SWCSpectrum(*pdf);
		return true;
	}
	virtual float Pdf(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1) {
			const Vector wi(WorldToLocal(wiW));
			if (wi.z > 0.f && (wi.x * wi.x + wi.y * wi.y) <= sin2ThetaMax)
				return INV_PI / sin2ThetaMax;
		}
		return 0.f;
	}
	virtual SWCSpectrum f(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1) {
			const Vector wi(WorldToLocal(wiW));
			if (wi.z > 0.f && (wi.x * wi.x + wi.y * wi.y) <= sin2ThetaMax)
				return SWCSpectrum(INV_PI / sin2ThetaMax);
		}
		return SWCSpectrum(0.f);
	}
	virtual SWCSpectrum rho(const TsPack *tspack,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }
	virtual SWCSpectrum rho(const TsPack *tspack, const Vector &woW,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }

protected:
	// SunBSDF Private Methods
	virtual ~SunBSDF() { }
	const float sin2ThetaMax;
};

// SunLight Method Definitions
SunLight::SunLight(const Transform &light2world, const float sunscale,
	const Vector &dir, float turb , float relSize, u_int ns)
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
		ss << "Reducing relative sun size to " <<
			sunMeanDistance / sunRadius;
		luxError(LUX_LIMIT, LUX_WARNING, ss.str().c_str());
		cosThetaMax = 0.f;
		sin2ThetaMax = 1.f;
	}

	Vector wh = Normalize(sundir);
	phiS = SphericalPhi(wh);
	thetaS = SphericalTheta(wh);

	// NOTE - lordcrc - sun_k_oWavelengths contains 64 elements, while sun_k_oAmplitudes contains 65?!?
	IrregularSPD k_oCurve(sun_k_oWavelengths, sun_k_oAmplitudes, 64);
	IrregularSPD k_gCurve(sun_k_gWavelengths, sun_k_gAmplitudes, 4);
	IrregularSPD k_waCurve(sun_k_waWavelengths, sun_k_waAmplitudes, 13);

	RegularSPD solCurve(sun_solAmplitudes, 380, 750, 38);  // every 5 nm

	float beta = 0.04608365822050f * turbidity - 0.04586025928522f;
	float tauR, tauA, tauO, tauG, tauWA;

	float m = 1.f / (cosf(thetaS) + 0.00094f * powf(1.6386f - thetaS,
		-1.253f));  // Relative Optical Mass

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
		tauA = expf(-m * beta * powf(lambda / 1000.f, -alpha));  // lambda should be in um
			// Attenuation due to ozone absorption
			// lOzone - amount of ozone in cm(NTP)
		const float lOzone = .35f;
		tauO = expf(-m * k_oCurve.sample(lambda) * lOzone);
			// Attenuation due to mixed gases absorption
		tauG = expf(-1.41f * k_gCurve.sample(lambda) * m / powf(1.f +
			118.93f * k_gCurve.sample(lambda) * m, 0.45f));
			// Attenuation due to water vapor absorbtion
			// w - precipitable water vapor in centimeters (standard = 2)
		const float w = 2.0f;
		tauWA = expf(-0.2385f * k_waCurve.sample(lambda) * w * m /
		powf(1.f + 20.07f * k_waCurve.sample(lambda) * w * m, 0.45f));

		Ldata[i] = (solCurve.sample(lambda) *
			tauR * tauA * tauO * tauG * tauWA);
	}
	LSPD = new RegularSPD(Ldata, 350,800,91);
	LSPD->Scale(sunscale);
}

bool SunLight::Le(const TsPack *tspack, const Scene *scene, const Ray &r,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *L) const
{
	const float xD = Dot(r.d, x);
	const float yD = Dot(r.d, y);
	const float zD = Dot(r.d, sundir);
	if (cosThetaMax == 1.f || zD < 0.f ||
		(xD * xD + yD * yD) > sin2ThetaMax)
		return false;
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	Vector toCenter(worldCenter - r.o);
	float approach = Dot(toCenter, sundir);
	float distance = (approach + worldRadius) / zD;
	Point ps(r.o + distance * r.d);
	Normal ns(-sundir);
	DifferentialGeometry dg(ps, ns, -x, y, Normal(0, 0, 0), Normal (0, 0, 0), 0, 0, NULL);
	*bsdf = ARENA_ALLOC(tspack->arena, SunBSDF)(dg, ns, NULL, NULL,
		sin2ThetaMax);
	if (pdf) {
		if (!havePortalShape)
			*pdf = 1.f / (M_PI * worldRadius * worldRadius);
		else {
			*pdf = 0.f;
			for (u_int i = 0; i < nrPortalShapes; ++i) {
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
	}
	if (pdfDirect)
		*pdfDirect = INV_PI * zD / (sin2ThetaMax * DistanceSquared(r.o, ps));
	*L = SWCSpectrum(tspack, *LSPD);
	return true;
}

float SunLight::Pdf(const TsPack *tspack, const Point &p,
	const Point &po, const Normal &ns) const
{
	const float cosTheta = AbsDot(Normalize(p - po), ns);
	if(cosTheta < cosThetaMax)
		return 0.f;
	else
		return INV_PI * cosTheta / (sin2ThetaMax * DistanceSquared(p, po));
}

bool SunLight::Sample_L(const TsPack *tspack, const Scene *scene,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	SWCSpectrum *Le) const
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
		u_int shapeIndex = 0;
		if (nrPortalShapes > 1) {
			u3 *= nrPortalShapes;
			shapeIndex = min(nrPortalShapes - 1, Floor2UInt(u3));
			u3 -= shapeIndex;
		}

		DifferentialGeometry dg;
		dg.time = tspack->time;
		PortalShapes[shapeIndex]->Sample(u1, u2, u3, &dg);
		ps = dg.p;
		const float cosPortal = Dot(ns, dg.nn);
		if (cosPortal <= 0.f)
			return false;

		*pdf = PortalShapes[shapeIndex]->Pdf(ps) / cosPortal;
		for (u_int i = 0; i < nrPortalShapes; ++i) {
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
		if (!(*pdf > 0.f))
			return false;

		ps += (worldRadius + Dot(worldCenter - ps, sundir)) * sundir;
	}

	DifferentialGeometry dg(ps, ns, -x, y, Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	*bsdf = ARENA_ALLOC(tspack->arena, SunBSDF)(dg, ns, NULL, NULL,
		sin2ThetaMax);

	*Le = SWCSpectrum(tspack, *LSPD) * (M_PI * sin2ThetaMax);
	return true;
}

bool SunLight::Sample_L(const TsPack *tspack, const Scene *scene,
	const Point &p, float u1, float u2, float u3,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *Le) const
{
	Vector wi;
	if(cosThetaMax == 1.f) {
		wi = sundir;
		*pdfDirect = 1.f;
	} else {
		const float sin2Theta = Lerp(u1, 0.f, sin2ThetaMax);
		const float sinTheta = sqrtf(sin2Theta);
		const float cosTheta = sqrtf(1.f - sin2Theta);
		const float phi = 2.f * M_PI * u2;
		wi = cosf(phi) * sinTheta * x + sinf(phi) * sinTheta * y +
			cosTheta * sundir;
		*pdfDirect = INV_PI / sin2ThetaMax;
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
	*bsdf = ARENA_ALLOC(tspack->arena, SunBSDF)(dg, ns, NULL, NULL,
		sin2ThetaMax);
	if (pdf) {
		if (!havePortalShape)
			*pdf = 1.f / (M_PI * worldRadius * worldRadius);
		else {
			*pdf = 0.f;
			for (u_int i = 0; i < nrPortalShapes; ++i) {
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
	}
	if (pdfDirect && cosThetaMax < 1.f)
		*pdfDirect *= AbsDot(wi, ns) / DistanceSquared(p, ps);

	*Le = SWCSpectrum(tspack, *LSPD) * (M_PI * sin2ThetaMax);
	return true;
}

Light* SunLight::CreateLight(const Transform &light2world,
	const ParamSet &paramSet)
{
	//NOTE - Ratow - Added relsize param and reactivated nsamples
	float scale = paramSet.FindOneFloat("gain", 1.f);				// gain (aka scale) factor to apply to sun/skylight (0.005)
	int nSamples = paramSet.FindOneInt("nsamples", 1);
	Vector sundir = paramSet.FindOneVector("sundir", Vector(0,0,-1));	// direction vector of the sun
	float turb = paramSet.FindOneFloat("turbidity", 2.0f);				// [in] turb  Turbidity (1.0,30+) 2-6 are most useful for clear days.
	float relSize = paramSet.FindOneFloat("relsize", 1.0f);				// relative size to the sun. Set to 0 for old behavior.

	SunLight *l = new SunLight(light2world, scale, sundir, turb, relSize, nSamples);
	l->hints.InitParam(paramSet);
	return l;
}

static DynamicLoader::RegisterLight<SunLight> r("sun");
