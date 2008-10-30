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

// cooktorrance.cpp*
#include "cooktorrance.h"
#include "spectrum.h"
#include "mc.h"
#include "microfacetdistribution.h"
#include "fresnel.h"

using namespace lux;

CookTorrance::CookTorrance(const SWCSpectrum &kd, u_int nl,
						   const SWCSpectrum *ks,
						   MicrofacetDistribution **dist, Fresnel **fres) : BxDF(BxDFType(BSDF_REFLECTION | BSDF_SPECULAR)) {
  KD = kd;
  KS = ks;
  nLobes = nl;
  distribution = dist;
  fresnel = fres;
}

void CookTorrance::f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const {
  *f += KD * INV_PI;

  float cosThetaO = fabsf(CosTheta(wo));
  float cosThetaI = fabsf(CosTheta(wi));
  Vector wh = Normalize(wi + wo);
  float cosThetaH = Dot(wi, wh);
  float cG = G(wo, wi, wh);

  for (u_int i = 0; i < nLobes; i++) {
	// Add contribution for $i$th Cook-Torrance lobe
	  SWCSpectrum F;
	  fresnel[i]->Evaluate(tspack, cosThetaH, &F);
	*f += KS[i] * distribution[i]->D(wh) * cG * F / (M_PI * cosThetaI * cosThetaO);
  }
}

float CookTorrance::G(const Vector &wo, const Vector &wi, const Vector &wh) const {
  float NdotWh = fabsf(CosTheta(wh));
  float NdotWo = fabsf(CosTheta(wo));
  float NdotWi = fabsf(CosTheta(wi));
  float WOdotWh = AbsDot(wo, wh);
  return min(1.f, min((2.f * NdotWh * NdotWo / WOdotWh), (2.f * NdotWh * NdotWi / WOdotWh)));
}

bool CookTorrance::Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi, float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack, bool reverse) const {
	// Pick a random component
	u_int comp = tspack->rng->uintValue() % (nLobes+1);								// TODO - REFACT - remove and add random value from sample

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
	*pdf = Pdf(tspack, wo, *wi);
	if (*pdf == 0.f) {
		if (pdfBack)
			*pdfBack = 0.f;
		return false;
	}
	if (pdfBack)
		*pdfBack = Pdf(tspack, *wi, wo);

	*f = SWCSpectrum(0.0);
	if (reverse) {
		this->f(tspack, *wi, wo, f);
//		*f *= (wo.z / wi->z);
	}
	else
		this->f(tspack, wo, *wi, f);
	return true;
}

float CookTorrance::Pdf(const TsPack *tspack, const Vector &wo, const Vector &wi) const {
  if (!SameHemisphere(wo, wi))
	return 0.f;

  // Average of all pdf's
  float pdfSum = fabsf(wi.z) * INV_PI;

  for (u_int i = 0; i < nLobes; ++i) {
	pdfSum += distribution[i]->Pdf(wo, wi);
  }
  return pdfSum / (1.f + nLobes);
}

