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

// reflection.cpp*
#include "reflection.h"
#include "color.h"
#include "spectrum.h"
#include "mc.h"
#include "sampling.h"
#include <stdarg.h>

using namespace lux;

namespace lux
{
// BxDF Utility Functions
 SWCSpectrum FrDiel(float cosi, float cost,
                        const SWCSpectrum &etai,
						const SWCSpectrum &etat) {
        SWCSpectrum Rparl = ((etat * cosi) - (etai * cost)) /
                         ((etat * cosi) + (etai * cost));
        SWCSpectrum Rperp = ((etai * cosi) - (etat * cost)) /
                         ((etai * cosi) + (etat * cost));
	return (Rparl*Rparl + Rperp*Rperp) / 2.f;
}
 SWCSpectrum FrCond(float cosi,
                         const SWCSpectrum &eta,
					     const SWCSpectrum &k) {
    SWCSpectrum tmp = (eta*eta + k*k) * cosi*cosi;
    SWCSpectrum Rparl2 = (tmp - (2.f * eta * cosi) + 1) /
    	(tmp + (2.f * eta * cosi) + 1);
    SWCSpectrum tmp_f = eta*eta + k*k;
    SWCSpectrum Rperp2 =
		(tmp_f - (2.f * eta * cosi) + cosi*cosi) /
	    (tmp_f + (2.f * eta * cosi) + cosi*cosi);
    return (Rparl2 + Rperp2) / 2.f;
}
 SWCSpectrum FresnelApproxEta(const SWCSpectrum &Fr) {
	SWCSpectrum reflectance = Fr.Clamp(0.f, .999f);
	return (SWCSpectrum(1.) + reflectance.Sqrt()) /
		(SWCSpectrum(1.) - reflectance.Sqrt());
}
 SWCSpectrum FresnelApproxK(const SWCSpectrum &Fr) {
	SWCSpectrum reflectance = Fr.Clamp(0.f, .999f);
	return 2.f * (reflectance /
	              (SWCSpectrum(1.) - reflectance)).Sqrt();
}
 
}//namespace lux

// BxDF Method Definitions
SWCSpectrum BRDFToBTDF::f(const Vector &wo,
                       const Vector &wi) const {
	return brdf->f(wo, otherHemisphere(wi));
}
SWCSpectrum BRDFToBTDF::Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf) const {
	SWCSpectrum f = brdf->Sample_f(wo, wi, u1, u2, pdf);
	*wi = otherHemisphere(*wi);
	return f;
}
Fresnel::~Fresnel() { }
SWCSpectrum FresnelConductor::Evaluate(float cosi) const {
	return FrCond(fabsf(cosi), eta, k);
}
SWCSpectrum FresnelDielectric::Evaluate(float cosi) const {
	// Compute Fresnel reflectance for dielectric
	cosi = Clamp(cosi, -1.f, 1.f);
	// Compute indices of refraction for dielectric
	bool entering = cosi > 0.;
	float ei = eta_i, et = eta_t;
	if (!entering)
		swap(ei, et);
	// Compute _sint_ using Snell's law
	float sint = ei/et * sqrtf(max(0.f, 1.f - cosi*cosi));
	if (sint > 1.) {
		// Handle total internal reflection
		return 1.;
	}
	else {
		float cost = sqrtf(max(0.f, 1.f - sint*sint));
		return FrDiel(fabsf(cosi), cost, ei, et);
	}
}

SWCSpectrum FresnelSlick::Evaluate(float cosi) const {
  return normal_incidence + (1.0f - normal_incidence) * powf (1.0 - cosi, 5.0f);
}

FresnelSlick::FresnelSlick (float ni) {
  normal_incidence = ni;
}

SWCSpectrum SpecularReflection::Sample_f(const Vector &wo,
		Vector *wi, float u1, float u2, float *pdf) const {
	// Compute perfect specular reflection direction
	*wi = Vector(-wo.x, -wo.y, wo.z);
	*pdf = 1.f;
	return fresnel->Evaluate(CosTheta(wo)) * R /
		fabsf(CosTheta(*wi));
}

extern boost::thread_specific_ptr<SpectrumWavelengths> thread_wavelengths;

