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

#include "data/sun_spect.h"

using namespace lux;

class SunBxDF : public BxDF
{
public:
	SunBxDF(float cosMax, float radius) : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), cosThetaMax(cosMax), worldRadius(radius) {}
	SWCSpectrum f(const Vector &wo, const Vector &wi) const {return wi.z < cosThetaMax ? 0.f : 1.f;}
	SWCSpectrum Sample_f(const Vector &wo, Vector *wi, float u1, float u2, float *pdf) const
	{
		*wi = UniformSampleCone(u1, u2, cosThetaMax, Vector(1, 0, 0), Vector(0, 1, 0), Vector(0, 0, 1));
		*pdf = UniformConePdf(cosThetaMax);
		return 1.f;
	}
	float Pdf(const Vector &wi, const Vector &wo) const
	{
		if (wo.z < cosThetaMax)
			return 0.;
		else
			return UniformConePdf(cosThetaMax);//1.f / (M_PI * worldRadius * worldRadius * (1.f - cosThetaMax * cosThetaMax));
	}
private:
	float cosThetaMax, worldRadius;
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
	const float sunRadius = 695500;
	const float sunMeanDistance = 149600000;
	if(relSize*sunRadius <= sunMeanDistance) {
		cosThetaMax = sqrt(1.0f - pow(relSize*sunRadius/sunMeanDistance, 2));
	} else {
		std::stringstream ss;
		ss <<"Reducing relative sun size to "<< sunMeanDistance/sunRadius;
		luxError(LUX_LIMIT, LUX_WARNING, ss.str().c_str());
		cosThetaMax = 0.0f;
	}

	float solidAngle = 2*M_PI*(1-cosThetaMax);

	Vector wh = Normalize(sundir);
	phiS = SphericalPhi(wh);
	thetaS = SphericalTheta(wh);

    // NOTE - lordcrc - sun_k_oWavelengths contains 64 elements, while sun_k_oAmplitudes contains 65?!?
	SPD *k_oCurve  = new IrregularSPD(sun_k_oWavelengths,sun_k_oAmplitudes,  64);
	SPD *k_gCurve  = new IrregularSPD(sun_k_gWavelengths, sun_k_gAmplitudes, 2);
	SPD *k_waCurve = new IrregularSPD(sun_k_waWavelengths,sun_k_waAmplitudes,  13);

	SPD *solCurve = new RegularSPD(sun_sun_irradiance, 380, 770, 79);  // every 5 nm

	float beta = 0.04608365822050 * turbidity - 0.04586025928522;
	float tauR, tauA, tauO, tauG, tauWA;

	float m = 1.0/(cos(thetaS) + 0.000940 * pow(1.6386 - thetaS,-1.253));  // Relative Optical Mass

	int i;
	float lambda;
    // NOTE - lordcrc - SPD stores data internally, no need for Ldata to stick around
    float Ldata[91];
	for(i = 0, lambda = 350; i < 91; i++, lambda+=5) {
			// Rayleigh Scattering
		tauR = exp( -m * 0.008735 * pow(lambda/1000, float(-4.08)));
			// Aerosal (water + dust) attenuation
			// beta - amount of aerosols present
			// alpha - ratio of small to large particle sizes. (0:4,usually 1.3)
		const float alpha = 1.3;
		tauA = exp(-m * beta * pow(lambda/1000, -alpha));  // lambda should be in um
			// Attenuation due to ozone absorption
			// lOzone - amount of ozone in cm(NTP)
		const float lOzone = .35;
		tauO = exp(-m * k_oCurve->sample(lambda) * lOzone);
			// Attenuation due to mixed gases absorption
		tauG = exp(-1.41 * k_gCurve->sample(lambda) * m / pow(1 + 118.93 * k_gCurve->sample(lambda) * m, 0.45));
			// Attenuation due to water vapor absorbtion
			// w - precipitable water vapor in centimeters (standard = 2)
		const float w = 2.0;
		tauWA = exp(-0.2385 * k_waCurve->sample(lambda) * w * m /
		pow(1 + 20.07 * k_waCurve->sample(lambda) * w * m, 0.45));

		// NOTE - Ratow - Transform unit to W*m^-2*nm^-1*sr-1
		const float unitConv = 1./(solidAngle*1000000000.);
		Ldata[i] = (solCurve->sample(lambda) * tauR * tauA * tauO * tauG * tauWA * unitConv);
	}
	LSPD = new RegularSPD(Ldata, 350,800,91);
	LSPD->Scale(sunscale);

    delete k_oCurve;
    delete k_gCurve;
    delete k_waCurve;
    delete solCurve;
}
SWCSpectrum SunLight::Le(const RayDifferential &r) const {
	Vector w = r.d;
	if(cosThetaMax < 1.0f && Dot(w,sundir) > cosThetaMax)
		return LSPD;
	else
		return 0.;
}

