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

// sky.cpp*
#include "sky.h"

using namespace lux;

// SkyLight Method Definitions
SkyLight::~SkyLight() {
}

SkyLight::SkyLight(const Transform &light2world,
		                const float skyscale, int ns, Vector sd, float turb,
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

	float chi = (4.0/9.0 - T / 120.0) * (M_PI - 2 * thetaS);
	zenith_Y = (4.0453 * T - 4.9710) * tan(chi) - .2155 * T + 2.4192;
	zenith_Y *= 1000;  // conversion from kcd/m^2 to cd/m^2

	zenith_x =
	(+0.00165*theta3 - 0.00374*theta2 + 0.00208*thetaS + 0)          * T2 +
	(-0.02902*theta3 + 0.06377*theta2 - 0.03202*thetaS + 0.00394) * T +
	(+0.11693*theta3 - 0.21196*theta2 + 0.06052*thetaS + 0.25885);

	zenith_y =
	(+0.00275*theta3 - 0.00610*theta2 + 0.00316*thetaS  + 0)          * T2 +
	(-0.04214*theta3 + 0.08970*theta2 - 0.04153*thetaS  + 0.00515) * T +
	(+0.15346*theta3 - 0.26756*theta2 + 0.06669*thetaS  + 0.26688);

	perez_Y[1] =  (0.17872 *T  - 1.46303)*aconst;
	perez_Y[2] =  (-0.35540 *T  + 0.42749)*bconst;
	perez_Y[3] =  (-0.02266 *T  + 5.32505)*cconst;
	perez_Y[4] =  (0.12064 *T  - 2.57705)*dconst;
	perez_Y[5] =  (-0.06696 *T  + 0.37027)*econst;

	perez_x[1] =  (-0.01925 *T  - 0.25922)*aconst;
	perez_x[2] =  (-0.06651 *T  + 0.00081)*bconst;
	perez_x[3] =  (-0.00041 *T  + 0.21247)*cconst;
	perez_x[4] =  (-0.06409 *T  - 0.89887)*dconst;
	perez_x[5] =  (-0.00325 *T  + 0.04517)*econst;

	perez_y[1] =  (-0.01669 *T  - 0.26078)*aconst;
	perez_y[2] =  (-0.09495 *T  + 0.00921)*bconst;
	perez_y[3] =  (-0.00792 *T  + 0.21023)*cconst;
	perez_y[4] =  (-0.04405 *T  - 1.65369)*dconst;
	perez_y[5] =  (-0.01092 *T  + 0.05291)*econst;
}

SWCSpectrum SkyLight::Le(const RayDifferential &r) const {
	Vector w = r.d;
	// Compute sky light radiance for direction
	SWCSpectrum L;

	Vector wh = Normalize(WorldToLight(w));
	const float phi = SphericalPhi(wh);
	const float theta = SphericalTheta(wh);

	GetSkySpectralRadiance(theta,phi,(SWCSpectrum * const)&L);
	L *= skyScale;

	return L;
}