SWCSpectrum SpecularTransmission::Sample_f(const Vector &wo,
		Vector *wi, float u1, float u2, float *pdf) const {
	// Figure out which $\eta$ is incident and which is transmitted
	bool entering = CosTheta(wo) > 0.;
	float ei = etai, et = etat;

	if(cb != 0.) {
		// Handle dispersion using cauchy formula
		float w = thread_wavelengths->SampleSingle();
		et = etat + (cb * 1000000) / (w*w);
	}

	if (!entering)
		swap(ei, et);
	// Compute transmitted ray direction
	float sini2 = SinTheta2(wo);
	float eta = ei / et;
	float sint2 = eta * eta * sini2;
	// Handle total internal reflection for transmission
	if (sint2 > 1.) return 0.;
	float cost = sqrtf(max(0.f, 1.f - sint2));
	if (entering) cost = -cost;
	float sintOverSini = eta;
	*wi = Vector(sintOverSini * -wo.x,
	             sintOverSini * -wo.y,
				 cost);
	*pdf = 1.f;
	SWCSpectrum F = fresnel.Evaluate(CosTheta(wo));
	return (et*et)/(ei*ei) * (SWCSpectrum(1.)-F) * T /
		fabsf(CosTheta(*wi));
}

SWCSpectrum Lambertian::f(const Vector &wo,
		const Vector &wi) const {
	return RoverPI;
}
SWCSpectrum OrenNayar::f(const Vector &wo,
		const Vector &wi) const {
	float sinthetai = SinTheta(wi);
	float sinthetao = SinTheta(wo);
	// Compute cosine term of Oren--Nayar model
	float maxcos = 0.f;
	if (sinthetai > 1e-4 && sinthetao > 1e-4) {
		float sinphii = SinPhi(wi), cosphii = CosPhi(wi);
		float sinphio = SinPhi(wo), cosphio = CosPhi(wo);
		float dcos = cosphii * cosphio + sinphii * sinphio;
		maxcos = max(0.f, dcos);
	}
	// Compute sine and tangent terms of Oren--Nayar model
	float sinalpha, tanbeta;
	if (fabsf(CosTheta(wi)) > fabsf(CosTheta(wo))) {
		sinalpha = sinthetao;
		tanbeta = sinthetai / fabsf(CosTheta(wi));
	}
	else {
		sinalpha = sinthetai;
		tanbeta = sinthetao / fabsf(CosTheta(wo));
	}
	return R * INV_PI *
	       (A + B * maxcos * sinalpha * tanbeta);
}
Microfacet::Microfacet(const SWCSpectrum &reflectance,
                       Fresnel *f,
					   MicrofacetDistribution *d)
	: BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
	 R(reflectance), distribution(d), fresnel(f) {
}
SWCSpectrum Microfacet::f(const Vector &wo,
                       const Vector &wi) const {
	float cosThetaO = fabsf(CosTheta(wo));
	float cosThetaI = fabsf(CosTheta(wi));
	Vector wh = Normalize(wi + wo);
	float cosThetaH = Dot(wi, wh);
	SWCSpectrum F = fresnel->Evaluate(cosThetaH);
	return R * distribution->D(wh) * G(wo, wi, wh) * F /
		 (4.f * cosThetaI * cosThetaO);
}
Lafortune::Lafortune(const SWCSpectrum &r, u_int nl,
	const SWCSpectrum *xx,
	const SWCSpectrum *yy,
	const SWCSpectrum *zz,
	const SWCSpectrum *e, BxDFType t)
	: BxDF(t), R(r) {
	nLobes = nl;
	x = xx;
	y = yy;
	z = zz;
	exponent = e;
}
SWCSpectrum Lafortune::f(const Vector &wo,
                      const Vector &wi) const {
	SWCSpectrum ret = R * INV_PI;
	for (u_int i = 0; i < nLobes; ++i) {
		// Add contribution for $i$th Phong lobe
		SWCSpectrum v = x[i] * wo.x * wi.x + y[i] * wo.y * wi.y +
			z[i] * wo.z * wi.z;
		ret += v.Pow(exponent[i]);
	}
	return ret;
}
FresnelBlend::FresnelBlend(const SWCSpectrum &d,
                           const SWCSpectrum &s,
						   MicrofacetDistribution *dist)
	: BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
	  Rd(d), Rs(s) {
	distribution = dist;
}
SWCSpectrum FresnelBlend::f(const Vector &wo,
                         const Vector &wi) const {
	SWCSpectrum diffuse = (28.f/(23.f*M_PI)) * Rd *
		(SWCSpectrum(1.) - Rs) *
		(1 - powf(1 - .5f * fabsf(CosTheta(wi)), 5)) *
		(1 - powf(1 - .5f * fabsf(CosTheta(wo)), 5));
	Vector H = Normalize(wi + wo);
	SWCSpectrum specular = distribution->D(H) /
		(4.f * AbsDot(wi, H) *
		max(fabsf(CosTheta(wi)), fabsf(CosTheta(wo)))) *
		SchlickFresnel(Dot(wi, H));
	return diffuse + specular;
}

