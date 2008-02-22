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
#include "color.h"
#include "spectrum.h"
#include "mc.h"
#include "sampling.h"
#include "microfacetdistribution.h"
#include "fresnel.h"
#include <stdarg.h>

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