SWCSpectrum SkyLight::Sample_L(const Point &p,
		const Normal &n, float u1, float u2,
		Vector *wi, float *pdf,
		VisibilityTester *visibility) const {
	if(!havePortalShape) {
		// Sample cosine-weighted direction on unit sphere
		float x, y, z;
		ConcentricSampleDisk(u1, u2, &x, &y);
		z = sqrtf(max(0.f, 1.f - x*x - y*y));
		if (lux::random::floatValue() < .5) z *= -1;
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
		int shapeidx = 0;
		if(nrPortalShapes > 1)
			shapeidx = Floor2Int(lux::random::floatValue() * nrPortalShapes);
		Normal ns;
		Point ps = PortalShapes[shapeidx]->Sample(p, u1, u2, &ns);
		*wi = Normalize(ps - p);
		*pdf = PortalShapes[shapeidx]->Pdf(p, *wi);
	}
	visibility->SetRay(p, *wi);
	return Le(RayDifferential(p, *wi));
}
float SkyLight::Pdf(const Point &, const Normal &n,
		const Vector &wi) const {
	return AbsDot(n, wi) * INV_TWOPI;
}
SWCSpectrum SkyLight::Sample_L(const Point &p,
		float u1, float u2, Vector *wi, float *pdf,
		VisibilityTester *visibility) const {
	if(!havePortalShape) {
		*wi = UniformSampleSphere(u1, u2);
		*pdf = UniformSpherePdf();
	} else {
	    // Sample a random Portal
		int shapeidx = 0;
		if(nrPortalShapes > 1)
			shapeidx = Floor2Int(lux::random::floatValue() * nrPortalShapes);
		Normal ns;
		Point ps = PortalShapes[shapeidx]->Sample(p, u1, u2, &ns);
		*wi = Normalize(ps - p);
		*pdf = PortalShapes[shapeidx]->Pdf(p, *wi);
	}
	visibility->SetRay(p, *wi);
	return Le(RayDifferential(p, *wi));
}
float SkyLight::Pdf(const Point &, const Vector &) const {
	return 1.f / (4.f * M_PI);
}
SWCSpectrum SkyLight::Sample_L(const Scene *scene,									// TODO - radiance - add portal implementation
		float u1, float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
	// Choose two points _p1_ and _p2_ on scene bounding sphere
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter,
	                                    &worldRadius);
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
	*pdf =
		costheta / ((4.f * M_PI * worldRadius * worldRadius));
	return Le(RayDifferential(ray->o, -ray->d));
}
SWCSpectrum SkyLight::Sample_L(const Point &p,
		Vector *wi, VisibilityTester *visibility) const {
	float pdf;
	SWCSpectrum L = Sample_L(p, lux::random::floatValue(), lux::random::floatValue(),
		wi, &pdf, visibility);
	if (pdf == 0.f) return Spectrum(0.f);
	return L / pdf;
}
Light* SkyLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
	float scale = paramSet.FindOneFloat("gain", 0.005f);				// gain (aka scale) factor to apply to sun/skylight (0.005)
	int nSamples = paramSet.FindOneInt("nsamples", 1);
	Vector sundir = paramSet.FindOneVector("sundir", Vector(0,0,1));	// direction vector of the sun
	Normalize(sundir);
	float turb = paramSet.FindOneFloat("turbidity", 2.0f);			// [in] turb  Turbidity (1.0,30+) 2-6 are most useful for clear days.
	float aconst = paramSet.FindOneFloat("aconst", 1.0f);				// Perez function multiplicative constants
	float bconst = paramSet.FindOneFloat("bconst", 1.0f);
	float cconst = paramSet.FindOneFloat("cconst", 1.0f);
	float dconst = paramSet.FindOneFloat("dconst", 1.0f);
	float econst = paramSet.FindOneFloat("econst", 1.0f);

	return new SkyLight(light2world, scale, nSamples, sundir, turb, aconst, bconst, cconst, dconst, econst);
}