CookTorrance::CookTorrance(const SWCSpectrum &kd, u_int nl,
                           const SWCSpectrum *ks,
                           MicrofacetDistribution **dist, Fresnel **fres) : BxDF(BxDFType(BSDF_REFLECTION | BSDF_SPECULAR)) {
  KD = kd;
  KS = ks;
  nLobes = nl;
  distribution = dist;
  fresnel = fres;
}

SWCSpectrum CookTorrance::f(const Vector &wo, const Vector &wi) const {
  SWCSpectrum ret = KD * INV_PI;

  float cosThetaO = fabsf(CosTheta(wo));
  float cosThetaI = fabsf(CosTheta(wi));
  Vector wh = Normalize(wi + wo);
  float cosThetaH = Dot(wi, wh);
  float cG = G(wo, wi, wh);

  for (u_int i = 0; i < nLobes; i++) {
    // Add contribution for $i$th Cook-Torrance lobe

    ret += KS[i] * distribution[i]->D(wh) * cG * fresnel[i]->Evaluate(cosThetaH) / (M_PI * cosThetaI * cosThetaO);
  }
  return ret;
}

float CookTorrance::G(const Vector &wo, const Vector &wi, const Vector &wh) const {
  float NdotWh = fabsf(CosTheta(wh));
  float NdotWo = fabsf(CosTheta(wo));
  float NdotWi = fabsf(CosTheta(wi));
  float WOdotWh = AbsDot(wo, wh);
  return min(1.f, min((2.f * NdotWh * NdotWo / WOdotWh), (2.f * NdotWh * NdotWi / WOdotWh)));
}

SWCSpectrum BxDF::Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf) const {
	// Cosine-sample the hemisphere, flipping the direction if necessary
	*wi = CosineSampleHemisphere(u1, u2);
	if (wo.z < 0.) wi->z *= -1.f;
	*pdf = Pdf(wo, *wi);
	return f(wo, *wi);
}
float BxDF::Pdf(const Vector &wo, const Vector &wi) const {
	return
		SameHemisphere(wo, wi) ? fabsf(wi.z) * INV_PI : 0.f;
}
float BRDFToBTDF::Pdf(const Vector &wo,
		const Vector &wi) const {
	return brdf->Pdf(wo, -wi);
}
SWCSpectrum Microfacet::Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf) const {
	distribution->Sample_f(wo, wi, u1, u2, pdf);
	if (!SameHemisphere(wo, *wi)) return SWCSpectrum(0.f);
	return f(wo, *wi);
}
float Microfacet::Pdf(const Vector &wo,
		const Vector &wi) const {
	if (!SameHemisphere(wo, wi)) return 0.f;
	return distribution->Pdf(wo, wi);
}

Beckmann::Beckmann(float rms) {
  r = rms;
}

float Beckmann::D(const Vector &wh) const {
  float costhetah = CosTheta(wh);
  float theta = acos(costhetah);
  float tanthetah = tan(theta);

  float dfac = tanthetah / r;

  return exp(-(dfac * dfac)) / (r * r * powf(costhetah, 4.0));
}

