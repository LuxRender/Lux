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

// sun3.cpp*
#include "sun3.h"
#include "sphere.h"
#include "shape.h"

using namespace lux;

#define SUN_DIST 1000.0f
#define SUN_RAD 20.0f

//#define SUN_RAD 6.955E8


// Sun3Light Method Definitions
Sun3Light::Sun3Light(const Transform &light2world, const Spectrum &le, int ns, Vector sundir, float turb) : Light(light2world, ns) {
  Lemit = le;
  turbidity = turb;
  sunDir = sundir;

  Transform l2w = Translate (SUN_DIST * sundir) * light2world;

  shape = boost::shared_ptr<Shape>(new Sphere(l2w, false, SUN_RAD, -SUN_RAD, SUN_RAD, 360.0));

  area = shape->Area();

  printf("Area: %f\n", area);

  float sunTheta = SphericalTheta(sundir);
  printf("sunTheta: %f\n", sunTheta);

  if (sunDir.z > 0.0)
    Lemit = computeAttenuatedSunlight(sunTheta, turbidity);
  else
    printf("Sun dir below horizon! No sunlight..\n");

  printf("sunspectralrad luminance: %f\n", Lemit.y() );

  float xyz[3];

  Lemit.ToXYZ(xyz);

  float sum = xyz[0] + xyz[1] + xyz[2];

  printf("sun xyY:: %f %f %f\n", xyz[0] / sum, xyz[1] / sum, xyz[1] );
}

Spectrum Sun3Light::Sample_L(const Point &p, const Normal &n, float u1, float u2, Vector *wi, float *pdf, VisibilityTester *visibility) const {
  Normal ns;

  Point ps = shape->Sample(p, u1, u2, &ns);

  *wi = Normalize(ps - p);
  *pdf = shape->Pdf(p, *wi);

  visibility->SetRay(p, *wi);

  return L(ps, ns, -*wi);
}

float Sun3Light::Pdf(const Point &p, const Normal &N, const Vector &wi) const {
  return shape->Pdf(p, wi);
}

Spectrum Sun3Light::Sample_L(const Point &P, float u1, float u2, Vector *wo, float *pdf, VisibilityTester *visibility) const {
  Normal Ns;

  Point Ps = shape->Sample(P, u1, u2, &Ns);

  *wo = Normalize(Ps - P);
  *pdf = shape->Pdf(P, *wo);

  visibility->SetRay(P, *wo);

  return L(Ps, Ns, -*wo);
}

Spectrum Sun3Light::Sample_L(const Scene *scene, float u1, float u2, float u3, float u4, Ray *ray, float *pdf) const {
  Normal ns;

  ray->o = shape->Sample(u1, u2, &ns);
  ray->d = UniformSampleSphere(u3, u4);

  if (Dot(ray->d, ns) < 0.)
    ray->d *= -1;

  *pdf = shape->Pdf(ray->o) * INV_TWOPI;

  return L(ray->o, ns, ray->d);
}

float Sun3Light::Pdf(const Point &P, const Vector &w) const {
  return shape->Pdf(P, w);
}

Spectrum Sun3Light::Sample_L(const Point &P, Vector *wo, VisibilityTester *visibility) const {
  Normal Ns;

  Point Ps = shape->Sample(P, lux::random::floatValue(), lux::random::floatValue(), &Ns);

  *wo = Normalize(Ps - P);

  visibility->SetRay(P, *wo);
  float pdf = shape->Pdf(P, *wo);

  if (pdf == 0.f)
    return Spectrum(0.f);

  return L(P, Ns, -*wo) / pdf;
}

Spectrum Sun3Light::L(const Point &p, const Normal &n, const Vector &w) const {
  return Dot(n, w) > 0 ? Lemit : 0.;
}

Spectrum Sun3Light::Power(const Scene *) const {
  return Lemit * area * M_PI;
}

bool Sun3Light::IsDeltaLight() const {
  return false;
}

