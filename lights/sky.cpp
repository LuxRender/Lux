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

// sky.cpp*
#include "sky.h"
#include "mc.h"
#include "spectrumwavelengths.h"
#include "paramset.h"
#include "reflection/bxdf.h"

#include "data/skychroma_spect.h"

using namespace lux;

class SkyBxDF : public BxDF
{
public:
	SkyBxDF(const SkyLight &sky, const Transform &WL, const Vector &x, const Vector &y, const Vector &z) : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), skyLight(sky), WorldToLight(WL), X(x), Y(y), Z(z) {}
	SWCSpectrum f(const Vector &wo, const Vector &wi) const
	{
		Vector w(wi.x * X.x + wi.y * Y.x + wi.z * Z.x, wi.x * X.y + wi.y * Y.y + wi.z * Z.y, wi.x * X.z + wi.y * Y.z + wi.z * Z.z);
		w = -Normalize(WorldToLight(w));
		const float phi = SphericalPhi(w);
		const float theta = SphericalTheta(w);
		SWCSpectrum L;
		skyLight.GetSkySpectralRadiance(theta, phi, &L);
		return L;
	}
private:
	const SkyLight &skyLight;
	const Transform &WorldToLight;
	Vector X, Y, Z;
};

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

	Vector wh = Normalize(WorldToLight(w));
	const float phi = SphericalPhi(wh);
	const float theta = SphericalTheta(wh);

	SWCSpectrum L;
	GetSkySpectralRadiance(theta,phi,(SWCSpectrum * const)&L);
	L *= skyScale;

	return L;
}
SWCSpectrum SkyLight::Le(const Scene *scene, const Ray &r,
	const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const
{
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	worldRadius *= 1.01f;
	Vector toCenter(worldCenter - r.o);
	float centerDistance = Dot(toCenter, toCenter);
	float approach = Dot(toCenter, r.d);
	float distance = approach + sqrtf(worldRadius * worldRadius - centerDistance + approach * approach);
	Point ps(r.o + distance * r.d);
	Normal ns(Normalize(worldCenter - ps));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Vector(0, 0, 0), Vector (0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(BSDF)(dg, ns);
	(*bsdf)->Add(BSDF_ALLOC(SkyBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns)));
	*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	*pdfDirect = AbsDot(r.d, n) * INV_TWOPI * AbsDot(r.d, ns) / DistanceSquared(r.o, ps);
	Vector wh = Normalize(WorldToLight(r.d));
	const float phi = SphericalPhi(wh);
	const float theta = SphericalTheta(wh);
	SWCSpectrum L;
	GetSkySpectralRadiance(theta, phi, &L);
	L *= skyScale;
	return L;
}

SWCSpectrum SkyLight::Sample_L(const Point &p,
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
		int shapeidx = 0;
		if(nrPortalShapes > 1)
			shapeidx = min<float>(nrPortalShapes - 1,
					Floor2Int(u3 * nrPortalShapes));
		Normal ns;
		Point ps;
		bool found = false;
		for (int i = 0; i < nrPortalShapes; ++i) {
			ps = PortalShapes[shapeidx]->Sample(p, u1, u2, &ns);
			*wi = Normalize(ps - p);

			if (Dot(*wi, ns) < 0.f) {
				found = true;
				break;
			}

			if (++shapeidx >= nrPortalShapes)
				shapeidx = 0;
		}
		if (found)
			*pdf = PortalShapes[shapeidx]->Pdf(p, *wi);
		else {
			*pdf = 0.f;
			return 0.f;
		}
	}
	visibility->SetRay(p, *wi);
	return Le(RayDifferential(p, *wi));
}
float SkyLight::Pdf(const Point &, const Normal &n,
		const Vector &wi) const {
	return AbsDot(n, wi) * INV_TWOPI;
}
SWCSpectrum SkyLight::Sample_L(const Point &p,
		float u1, float u2, float u3, Vector *wi, float *pdf,
		VisibilityTester *visibility) const {
	if(!havePortalShape) {
		*wi = UniformSampleSphere(u1, u2);
		*pdf = UniformSpherePdf();
	} else {
	    // Sample a random Portal
		int shapeidx = 0;
		if(nrPortalShapes > 1)
			shapeidx = min<float>(nrPortalShapes - 1,
					Floor2Int(u3 * nrPortalShapes));
		Normal ns;
		Point ps;
		bool found = false;
		for (int i = 0; i < nrPortalShapes; ++i) {
			ps = PortalShapes[shapeidx]->Sample(p, u1, u2, &ns);
			*wi = Normalize(ps - p);
			if (Dot(*wi, ns) < 0.f) {
				found = true;
				break;
			}

			if (++shapeidx >= nrPortalShapes)
				shapeidx = 0;
		}
		if (found)
			*pdf = PortalShapes[shapeidx]->Pdf(p, *wi);
		else {
			*pdf = 0.f;
			return 0.f;
		}
	}
	visibility->SetRay(p, *wi);
	return Le(RayDifferential(p, *wi));
}
float SkyLight::Pdf(const Point &, const Vector &) const {
	return 1.f / (4.f * M_PI);
}
SWCSpectrum SkyLight::Sample_L(const Scene *scene,
		float u1, float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
	if(!havePortalShape) {
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
	} else {
		// Dade - choose a random portal. This strategy is quite bad if there
		// is more than one portal.
		int shapeidx = 0;
		if(nrPortalShapes > 1) 
			shapeidx = min<float>(nrPortalShapes - 1,
					Floor2Int(lux::random::floatValue() * nrPortalShapes));

		Normal ns;
		ray->o = PortalShapes[shapeidx]->Sample(u1, u2, &ns);
		ray->d = UniformSampleSphere(u3, u4);
		if (Dot(ray->d, ns) < 0.) ray->d *= -1;

		*pdf = PortalShapes[shapeidx]->Pdf(ray->o) * INV_TWOPI / nrPortalShapes;
	}

	return Le(RayDifferential(ray->o, -ray->d));
}
SWCSpectrum SkyLight::Sample_L(const Point &p,
		Vector *wi, VisibilityTester *visibility) const {
	float pdf;
	SWCSpectrum L = Sample_L(p, lux::random::floatValue(), lux::random::floatValue(),
		lux::random::floatValue(), wi, &pdf, visibility);
	if (pdf == 0.f) return Spectrum(0.f);
	return L / pdf;
}
SWCSpectrum SkyLight::Sample_L(const Scene *scene, float u1, float u2, BSDF **bsdf, float *pdf) const
{
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter, &worldRadius);
	worldRadius *= 1.01f;
	Point ps = worldCenter + worldRadius * UniformSampleSphere(u1, u2);
	Normal ns = Normal(Normalize(worldCenter - ps));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Vector(0, 0, 0), Vector (0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(BSDF)(dg, ns);
	(*bsdf)->Add(BSDF_ALLOC(SkyBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns)));
	*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	return SWCSpectrum(skyScale);
}
SWCSpectrum SkyLight::Sample_L(const Scene *scene, const Point &p, const Normal &n,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility) const
{
	Vector wi;
	if(!havePortalShape) {
		// Sample cosine-weighted direction on unit sphere
		float x, y, z;
		ConcentricSampleDisk(u1, u2, &x, &y);
		z = sqrtf(max(0.f, 1.f - x*x - y*y));
		if (u3 < .5)
			z *= -1;
		wi = Vector(x, y, z);
		// Compute _pdf_ for cosine-weighted infinite light direction
		*pdfDirect = fabsf(wi.z) * INV_TWOPI;
		// Transform direction to world space
		Vector v1, v2;
		CoordinateSystem(Normalize(Vector(n)), &v1, &v2);
		wi = Vector(v1.x * wi.x + v2.x * wi.y + n.x * wi.z,
			 v1.y * wi.x + v2.y * wi.y + n.y * wi.z,
			 v1.z * wi.x + v2.z * wi.y + n.z * wi.z);
	} else {
		// Sample a random Portal
		int shapeIndex = 0;
		if(nrPortalShapes > 1)
			shapeIndex = Floor2Int(u3 * nrPortalShapes);
		Normal ns;
		Point ps;
		bool found = false;
		for (int i = 0; i < nrPortalShapes; ++i) {
			ps = PortalShapes[shapeIndex]->Sample(p, u1, u2, &ns);
			wi = Normalize(ps - p);
			if (Dot(wi, ns) < 0.f) {
				found = true;
				break;
			}
			if (++shapeIndex >= nrPortalShapes)
				shapeIndex = 0;
		}
		if (found)
			*pdfDirect = PortalShapes[shapeIndex]->Pdf(p, wi);
		else {
			*pdf = 0.f;
			*pdfDirect = 0.f;
			return 0.f;
		}
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
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Vector(0, 0, 0), Vector (0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(BSDF)(dg, ns);
	(*bsdf)->Add(BSDF_ALLOC(SkyBxDF)(*this, WorldToLight, dpdu, dpdv, Vector(ns)));
	*pdf = 1.f / (4.f * M_PI * worldRadius * worldRadius);
	*pdfDirect *= AbsDot(wi, ns) / DistanceSquared(p, ps);
	visibility->SetSegment(p, ps);
	return SWCSpectrum(skyScale);
}

Light* SkyLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
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

// thread specific wavelengths
extern boost::thread_specific_ptr<SpectrumWavelengths> thread_wavelengths;

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
	// Change to full spectrum to have correct scale factor
	*dst_spect *= (Y / 30.35/*dst_spect->y()*/ * 0.00000260f); // lyc - nasty scaling factor :( // radiance - tweaked - was 0.00000165f
	// Jeanphi - hard value to avoid problems with degraded spectra
}

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