void Beckmann::Sample_f(const Vector &wo, Vector *wi, float u1, float u2, float *pdf) const {
  // Compute sampled half-angle vector $\wh$ for Beckmann distribution
  // Adapted from B. Walter et al, Microfacet Models for Refraction, Eurographics Symposium on Rendering, 2007, page 7

  float theta = atan (sqrt (-(r * r) * log(1.0 - u1)));
  float costheta = cos (theta);
  float sintheta = sqrtf(max(0.f, 1.f - costheta*costheta));
  float phi = u2 * 2.f * M_PI;

  Vector H = SphericalDirection(sintheta, costheta, phi);

  if (!SameHemisphere(wo, H))
    H.z *= -1.f;

  // Compute incident direction by reflecting about $\wh$
  *wi = -wo + 2.f * Dot(wo, H) * H;

  // Compute PDF for \wi from Beckmann distribution - note that the inverse of the integral over
  // the Beckmann distribution is not available in closed form, so this is not really correct
  // (see Kelemen and Szirmay-Kalos / Microfacet Based BRDF Model, Eurographics 2001)

  float conversion_factor = 1.0 / (4.f * Dot(wo, H));
  float beckmann_pdf = conversion_factor * D(H);

  *pdf = beckmann_pdf;
}

// NB: See note above!
float Beckmann::Pdf(const Vector &wo, const Vector &wi) const {
  Vector H = Normalize(wo + wi);
  float conversion_factor = 1.0 / 4.f * Dot(wo, H);
  float beckmann_pdf = conversion_factor * D(H);

  return beckmann_pdf;
}

// Ward isotropic distribution, adapted from Kelemen and Szirmay-Kalos / Microfacet Based BRDF Model, Eurographics 2001
WardIsotropic::WardIsotropic(float rms) {
  r = rms;
}

float WardIsotropic::D(const Vector &wh) const {
  float costhetah = CosTheta(wh);
  float theta = acos(costhetah);
  float tanthetah = tan(theta);

  float dfac = tanthetah / r;

  return exp(-(dfac * dfac)) / (M_PI * r * r * powf(costhetah, 3.0));
}

void WardIsotropic::Sample_f(const Vector &wo, Vector *wi, float u1, float u2, float *pdf) const {
  // Compute sampled half-angle vector $\wh$ for Ward distribution

  float theta = atan (r * sqrt (-log(1.0 - u1)));
  float costheta = cos (theta);
  float sintheta = sqrtf(max(0.f, 1.f - costheta*costheta));
  float phi = u2 * 2.f * M_PI;

  Vector H = SphericalDirection(sintheta, costheta, phi);

  if (!SameHemisphere(wo, H))
    H.z *= -1.f;

  // Compute incident direction by reflecting about $\wh$
  *wi = -wo + 2.f * Dot(wo, H) * H;

  // Compute PDF for \wi from isotropic Ward distribution

  float conversion_factor = 1.0 / (4.f * Dot(wo, H));
  float ward_pdf = conversion_factor * D(H);

  *pdf = ward_pdf;
}

float WardIsotropic::Pdf(const Vector &wo, const Vector &wi) const {
  Vector H = Normalize(wo + wi);
  float conversion_factor = 1.0 / 4.f * Dot(wo, H);
  float ward_pdf = conversion_factor * D(H);

  return ward_pdf;
}

