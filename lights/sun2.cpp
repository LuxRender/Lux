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

// point.cpp*
#include "sun2.h"

// Derived from PointLight Method Definitions
NSunLight::NSunLight (const Transform &light2world, const Spectrum &intensity, Vector sundir, float turb) : Light (light2world) {
  lightPos = LightToWorld (Point (0.0, 0.0, 0.0));
  sunDir = sundir;
  turbidity = turb;

  float sunTheta = SphericalTheta(sundir);
  printf("sunTheta: %f\n", sunTheta);

  if (sunDir.z > 0.0)
    Intensity = computeAttenuatedSunlight(sunTheta, turbidity);
  else
    printf("Sun dir below horizon! No sunlight..\n");

  printf("sunspectralrad luminance: %f\n", Intensity.y() );

  float xyz[3];

  Intensity.XYZ(xyz);

  float sum = xyz[0] + xyz[1] + xyz[2];

  printf("sun xyY:: %f %f %f\n", xyz[0] / sum, xyz[1] / sum, xyz[1] );

}

Spectrum NSunLight::Power(const Scene *) const {
  return Intensity * 4.0f * M_PI;
}

bool NSunLight::IsDeltaLight() const {
  return true;
}

Spectrum NSunLight::Sample_L (const Point &p, Vector *wi, VisibilityTester *visibility) const {
  *wi = sunDir;
  visibility->SetRay(p, *wi);

  return Intensity;
}

Spectrum NSunLight::Sample_L(const Point &p, float u1, float u2, Vector *wi, float *pdf, VisibilityTester *visibility) const {
  *pdf = 1.0f;

  return Sample_L (p, wi, visibility);
}

float NSunLight::Pdf (const Point &, const Vector &) const {
  return 0.0f;
}

Spectrum NSunLight::Sample_L (const Scene *scene, float u1, float u2, float u3, float u4, Ray *ray, float *pdf) const {
  ray->o = lightPos;
  ray->d = UniformSampleSphere (u1, u2);
  *pdf = UniformSpherePdf ();

  return Intensity;
}

Light* NSunLight::CreateLight (const Transform &light2world, const ParamSet &paramSet) {
  Spectrum I = paramSet.FindOneSpectrum ("L", Spectrum (1.0));

  // Get turbidity - (1.0,30+) 2-6 are most useful for clear days.
  float turb = paramSet.FindOneFloat("turbidity", 2.0f);

  // Get direction vector of the sun
  Vector sundir = paramSet.FindOneVector("sundir", Vector (0.0f, 0.0f, -1.0f));
  Normalize (sundir);

  Transform l2w = Translate (SUN_DIST * sundir) * light2world;

  return new NSunLight(l2w, I, sundir, turb);
}

Spectrum NSunLight::computeAttenuatedSunlight(float theta, float turbidity) {
  IrregularSpectrum k_oCurve(k_oAmplitudes, k_oWavelengths, 65);
  IrregularSpectrum k_gCurve(k_gAmplitudes, k_gWavelengths, 4);
  IrregularSpectrum k_waCurve(k_waAmplitudes, k_waWavelengths, 13);
  RegularSpectrum   solCurve(solAmplitudes, 380, 780, 401);

  // (800 - 350) / 5  + 1
  float  data[91];

  float beta = 0.04608365822050f * turbidity - 0.04586025928522f;
  float tauR, tauA, tauO, tauG, tauWA;

  // Relative Optical Mass
  float m = 1.0f / (cos(theta) + 0.000940f * pow(1.6386f - theta, -1.253f));

  // Relative Optical Mass (this is equivalent)
  //float m = 1.0/(cos(theta) + 0.15*pow(93.885-theta/M_PI*180.0,-1.253));

  int i = 0;
  float lambda;

  // Approximately 0.285 degrees
  float sunSolidAngle = 6.7443e-05;

  for (lambda = 350; i < 91; lambda+=5) {
    // Rayleigh Scattering
    // Results agree with the graph (pg 115, MI)
    tauR = exp( -m * 0.008735f * pow(lambda/1000.0f, -4.08f));

    // Aerosol (water + dust) attenuation
    // beta - amount of aerosols present
    // alpha - ratio of small to large particle sizes. (0:4,usually 1.3)
    // Results agree with the graph (pg 121, MI)
    const float alpha = 1.3f;

    // lambda should be in um
    tauA = exp(-m * beta * pow(lambda/1000.0f, -alpha));

    // Attenuation due to ozone absorption
    // lOzone - amount of ozone in cm(NTP)
    // Results agree with the graph (pg 128, MI) 
    const float lOzone = 0.35f;

    tauO = exp(-m * k_oCurve.sample(lambda) * lOzone);

    // Attenuation due to mixed gases absorption
    // Results agree with the graph (pg 131, MI)

    tauG = exp(-1.41f * k_gCurve.sample(lambda) * m / pow(1.0f + 118.93 * k_gCurve.sample(lambda) * m, 0.45f));

    // Attenuation due to water vapor absorbtion  
    // w - precipitable water vapor in centimeters (standard = 2)
    // Results agree with the graph (pg 132, MI)

    const float w = 2.0f;
    tauWA = exp(-0.2385f * k_waCurve.sample(lambda) * w * m / pow(1.0f + 20.07f * k_waCurve.sample(lambda) * w * m, 0.45f));

    data[i++] = 1000 * (1.0f / sunSolidAngle) * solCurve.sample(lambda) * tauR * tauA * tauO * tauG * tauWA;
  }

  RegularSpectrum oSC(data, 350,800,91);

  // Converts to Spectrum
  return oSC.toSpectrum();
}

