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

// SunLight Method Definitions
SunLight::SunLight(const Transform &light2world,
		const Spectrum &radiance, const Vector &dir, float turb)
	: Light(light2world) {
	lightDir = Normalize(LightToWorld(dir));
	sundir = lightDir;
	turbidity = turb;

	V = 4.0; // Junge's exponent.

    InitSunThetaPhi();
    toSun = Vector(cos(phiS)*sin(thetaS), sin(phiS)*sin(thetaS), cos(thetaS));
	float sunSolidAngle =  0.25*M_PI*1.39*1.39/(150*150);  // = 6.7443e-05
    L = radiance * ComputeAttenuatedSunlight(thetaS, turbidity);

	printf("sunsolidangle: %f\n", sunSolidAngle);
	printf("sunspectralrad luminance: %f\n", L.y() );
}
Spectrum SunLight::Sample_L(const Point &p,
		Vector *wi, VisibilityTester *visibility) const {
	*wi = lightDir;
	visibility->SetRay(p, *wi);
	return L;
}
Spectrum SunLight::Sample_L(const Point &p, float u1, float u2,
		Vector *wi, float *pdf, VisibilityTester *visibility) const {
	*pdf = 1.f;
	return Sample_L(p, wi, visibility);
}
float SunLight::Pdf(const Point &, const Vector &) const {
	return 0.;
}
Spectrum SunLight::Sample_L(const Scene *scene,											// TODO - radiance - add portal implementation?
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
	Spectrum L = paramSet.FindOneSpectrum("L", Spectrum(0.005));		// Base color (gain) 0.005 is good
	int nSamples = paramSet.FindOneInt("nsamples", 1);					// unused for SunLight - MUST stay for compatiblity with SkyLight params.
	Vector sundir = paramSet.FindOneVector("sundir", Vector(0,0,-1));	// direction vector of the sun
	Normalize(sundir);
	float turb = paramSet.FindOneFloat("turbidity", 2.0f);				// [in] turb  Turbidity (1.0,30+) 2-6 are most useful for clear days.
	return new SunLight(light2world, L, sundir, turb);
}

/**********************************************************
// South = x,  East = y, up = z
// All times in decimal form (6.25 = 6:15 AM)
// All angles in Radians
// From IES Lighting Handbook pg 361
// ********************************************************/

void SunLight::InitSunThetaPhi()
{
	Vector wh = Normalize(sundir);
	phiS = SphericalPhi(wh);
	thetaS = SphericalTheta(wh);
}

// -------------------------------------------------------------------------------------------------------------

/* All data lifted from MI */
/* Units are either [] or cm^-1. refer when in doubt MI */


// k_o Spectrum table from pg 127, MI.
static float k_oWavelengths[64] = {
300, 305, 310, 315, 320,
325, 330, 335, 340, 345,
350, 355,

445, 450, 455, 460, 465,
470, 475, 480, 485, 490,
495,

500, 505, 510, 515, 520,
525, 530, 535, 540, 545,
550, 555, 560, 565, 570,
575, 580, 585, 590, 595,

600, 605, 610, 620, 630,
640, 650, 660, 670, 680,
690,

700, 710, 720, 730, 740,
750, 760, 770, 780, 790
};

static float k_oAmplitudes[65] = {
  10.0,
  4.8,
  2.7,
  1.35,
  .8,
  .380,
  .160,
  .075,
  .04,
  .019,
  .007,
  .0,
  
  .003,
  .003,
  .004,
  .006,
  .008,
  .009,
  .012,
  .014,
  .017,
  .021,
  .025,

  .03,
  .035,
  .04,
  .045,
  .048,
  .057,
  .063,
  .07,
  .075,
  .08,
  .085,
  .095,
  .103,
  .110,
  .12,
  .122,
  .12,
  .118,
  .115,
  .12,

  .125,
  .130,
  .12,
  .105,
  .09,
  .079,
  .067,
  .057,
  .048,
  .036,
  .028,
  
  .023,
  .018,
  .014,
  .011,
  .010,
  .009,
  .007,
  .004,
  .0,
  .0
};

// k_g Spectrum table from pg 130, MI.
static float k_gWavelengths[4] = {
  759,
  760,
  770,
  771
};

