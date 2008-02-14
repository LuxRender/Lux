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


// -------------------------------------------------------------------------------------------------------------

/* All data lifted from MI */
/* Units are either [] or cm^-1. refer when in doubt MI */

// k_o Spectrum table from pg 127, MI.
static float sun_k_oWavelengths[64] = {
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

static float sun_k_oAmplitudes[65] = {
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
static float sun_k_gWavelengths[4] = {
  759,
  760,
  770,
  771
};

static float sun_k_gAmplitudes[4] = {
  0,
  3.0,
  0.210,
  0
};

// k_wa Spectrum table from pg 130, MI.
static float sun_k_waWavelengths[13] = {
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

static float sun_k_waAmplitudes[13] = {
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
static float sun_solAmplitudes[38] = {
    165.5, 162.3, 211.2, 258.8, 258.2,
    242.3, 267.6, 296.6, 305.4, 300.6,
    306.6, 288.3, 287.1, 278.2, 271.0,
    272.3, 263.6, 255.0, 250.6, 253.1,
    253.5, 251.3, 246.3, 241.7, 236.8,
    232.1, 228.2, 223.4, 219.7, 215.3,
    211.0, 207.3, 202.4, 198.7, 194.3,
    190.7, 186.3, 182.6
};

// The suns irradiance as sampled outside the atmosphere from 380 - 770 nm at 5nm intervals
static float sun_sun_irradiance[79] = {
    1.1200E09,
    1.0980E09,
    1.0980E09,
    1.1890E09,
    1.4290E09,
    1.6440E09,
    1.7510E09,
    1.7740E09,
    1.7470E09,
    1.6930E09,
    1.6390E09,
    1.6630E09,
    1.8100E09,
    1.9220E09,
    2.0060E09,
    2.0570E09,
    2.0660E09,
    2.0480E09,
    2.0330E09,
    2.0440E09,
    2.0740E09,
    1.9760E09,
    1.9500E09,
    1.9600E09,
    1.9420E09,
    1.9200E09,
    1.8820E09,
    1.8330E09,
    1.8330E09,
    1.8520E09,
    1.8420E09,
    1.8180E09,
    1.7830E09,
    1.7540E09,
    1.7250E09,
    1.7200E09,
    1.6950E09,
    1.7050E09,
    1.7120E09,
    1.7190E09,
    1.7150E09,
    1.7120E09,
    1.7000E09,
    1.6820E09,
    1.6660E09,
    1.6470E09,
    1.6350E09,
    1.6150E09,
    1.6020E09,
    1.5900E09,
    1.5700E09,
    1.5550E09,
    1.5440E09,
    1.5270E09,
    1.5110E09,
    1.4985E09,
    1.4860E09,
    1.4710E09,
    1.4560E09,
    1.4415E09,
    1.4270E09,
    1.4145E09,
    1.4020E09,
    1.4855E09,
    1.3690E09,
    1.3565E09,
    1.3440E09,
    1.3290E09,
    1.3140E09,
    1.3020E09,
    1.2900E09,
    1.2750E09,
    1.2600E09,
    1.2475E09,
    1.2350E09,
    1.2230E09,
    1.2110E09,
    1.1980E09,
    1.1850E09
};

// SunLight Method Definitions
SunLight::SunLight(const Transform &light2world,
		const Spectrum &radiance, const Vector &dir, float turb , float relSize, int ns)
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

	IrregularSpectrum k_oCurve(sun_k_oWavelengths,sun_k_oAmplitudes,  65);
	IrregularSpectrum k_gCurve(sun_k_gWavelengths, sun_k_gAmplitudes, 4);
	IrregularSpectrum k_waCurve(sun_k_waWavelengths,sun_k_waAmplitudes,  13);

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

		// NOTE - Ratow - Transform unit to W*m^-2*nm^-1*sr-1
		const float unitConv = 1./(solidAngle*1000000000.);
		Ldata[i] = solCurve.sample(lambda) * tauR * tauA * tauO * tauG * tauWA * unitConv * 100000;
	}
	L = new RegularSpectrum(Ldata, 350,800,91);
}
SWCSpectrum SunLight::Le(const RayDifferential &r) const {
	Vector w = r.d;
	if(cosThetaMax < 1.0f && Dot(w,sundir) > cosThetaMax)
		return L;
	else
		return SWCSpectrum(0.);
}

SWCSpectrum SunLight::Sample_L(const Point &p, float u1, float u2,
		Vector *wi, float *pdf, VisibilityTester *visibility) const {
	if(cosThetaMax == 1) {
		*pdf = 1.f;
		return Sample_L(p, wi, visibility);
	} else {
		*wi = UniformSampleCone(u1, u2, cosThetaMax, x, y, sundir);
		*pdf = UniformConePdf(cosThetaMax);
		visibility->SetRay(p, *wi);
		return L;
	}
}
SWCSpectrum SunLight::Sample_L(const Point &p,
		Vector *wi, VisibilityTester *visibility) const {
	if(cosThetaMax == 1) {
		*wi = sundir;
		visibility->SetRay(p, *wi);
		return L;
	} else {
		float pdf;
		SWCSpectrum Le = Sample_L(p, lux::random::floatValue(), lux::random::floatValue(),
			wi, &pdf, visibility);
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
	return L;
}
Light* SunLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
	//NOTE - Ratow - Added relsize param and reactivated nsamples
	Spectrum L = paramSet.FindOneSpectrum("L", Spectrum(1.000));
	int nSamples = paramSet.FindOneInt("nsamples", 1);
	Vector sundir = paramSet.FindOneVector("sundir", Vector(0,0,-1));	// direction vector of the sun
	float turb = paramSet.FindOneFloat("turbidity", 2.0f);				// [in] turb  Turbidity (1.0,30+) 2-6 are most useful for clear days.
	float relSize = paramSet.FindOneFloat("relsize", 1.0f);				// relative size to the sun. Set to 0 for old behavior.
	return new SunLight(light2world, L, sundir, turb, relSize, nSamples);
}