/* All angles in radians, theta angles measured from normal */
inline float RiAngleBetween(float thetav, float phiv, float theta, float phi)
{
  float cospsi = sin(thetav) * sin(theta) * cos(phi-phiv) + cos(thetav) * cos(theta);
  if (cospsi > 1) return 0;
  if (cospsi < -1) return M_PI;
  return  acos(cospsi);
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


inline float SkyLight::PerezFunction(const float *lam, float theta, float gamma, float lvz) const
{
	float den = ((1 + lam[1]*exp(lam[2])) *
		(1 + lam[3]*exp(lam[4]*thetaS) + lam[5]*cos(thetaS)*cos(thetaS)));
	float num = ((1 + lam[1]*exp(lam[2]/cos(theta))) *
		(1 + lam[3]*exp(lam[4]*gamma)  + lam[5]*cos(gamma)*cos(gamma)));
	return lvz* num/den;
}

// note - lyc - optimised return call to not create temporaries, passed in scale factor
void SkyLight::GetSkySpectralRadiance(const float theta, const float phi, SWCSpectrum * const dst_spect) const
{
	// add bottom half of hemisphere with horizon colour
	const float theta_fin = min(theta,(M_PI * 0.5f) - 0.001f);
	const float gamma = RiAngleBetween(theta,phi,thetaS,phiS);

	// Compute xyY values
	const float x = PerezFunction(perez_x, theta_fin, gamma, zenith_x);
	const float y = PerezFunction(perez_y, theta_fin, gamma, zenith_y);
	const float Y = PerezFunction(perez_Y, theta_fin, gamma, zenith_Y);

	ChromaticityToSpectrum(x,y,dst_spect);
	*dst_spect *= (Y / dst_spect->y() * 0.00000165f); // lyc - nasty scaling factor :(
}

//300-830 10nm
static float S0Amplitudes[54] = {
0.04,6.0,29.6,55.3,57.3,
61.8,61.5,68.8,63.4,65.8,
94.8,104.8,105.9,96.8,113.9,
125.6,125.5,121.3,121.3,113.5,
113.1,110.8,106.5,108.8,105.3,
104.4,100.0,96.0,95.1,89.1,
90.5,90.3,88.4,84.0,85.1,
81.9,82.6,84.9,81.3,71.9,
74.3,76.4,63.3,71.7,77.0,
65.2,47.7,68.6,65.0,66.0,
61.0,53.3,58.9,61.9
};

static float S1Amplitudes[54] = {
0.02,4.5,22.4,42.0,40.6,
41.6,38.0,42.4,38.5,35.0,
43.4,46.3,43.9,37.1,36.7,
35.9,32.6,27.9,24.3,20.1,
16.2,13.2,8.6,6.1,4.2,
1.9,0.0,-1.6,-3.5,-3.5,
-5.8,-7.2,-8.6,-9.5,-10.9,
-10.7,-12.0,-14.0,-13.6,-12.0,
-13.3,-12.9,-10.6,-11.6,-12.2,
-10.2,-7.8,-11.2,-10.4,-10.6,
-9.7,-8.3,-9.3,-9.8
};

static float S2Amplitudes[54] = {
0.0,2.0,4.0,8.5,7.8,
6.7,5.3,6.1,3.0,1.2,
-1.1,-0.5,-0.7,-1.2,-2.6,
-2.9,-2.8,-2.6,-2.6,-1.8,
-1.5,-1.3,-1.2,-1.0,-0.5,
-0.3,0.0,0.2,0.5,2.1,
3.2,4.1,4.7,5.1,6.7,
7.3,8.6,9.8,10.2,8.3,
9.6,8.5,7.0,7.6,8.0,
6.7,5.2,7.4,6.8,7.0,
6.4,5.5,6.1,6.5
};

// thread specific wavelengths
extern boost::thread_specific_ptr<SpectrumWavelengths> thread_wavelengths;

// note - lyc - removed redundant computations and optimised
void SkyLight::ChromaticityToSpectrum(const float x, const float y, SWCSpectrum * const dst_spect) const
{
	const float den = 1.0f / (0.0241f + 0.2562f * x - 0.7341f * y);
	const float M1 = (-1.3515f -  1.7703f * x +  5.9114f * y) * den;
	const float M2 = ( 0.03f   - 31.4424f * x + 30.0717f * y) * den;

	for (unsigned int j = 0; j < WAVELENGTH_SAMPLES; ++j)
	{
		const float w = (thread_wavelengths->w[j] - 300.0f) * 0.1018867f;
		const int i  = Floor2Int(w);
		const int i1 = i + 1;

		const float b = w - float(i);
		const float a = 1.0f - b;

		const float t0 = S0Amplitudes[i] * a + S0Amplitudes[i1] * b;
		const float t1 = S1Amplitudes[i] * a + S1Amplitudes[i1] * b;
		const float t2 = S2Amplitudes[i] * a + S2Amplitudes[i1] * b;

		dst_spect->c[j] = t0 + M1 * t1 + M2 * t2;
	}
}