// Data section
// All data lifted from MI
// Units are either [] or cm^-1. refer when in doubt MI

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
//static float solAmplitudes[38] = {
//    165.5, 162.3, 211.2, 258.8, 258.2,
//    242.3, 267.6, 296.6, 305.4, 300.6,
//    306.6, 288.3, 287.1, 278.2, 271.0,
//    272.3, 263.6, 255.0, 250.6, 253.1,
//    253.5, 251.3, 246.3, 241.7, 236.8,
//    232.1, 228.2, 223.4, 219.7, 215.3,
//    211.0, 207.3, 202.4, 198.7, 194.3,
//    190.7, 186.3, 182.6
//};

// 380-780 by 1nm - contributed by Tinman (thanks!)
static float solAmplitudes[401] = {
  1.0330, 1.6610, 1.2456, 0.8708, 0.7151, 1.3499, 0.8637, 1.2578, 1.0458, 1.0092, 1.4570,
  1.6589, 1.4746, 0.7372, 0.9457, 1.4301, 1.6219, 0.3363, 1.6448, 1.8587, 1.8344, 1.9217,
  1.9802, 2.0327, 2.0911, 1.9701, 1.7134, 2.0167, 1.6921, 2.1496, 1.7309, 1.8885, 1.8452,
  1.9101, 2.1234, 1.9385, 1.9402, 1.9792, 1.8671, 1.8175, 1.6116, 2.0423, 1.9831, 1.9124,
  1.7873, 1.6942, 1.7383, 1.8276, 1.9392, 1.5929, 1.2553, 1.0524, 2.1508, 2.0709, 1.4193,
  2.0752, 1.8733, 1.9818, 2.0194, 1.8237, 2.0300, 1.8720, 2.1528, 2.1750, 2.2371, 2.1642,
  1.8369, 1.9679, 2.2016, 2.0058, 2.1900, 2.2225, 2.1548, 1.8227, 2.2303, 1.9708, 2.2165,
  2.2283, 2.1559, 2.2131, 2.1084, 2.2610, 2.1131, 2.1870, 2.0672, 1.9929, 2.2509, 1.8847,
  2.0252, 2.1231, 2.0122, 1.9674, 2.2129, 1.9849, 2.1787, 2.1760, 2.1769, 2.1352, 2.2382,
  2.0793, 2.0997, 2.0809, 2.2121, 2.0921, 2.0503, 2.0213, 1.7492, 1.9751, 2.0147, 1.9478,
  2.1720, 2.0571, 1.7424, 2.1385, 1.8131, 2.1691, 1.8750, 2.0241, 2.0059, 2.0939, 2.0339,
  2.0637, 1.8259, 1.9749, 1.9349, 1.9154, 2.1179, 1.9831, 1.9189, 2.0465, 1.7811, 1.9441,
  2.1212, 1.9186, 1.8172, 1.9156, 2.0179, 1.4642, 2.0368, 1.8864, 1.9649, 1.7637, 1.9830,
  1.8756, 2.0596, 1.9809, 2.0901, 1.7147, 2.0605, 2.0507, 1.8036, 2.0437, 2.0355, 1.5407,
  1.9792, 1.9048, 2.0757, 1.9042, 2.0022, 1.9879, 1.8828, 1.9183, 1.9949, 1.9151, 2.0344,
  1.9726, 2.0417, 1.9998, 1.9125, 1.8976, 1.9488, 1.9558, 1.9863, 1.7230, 1.9348, 1.9844,
  1.9325, 1.8764, 1.9207, 1.6981, 1.7904, 1.9560, 1.8828, 1.9176, 1.8479, 1.9164, 1.6810,
  1.8701, 1.8995, 1.8025, 1.8684, 1.7268, 1.8777, 1.9168, 1.9090, 1.8438, 1.9051, 1.9148,
  1.8907, 1.8676, 1.8326, 1.8523, 1.8873, 1.8805, 1.8557, 1.8426, 1.7316, 1.8519, 1.8541,
  1.6736, 1.7869, 1.8487, 1.8461, 1.7972, 1.8443, 1.7480, 1.8092, 1.8326, 1.7915, 1.7940,
  1.7582, 1.6858, 1.7784, 1.7843, 1.8082, 1.8021, 1.7952, 1.8013, 1.6993, 1.7612, 1.7667,
  1.7339, 1.7663, 1.7213, 1.6178, 1.7029, 1.7111, 1.6606, 1.7295, 1.7060, 1.7403, 1.7421,
  1.6956, 1.7071, 1.6630, 1.6846, 1.6247, 1.7152, 1.6944, 1.7155, 1.6491, 1.7001, 1.5613,
  1.6791, 1.6213, 1.6732, 1.6236, 1.6756, 1.6579, 1.6799, 1.6436, 1.5647, 1.6338, 1.6603,
  1.5620, 1.6183, 1.6418, 1.6262, 1.6436, 1.6398, 1.5549, 1.6254, 1.5668, 1.5900, 1.6064,
  1.5801, 1.4622, 1.4641, 1.5786, 1.5781, 1.5469, 1.5438, 1.5843, 1.5710, 1.5760, 1.5756,
  1.5777, 1.5559, 1.4739, 1.5691, 1.5418, 1.5533, 1.4751, 1.5447, 1.5240, 1.5239, 1.5346,
  1.4909, 1.5362, 1.5265, 1.5159, 1.5161, 1.5212, 1.4772, 1.4762, 1.4989, 1.4630, 1.4987,
  1.4920, 1.4973, 1.4861, 1.4899, 1.4644, 1.4808, 1.4782, 1.4719, 1.4746, 1.4721, 1.3673,
  1.4488, 1.4495, 1.4332, 1.4038, 1.4350, 1.3909, 1.4486, 1.4343, 1.3841, 1.4354, 1.4150,
  1.4160, 1.4081, 1.4000, 1.3975, 1.4116, 1.3098, 1.3997, 1.4003, 1.3883, 1.3096, 1.3678,
  1.2865, 1.3620, 1.3616, 1.3718, 1.3752, 1.3739, 1.3596, 1.3526, 1.3030, 1.3583, 1.3026,
  1.3466, 1.3312, 1.3487, 1.3414, 1.3255, 1.3309, 1.3330, 1.2372, 1.3245, 1.2901, 1.2907,
  1.3189, 1.3125, 1.2796, 1.3092, 1.3071, 1.2925, 1.3002, 1.2892, 1.2678, 1.2934, 1.2864,
  1.2861, 1.2641, 1.2676, 1.2461, 1.2784, 1.2499, 1.2712, 1.2687, 1.2052, 1.2675, 1.2591,
  1.2520, 1.1657, 1.2434, 1.2478, 1.2299, 1.1960, 1.2383, 1.2298, 1.1848, 1.2306, 1.1457,
  1.2212, 1.2230, 1.2159, 1.1926, 1.1680
};