void Blinn::Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf) const {
	// Compute sampled half-angle vector $\wh$ for Blinn distribution
	float costheta = powf(u1, 1.f / (exponent+2.0f));
	float sintheta = sqrtf(max(0.f, 1.f - costheta*costheta));
	float phi = u2 * 2.f * M_PI;
	Vector H = SphericalDirection(sintheta, costheta, phi);
	if (!SameHemisphere(wo, H)) H.z *= -1.f;
	// Compute incident direction by reflecting about $\wh$
	*wi = -wo + 2.f * Dot(wo, H) * H;
	// Compute PDF for \wi from Blinn distribution
	float blinn_pdf = ((exponent + 2.f) *
	                   powf(costheta, exponent)) /
		(2.f * M_PI * 4.f * Dot(wo, H));

	*pdf = blinn_pdf;
}
float Blinn::Pdf(const Vector &wo, const Vector &wi) const {
	Vector H = Normalize(wo + wi);
	float costheta = fabsf(H.z);
	// Compute PDF for \wi from Blinn distribution
	float blinn_pdf = ((exponent + 2.f) *
	                   powf(costheta, exponent)) /
		(2.f * M_PI * 4.f * Dot(wo, H));
	return blinn_pdf;
}
void Anisotropic::Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf) const {
	// Sample from first quadrant and remap to hemisphere to sample \wh
	float phi, costheta;
	if (u1 < .25f) {
		sampleFirstQuadrant(4.f * u1, u2, &phi, &costheta);
	} else if (u1 < .5f) {
		u1 = 4.f * (.5f - u1);
		sampleFirstQuadrant(u1, u2, &phi, &costheta);
		phi = M_PI - phi;
	} else if (u1 < .75f) {
		u1 = 4.f * (u1 - .5f);
		sampleFirstQuadrant(u1, u2, &phi, &costheta);
		phi += M_PI;
	} else {
		u1 = 4.f * (1.f - u1);
		sampleFirstQuadrant(u1, u2, &phi, &costheta);
		phi = 2.f * M_PI - phi;
	}
	float sintheta = sqrtf(max(0.f, 1.f - costheta*costheta));
	Vector H = SphericalDirection(sintheta, costheta, phi);
	if (Dot(wo, H) < 0.f) H = -H;
	// Compute incident direction by reflecting about $\wh$
	*wi = -wo + 2.f * Dot(wo, H) * H;
	// Compute PDF for \wi from Anisotropic distribution
	float anisotropic_pdf = D(H) / (4.f * Dot(wo, H));
	*pdf = anisotropic_pdf;
}
void Anisotropic::sampleFirstQuadrant(float u1, float u2,
		float *phi, float *costheta) const {
	if (ex == ey)
		*phi = M_PI * u1 * 0.5f;
	else
		*phi = atanf(sqrtf((ex+1)/(ey+1)) *
			tanf(M_PI * u1 * 0.5f));
	float cosphi = cosf(*phi), sinphi = sinf(*phi);
	*costheta = powf(u2, 1.f/(ex * cosphi * cosphi +
		ey * sinphi * sinphi + 1));
}
float Anisotropic::Pdf(const Vector &wo,
		const Vector &wi) const {
	Vector H = Normalize(wo + wi);
	// Compute PDF for \wi from Anisotropic distribution
	float anisotropic_pdf = D(H) / (4.f * Dot(wo, H));
	return anisotropic_pdf;
}
SWCSpectrum Lafortune::Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf) const {
	u_int comp = lux::random::uintValue() % (nLobes+1);
	if (comp == nLobes) {
		// Cosine-sample the hemisphere, flipping the direction if necessary
		*wi = CosineSampleHemisphere(u1, u2);
		if (wo.z < 0.) wi->z *= -1.f;
	}
	else {
		// Sample lobe _comp_ for Lafortune BRDF
		float xlum = x[comp].y();
		float ylum = y[comp].y();
		float zlum = z[comp].y();
		float costheta = powf(u1, 1.f / (.8f * exponent[comp].y() + 1));
		float sintheta = sqrtf(max(0.f, 1.f - costheta*costheta));
		float phi = u2 * 2.f * M_PI;
		Vector lobeCenter = Normalize(Vector(xlum * wo.x, ylum * wo.y, zlum * wo.z));
		Vector lobeX, lobeY;
		CoordinateSystem(lobeCenter, &lobeX, &lobeY);
		*wi = SphericalDirection(sintheta, costheta, phi, lobeX, lobeY,
			lobeCenter);
	}
	if (!SameHemisphere(wo, *wi)) return SWCSpectrum(0.f);
	*pdf = Pdf(wo, *wi);
	return f(wo, *wi);
}
float Lafortune::Pdf(const Vector &wo, const Vector &wi) const {
	if (!SameHemisphere(wo, wi)) return 0.f;
	float pdfSum = fabsf(wi.z) * INV_PI;
	for (u_int i = 0; i < nLobes; ++i) {
		float xlum = x[i].y();
		float ylum = y[i].y();
		float zlum = z[i].y();
		Vector lobeCenter =
			Normalize(Vector(wo.x * xlum, wo.y * ylum, wo.z * zlum));
		float e = .8f * exponent[i].y();
		pdfSum += (e + 1.f) * powf(max(0.f, Dot(wi, lobeCenter)), e);
	}
	return pdfSum / (1.f + nLobes);
}
SWCSpectrum FresnelBlend::Sample_f(const Vector &wo,
		Vector *wi, float u1, float u2, float *pdf) const {
	if (u1 < .5) {
		u1 = 2.f * u1;
		// Cosine-sample the hemisphere, flipping the direction if necessary
		*wi = CosineSampleHemisphere(u1, u2);
		if (wo.z < 0.) wi->z *= -1.f;
	}
	else {
		u1 = 2.f * (u1 - .5f);
		distribution->Sample_f(wo, wi, u1, u2, pdf);
		if (!SameHemisphere(wo, *wi)) return SWCSpectrum(0.f);
	}
	*pdf = Pdf(wo, *wi);
	return f(wo, *wi);
}
float FresnelBlend::Pdf(const Vector &wo,
		const Vector &wi) const {
	if (!SameHemisphere(wo, wi)) return 0.f;
	return .5f * (fabsf(wi.z) * INV_PI +
		distribution->Pdf(wo, wi));
}