static float k_gAmplitudes[4] = {
  0,
  3.0,
  0.210,
  0
};

// k_wa Spectrum table from pg 130, MI.
static float k_waWavelengths[13] = {
  689,
  690,
  700,
  710,
  720,
  730,
  740,
  750,
  760,
  770,
  780,
  790,
  800
};

static float k_waAmplitudes[13] = {
  0,
  0.160e-1,
  0.240e-1,
  0.125e-1,
  0.100e+1,
  0.870,
  0.610e-1,
  0.100e-2,
  0.100e-4,
  0.100e-4,
  0.600e-3,
  0.175e-1,
  0.360e-1
};

// 380-750 by 10nm
static float solAmplitudes[38] = {
    165.5, 162.3, 211.2, 258.8, 258.2,
    242.3, 267.6, 296.6, 305.4, 300.6,
    306.6, 288.3, 287.1, 278.2, 271.0,
    272.3, 263.6, 255.0, 250.6, 253.1,
    253.5, 251.3, 246.3, 241.7, 236.8,
    232.1, 228.2, 223.4, 219.7, 215.3,
    211.0, 207.3, 202.4, 198.7, 194.3,
    190.7, 186.3, 182.6
};

/**********************************************************
// Sunlight Transmittance Functions
// 
// ********************************************************/

/* Most units not in SI system - For units, refer MI */
Spectrum SunLight::ComputeAttenuatedSunlight(float theta, float turbidity)
{
    IrregularSpectrum k_oCurve(k_oAmplitudes, k_oWavelengths, 65);
    IrregularSpectrum k_gCurve(k_gAmplitudes, k_gWavelengths, 4);
    IrregularSpectrum k_waCurve(k_waAmplitudes, k_waWavelengths, 13);
    RegularSpectrum   solCurve(solAmplitudes, 380, 750, 38);  // every 10 nm  IN WRONG UNITS
				                                     // Need a factor of 100 (done below)
    float  data[91];  // (800 - 350) / 5  + 1

    float beta = 0.04608365822050 * turbidity - 0.04586025928522;
    float tauR, tauA, tauO, tauG, tauWA;

    //float m = 1.0/(cos(theta) + 0.15*pow(93.885-theta/M_PI*180.0,-1.253));  // Relative Optical Mass
				// equivalent  
    float m = 1.0/(cos(theta) + 0.000940 * pow(1.6386 - theta,-1.253));  // Relative Optical Mass

    int i;
    float lambda;
    for(i = 0, lambda = 350; i < 91; i++, lambda+=5) {
				// Rayleigh Scattering
				// Results agree with the graph (pg 115, MI) */
		tauR = exp( -m * 0.008735 * pow(lambda/1000, float(-4.08)));

				// Aerosal (water + dust) attenuation
				// beta - amount of aerosols present 
				// alpha - ratio of small to large particle sizes. (0:4,usually 1.3)
				// Results agree with the graph (pg 121, MI) 
	const float alpha = 1.3;
	tauA = exp(-m * beta * pow(lambda/1000, -alpha));  // lambda should be in um

				// Attenuation due to ozone absorption  
				// lOzone - amount of ozone in cm(NTP) 
				// Results agree with the graph (pg 128, MI) 
	const float lOzone = .35;
	tauO = exp(-m * k_oCurve.sample(lambda) * lOzone);

				// Attenuation due to mixed gases absorption  
				// Results agree with the graph (pg 131, MI)
	tauG = exp(-1.41 * k_gCurve.sample(lambda) * m / pow(1 + 118.93 * k_gCurve.sample(lambda) * m, 0.45));

				// Attenuation due to water vapor absorbtion  
				// w - precipitable water vapor in centimeters (standard = 2) 
				// Results agree with the graph (pg 132, MI)
	const float w = 2.0;
	tauWA = exp(-0.2385 * k_waCurve.sample(lambda) * w * m /
		    pow(1 + 20.07 * k_waCurve.sample(lambda) * w * m, 0.45));

	data[i] = 100 * solCurve.sample(lambda) * tauR * tauA * tauO * tauG * tauWA;  // 100 comes from solCurve being
																				// in wrong units. 
    }
    RegularSpectrum oSC(data, 350,800,91);
    return oSC.toSpectrum();  // Converts to Spectrum
}