Spectrum Sun3Light::computeAttenuatedSunlight(float theta, float turbidity) {
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
  float sunSolidAngle = 6.7443e-07;

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

    tauG = exp(-1.41f * k_gCurve.sample(lambda) * m / powf(1.0f + 118.93 * k_gCurve.sample(lambda) * m, 0.45f));

    // Attenuation due to water vapor absorbtion  
    // w - precipitable water vapor in centimeters (standard = 2)
    // Results agree with the graph (pg 132, MI)

    const float w = 2.0f;
    tauWA = exp(-0.2385f * k_waCurve.sample(lambda) * w * m / pow(1.0f + 20.07f * k_waCurve.sample(lambda) * w * m, 0.45f));

    data[i++] = (1.0f / sunSolidAngle) * 100 * solCurve.sample(lambda) * tauR * tauA * tauO * tauG * tauWA;
  }

  RegularSpectrum oSC(data, 350,800,91);

  // Converts to Spectrum
  return oSC.toSpectrum();
}

Sun3Light* Sun3Light::CreateLight(const Transform &light2world, const ParamSet &paramSet) {
  Spectrum L = paramSet.FindOneSpectrum("L", Spectrum(1.0));
  int nSamples = paramSet.FindOneInt("nsamples", 1);
  float turb = paramSet.FindOneFloat("turbidity", 2.0f);

  // Get direction vector of the sun
  Vector sundir = paramSet.FindOneVector("sundir", Vector (0.0f, 0.0f, -1.0f));
  Normalize (sundir);

  return new Sun3Light(light2world, L, nSamples, sundir, turb);
}