SWCSpectrum CookTorrance::Sample_f(const Vector &wo, Vector *wi, float u1, float u2, float *pdf) const {
  // Pick a random component
  u_int comp = lux::random::uintValue() % (nLobes+1);

  if (comp == nLobes) {
    // The diffuse term; cosine-sample the hemisphere, flipping the direction if necessary
    *wi = CosineSampleHemisphere(u1, u2);
    if (wo.z < 0.)
      wi->z *= -1.f;
  }
  else {
    // Sample lobe number _comp_ for Cook-Torrance BRDF
   distribution[comp]->Sample_f(wo, wi, u1, u2, pdf);
  }
  // If outgoing and incoming is in different hemispheres, return None
  if (!SameHemisphere(wo, *wi))
    return SWCSpectrum(0.f);

  *pdf = Pdf(wo, *wi);
  return f(wo, *wi);
}

float CookTorrance::Pdf(const Vector &wo, const Vector &wi) const {
  if (!SameHemisphere(wo, wi))
    return 0.f;

  // Average of all pdf's
  float pdfSum = fabsf(wi.z) * INV_PI;

  for (u_int i = 0; i < nLobes; ++i) {
    pdfSum += distribution[i]->Pdf(wo, wi);
  }
  return pdfSum / (1.f + nLobes);
}

