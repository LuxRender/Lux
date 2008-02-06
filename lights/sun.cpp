/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// sun.cpp*
#include "sun.h"

using namespace lux;

// SunLight Method Definitions
SunLight::SunLight(const Transform &light2world,
		const Spectrum &radiance, const Vector &dir, float turb)
	: Light(light2world) {
	lightDir = Normalize(LightToWorld(dir));
	sundir = lightDir;
	turbidity = turb;

	V = 4.0; // Junge's exponent.

	Vector wh = Normalize(sundir);
	phiS = SphericalPhi(wh);
	thetaS = SphericalTheta(wh);

    toSun = Vector(cos(phiS)*sin(thetaS), sin(phiS)*sin(thetaS), cos(thetaS));
	float sunSolidAngle =  0.25*M_PI*1.39*1.39/(150*150);  // = 6.7443e-05

    IrregularSpectrum k_oCurve(sun_k_oAmplitudes, sun_k_oWavelengths, 65);
    IrregularSpectrum k_gCurve(sun_k_gAmplitudes, sun_k_gWavelengths, 4);
    IrregularSpectrum k_waCurve(sun_k_waAmplitudes, sun_k_waWavelengths, 13);

    RegularSpectrum   solCurve(sun_sun_irradiance, 380, 770, 79);  // every 5 nm

    float beta = 0.04608365822050 * turbidity - 0.04586025928522;
    float tauR, tauA, tauO, tauG, tauWA;

    float m = 1.0/(cos(thetaS) + 0.000940 * pow(1.6386 - thetaS,-1.253));  // Relative Optical Mass

    int i;
    float lambda;
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
		tauO = exp(-m * k_oCurve.sample(lambda) * lOzone);
				// Attenuation due to mixed gases absorption  
		tauG = exp(-1.41 * k_gCurve.sample(lambda) * m / pow(1 + 118.93 * k_gCurve.sample(lambda) * m, 0.45));
				// Attenuation due to water vapor absorbtion  
				// w - precipitable water vapor in centimeters (standard = 2) 
		const float w = 2.0;
		tauWA = exp(-0.2385 * k_waCurve.sample(lambda) * w * m /
				pow(1 + 20.07 * k_waCurve.sample(lambda) * w * m, 0.45));

		Ldata[i] = solCurve.sample(lambda) * tauR * tauA * tauO * tauG * tauWA;
    }

    L = new RegularSpectrum(Ldata, 350,800,91);
}

SWCSpectrum SunLight::Sample_L(const Point &p,
		Vector *wi, VisibilityTester *visibility) const {
	*wi = lightDir;
	visibility->SetRay(p, *wi);
	return L;
}
SWCSpectrum SunLight::Sample_L(const Point &p, float u1, float u2,
		Vector *wi, float *pdf, VisibilityTester *visibility) const {
	*pdf = 1.f;
	return Sample_L(p, wi, visibility);
}
float SunLight::Pdf(const Point &, const Vector &) const {
	return 0.;
}
SWCSpectrum SunLight::Sample_L(const Scene *scene,											// TODO - radiance - add portal implementation?
		float u1, float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
	// Choose point on disk oriented toward infinite light direction
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter,
	                                   &worldRadius);
	Vector v1, v2;
	CoordinateSystem(lightDir, &v1, &v2);
	float d1, d2;
	ConcentricSampleDisk(u1, u2, &d1, &d2);
	Point Pdisk =
		worldCenter + worldRadius * (d1 * v1 + d2 * v2);
	// Set ray origin and direction for infinite light ray
	ray->o = Pdisk + worldRadius * lightDir;
	ray->d = -lightDir;
	*pdf = 1.f / (M_PI * worldRadius * worldRadius);
	return L;
}
Light* SunLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
	Spectrum L = paramSet.FindOneSpectrum("L", Spectrum(1.000));
	int nSamples = paramSet.FindOneInt("nsamples", 1);					// unused for SunLight - MUST stay for compatiblity with SkyLight params.
	Vector sundir = paramSet.FindOneVector("sundir", Vector(0,0,-1));	// direction vector of the sun
	float turb = paramSet.FindOneFloat("turbidity", 2.0f);				// [in] turb  Turbidity (1.0,30+) 2-6 are most useful for clear days.
	return new SunLight(light2world, L, sundir, turb);
}