SWCSpectrum SunLight::Sample_L(const Point &p, float u1, float u2, float u3,
		Vector *wi, float *pdf, VisibilityTester *visibility) const {
	if(cosThetaMax == 1) {
		*pdf = 1.f;
		return Sample_L(p, wi, visibility);
	} else {
		*wi = UniformSampleCone(u1, u2, cosThetaMax, x, y, sundir);
		*pdf = UniformConePdf(cosThetaMax);
		visibility->SetRay(p, *wi);
		return LSPD;
	}
}
SWCSpectrum SunLight::Sample_L(const Point &p,
		Vector *wi, VisibilityTester *visibility) const {
	if(cosThetaMax == 1) {
		*wi = sundir;
		visibility->SetRay(p, *wi);
		return LSPD;
	} else {
		float pdf;
		SWCSpectrum Le = Sample_L(p, lux::random::floatValue(), lux::random::floatValue(),
			lux::random::floatValue(), wi, &pdf, visibility);
		if (pdf == 0.f) return Spectrum(0.f);
		return Le / pdf;
	}
}
float SunLight::Pdf(const Point &, const Vector &) const {
	if(cosThetaMax == 1) {
		return 0.;
	} else {
		return UniformConePdf(cosThetaMax);
	}
}
SWCSpectrum SunLight::Sample_L(const Scene *scene,											// TODO - radiance - add portal implementation?
		float u1, float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
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
	return LSPD;
}
SWCSpectrum SunLight::Sample_L(const Scene *scene, float u1, float u2, BSDF **bsdf, float *pdf) const
{
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	float d1, d2;
	ConcentricSampleDisk(u1, u2, &d1, &d2);
	Point p = worldCenter + worldRadius * (sundir + d1 * x + d2 * y);
	Normal ns(-sundir);
	DifferentialGeometry dg(p, ns, -x, y, Vector(0, 0, 0), Vector(0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(BSDF)(dg, ns);
	(*bsdf)->Add(BSDF_ALLOC(SunBxDF)(cosThetaMax, worldRadius));
	*pdf = 1.f / (M_PI * worldRadius * worldRadius);
	return LSPD;
}
SWCSpectrum SunLight::Sample_L(const Scene *scene, const Point &p, const Normal &n,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	VisibilityTester *visibility) const
{
	Vector wi;
	if(cosThetaMax == 1) {
		wi = sundir;
		*pdf = 1.f;
	} else {
		wi = UniformSampleCone(u1, u2, cosThetaMax, x, y, sundir);
		*pdf = UniformConePdf(cosThetaMax);
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
	DifferentialGeometry dg(p, ns, dpdu, dpdv, Vector(0, 0, 0), Vector (0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(BSDF)(dg, ns);
	(*bsdf)->Add(BSDF_ALLOC(SunBxDF)(cosThetaMax, worldRadius));
	*pdf *= AbsDot(wi, ns) / DistanceSquared(worldCenter, ps);
	visibility->SetSegment(p, ps);
	return LSPD;
}
float SunLight::Pdf(const Scene *scene, const Point &p) const
{
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	return 1.f / (M_PI * worldRadius * worldRadius);
}
Light* SunLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
	//NOTE - Ratow - Added relsize param and reactivated nsamples
	float scale = paramSet.FindOneFloat("gain", 1.f);				// gain (aka scale) factor to apply to sun/skylight (0.005)
	int nSamples = paramSet.FindOneInt("nsamples", 1);
	Vector sundir = paramSet.FindOneVector("sundir", Vector(0,0,-1));	// direction vector of the sun
	float turb = paramSet.FindOneFloat("turbidity", 2.0f);				// [in] turb  Turbidity (1.0,30+) 2-6 are most useful for clear days.
	float relSize = paramSet.FindOneFloat("relsize", 1.0f);				// relative size to the sun. Set to 0 for old behavior.
	return new SunLight(light2world, scale, sundir, turb, relSize, nSamples);
}