SWCSpectrum BxDF::rho(const Vector &w, int nSamples,
		float *samples) const {
	if (!samples) {
		samples =
			(float *)alloca(2 * nSamples * sizeof(float));
		LatinHypercube(samples, nSamples, 2);
	}
	SWCSpectrum r = 0.;
	for (int i = 0; i < nSamples; ++i) {
		// Estimate one term of $\rho_{dh}$
		Vector wi;
		float pdf = 0.f;
		SWCSpectrum f =
			Sample_f(w, &wi, samples[2*i], samples[2*i+1], &pdf);
		if (pdf > 0.) r += f * fabsf(wi.z) / pdf;
	}
	return r / nSamples;
}
SWCSpectrum BxDF::rho(int nSamples, float *samples) const {
	if (!samples) {
		samples =
			(float *)alloca(4 * nSamples * sizeof(float));
		LatinHypercube(samples, nSamples, 4);
	}
	SWCSpectrum r = 0.;
	for (int i = 0; i < nSamples; ++i) {
		// Estimate one term of $\rho_{hh}$
		Vector wo, wi;
		wo = UniformSampleHemisphere(samples[4*i], samples[4*i+1]);
		float pdf_o = INV_TWOPI, pdf_i = 0.f;
		SWCSpectrum f =
			Sample_f(wo, &wi, samples[4*i+2], samples[4*i+3],
				&pdf_i);
		if (pdf_i > 0.)
			r += f * fabsf(wi.z * wo.z) / (pdf_o * pdf_i);
	}
	return r / (M_PI*nSamples);
}
// BSDF Method Definitions
SWCSpectrum BSDF::Sample_f(const Vector &wo, Vector *wi, BxDFType flags,
	BxDFType *sampledType) const {
	float pdf;
	SWCSpectrum f = Sample_f(wo, wi, lux::random::floatValue(), lux::random::floatValue(),
		lux::random::floatValue(), &pdf, flags, sampledType);
	if (!f.Black() && pdf > 0.) f /= pdf;
	return f;
}
SWCSpectrum BSDF::Sample_f(const Vector &woW, Vector *wiW,
		float u1, float u2, float u3, float *pdf,
		BxDFType flags, BxDFType *sampledType) const {
	// Choose which _BxDF_ to sample
	int matchingComps = NumComponents(flags);
	if (matchingComps == 0) {
		*pdf = 0.f;
		return SWCSpectrum(0.f);
	}
	int which = min(Floor2Int(u3 * matchingComps),
		matchingComps-1);
	BxDF *bxdf = NULL;
	int count = which;
	for (int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags))
			if (count-- == 0) {
				bxdf = bxdfs[i];
				break;
			}
	BOOST_ASSERT(bxdf); // NOBOOK
	// Sample chosen _BxDF_
	Vector wi;
	Vector wo = WorldToLocal(woW);
	*pdf = 0.f;
	SWCSpectrum f = bxdf->Sample_f(wo, &wi, u1, u2, pdf);
	if (*pdf == 0.f) return 0.f;
	if (sampledType) *sampledType = bxdf->type;
	*wiW = LocalToWorld(wi);
	// Compute overall PDF with all matching _BxDF_s
	if (!(bxdf->type & BSDF_SPECULAR) && matchingComps > 1) {
		for (int i = 0; i < nBxDFs; ++i) {
			if (bxdfs[i] != bxdf &&
			    bxdfs[i]->MatchesFlags(flags))
				*pdf += bxdfs[i]->Pdf(wo, wi);
		}
	}
	if (matchingComps > 1) *pdf /= matchingComps;
	// Compute value of BSDF for sampled direction
	if (!(bxdf->type & BSDF_SPECULAR)) {
		f = 0.;
		if (Dot(*wiW, ng) * Dot(woW, ng) > 0)
			// ignore BTDFs
			flags = BxDFType(flags & ~BSDF_TRANSMISSION);
		else
			// ignore BRDFs
			flags = BxDFType(flags & ~BSDF_REFLECTION);
		for (int i = 0; i < nBxDFs; ++i)
			if (bxdfs[i]->MatchesFlags(flags))
				f += bxdfs[i]->f(wo, wi);
	}
	return f;
}
float BSDF::Pdf(const Vector &woW, const Vector &wiW,
		BxDFType flags) const {
	if (nBxDFs == 0.) return 0.;
	Vector wo = WorldToLocal(woW), wi = WorldToLocal(wiW);
	float pdf = 0.f;
	int matchingComps = 0;
	for (int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags)) {
			++matchingComps;
			pdf += bxdfs[i]->Pdf(wo, wi);
		}
	return matchingComps > 0 ? pdf / matchingComps : 0.f;
}
BSDF::BSDF(const DifferentialGeometry &dg,
           const Normal &ngeom, float e)
	: dgShading(dg), eta(e) {
	ng = ngeom;
	nn = dgShading.nn;
	sn = Normalize(dgShading.dpdu);
	tn = Cross(nn, sn);
	nBxDFs = 0;
}
SWCSpectrum BSDF::f(const Vector &woW,
		const Vector &wiW, BxDFType flags) const {
	Vector wi = WorldToLocal(wiW), wo = WorldToLocal(woW);
	if (Dot(wiW, ng) * Dot(woW, ng) > 0)
		// ignore BTDFs
		flags = BxDFType(flags & ~BSDF_TRANSMISSION);
	else
		// ignore BRDFs
		flags = BxDFType(flags & ~BSDF_REFLECTION);
	SWCSpectrum f = 0.;
	for (int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags))
			f += bxdfs[i]->f(wo, wi);
	return f;
}
SWCSpectrum BSDF::rho(BxDFType flags) const {
	SWCSpectrum ret(0.);
	for (int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags))
			ret += bxdfs[i]->rho();
	return ret;
}
SWCSpectrum BSDF::rho(const Vector &wo, BxDFType flags) const {
	SWCSpectrum ret(0.);
	for (int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags))
			ret += bxdfs[i]->rho(wo);
	return ret;
}
//MemoryArena BSDF::arena;
boost::thread_specific_ptr<MemoryArena> BSDF::arena;

