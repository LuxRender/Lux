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

// carpaint.cpp*

// Simulate car paint - adopted from Gunther et al, "Effcient Acquisition and Realistic Rendering of Car Paint", 2005

#include "carpaint.h"

CarPaint::CarPaint(boost::shared_ptr<Texture<Spectrum> > kd,
                   boost::shared_ptr<Texture<Spectrum> > ks1, boost::shared_ptr<Texture<Spectrum> > ks2, boost::shared_ptr<Texture<Spectrum> > ks3,
                   boost::shared_ptr<Texture<float> > r1, boost::shared_ptr<Texture<float> > r2, boost::shared_ptr<Texture<float> > r3,
                   boost::shared_ptr<Texture<float> > m1, boost::shared_ptr<Texture<float> > m2, boost::shared_ptr<Texture<float> > m3,
                   boost::shared_ptr<Texture<float> > bump) {
  Kd = kd;
  Ks1 = ks1;
  Ks2 = ks2;
  Ks3 = ks3;
  R1 = r1;
  R2 = r2;
  R3 = r3;
  M1 = m1;
  M2 = m2;
  M3 = m3;
  bumpMap = bump;
}

// CarPaint Method Definitions
BSDF *CarPaint::GetBSDF(MemoryArena &arena, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const {

  // Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
  DifferentialGeometry dgs;

  if (bumpMap)
    Bump(bumpMap, dgGeom, dgShading, &dgs);
  else
    dgs = dgShading;

  BSDF *bsdf = BSDF_ALLOC(arena, BSDF)(dgs, dgGeom.nn);

  Spectrum kd = Kd->Evaluate(dgs).Clamp();
  Spectrum ks1 = Ks1->Evaluate(dgs).Clamp();
  Spectrum ks2 = Ks2->Evaluate(dgs).Clamp();
  Spectrum ks3 = Ks3->Evaluate(dgs).Clamp();

  float r1 = R1->Evaluate(dgs);
  float r2 = R2->Evaluate(dgs);
  float r3 = R3->Evaluate(dgs);

  float m1 = M1->Evaluate(dgs);
  float m2 = M2->Evaluate(dgs);
  float m3 = M3->Evaluate(dgs);

  MicrofacetDistribution *md1 = BSDF_ALLOC(arena, Blinn)((2.0 * M_PI / (m1 * m1)) - 1.0);
  MicrofacetDistribution *md2 = BSDF_ALLOC(arena, Blinn)((2.0 * M_PI / (m2 * m2)) - 1.0);
  MicrofacetDistribution *md3 = BSDF_ALLOC(arena, Blinn)((2.0 * M_PI / (m3 * m3)) - 1.0);

  // The Slick approximation is much faster and visually almost the same
  Fresnel *fr1 = BSDF_ALLOC(arena, FresnelSlick)(r1);
  Fresnel *fr2 = BSDF_ALLOC(arena, FresnelSlick)(r2);
  Fresnel *fr3 = BSDF_ALLOC(arena, FresnelSlick)(r3);

  /*Spectrum k = 0.;
  Fresnel *fr1 = BSDF_ALLOC(arena, FresnelConductor)(FresnelApproxEta(r1), k);
  Fresnel *fr2 = BSDF_ALLOC(arena, FresnelConductor)(FresnelApproxEta(r2), k);
  Fresnel *fr3 = BSDF_ALLOC(arena, FresnelConductor)(FresnelApproxEta(r3), k);*/

  // The Carpaint BRDF is really a Multi-lobe Cook-Torrance model with a Lambertian base

  Spectrum *lobe_ks = (Spectrum *)arena.Alloc(3 * sizeof(Spectrum));
  lobe_ks[0] = ks1;
  lobe_ks[1] = ks2;
  lobe_ks[2] = ks3;

  MicrofacetDistribution **lobe_dist = (MicrofacetDistribution **)arena.Alloc(3 * sizeof(MicrofacetDistribution *));
  lobe_dist[0] = md1;
  lobe_dist[1] = md2;
  lobe_dist[2] = md3;

  Fresnel **lobe_fres = (Fresnel **)arena.Alloc(3 * sizeof(Fresnel *));
  lobe_fres[0] = fr1;
  lobe_fres[1] = fr2;
  lobe_fres[2] = fr3;

  bsdf->Add(BSDF_ALLOC(arena, CookTorrance)(kd, 3, lobe_ks, lobe_dist, lobe_fres));

  return bsdf;
}

Material* CarPaint::CreateMaterial(const Transform &xform, const TextureParams &mp) {
  // The default values are for "Polaris Silver"
  float def_kd[3] = {0.055f, 0.063f, 0.071f};
  float def_ks1[3] = {0.065f, 0.082f, 0.088f};
  float def_ks2[3] = {0.11f, 0.11f, 0.13f};
  float def_ks3[3] = {0.008f, 0.013f, 0.015f};
  float def_r[3] = {1.0f, 0.92f, 0.9f};
  float def_m[3] = {0.38f, 0.17f, 0.013f};

  boost::shared_ptr<Texture<Spectrum> > Kd = mp.GetSpectrumTexture("Kd", Spectrum(def_kd));

  boost::shared_ptr<Texture<Spectrum> > Ks1 = mp.GetSpectrumTexture("Ks1", Spectrum(def_ks1));
  boost::shared_ptr<Texture<Spectrum> > Ks2 = mp.GetSpectrumTexture("Ks2", Spectrum(def_ks2));
  boost::shared_ptr<Texture<Spectrum> > Ks3 = mp.GetSpectrumTexture("Ks3", Spectrum(def_ks3));

  boost::shared_ptr<Texture<float> > R1 = mp.GetFloatTexture("R1", def_r[0]);
  boost::shared_ptr<Texture<float> > R2 = mp.GetFloatTexture("R2", def_r[1]);
  boost::shared_ptr<Texture<float> > R3 = mp.GetFloatTexture("R3", def_r[2]);

  boost::shared_ptr<Texture<float> > M1 = mp.GetFloatTexture("M1", def_m[0]);
  boost::shared_ptr<Texture<float> > M2 = mp.GetFloatTexture("M2", def_m[1]);
  boost::shared_ptr<Texture<float> > M3 = mp.GetFloatTexture("M3", def_m[2]);

  boost::shared_ptr<Texture<float> > bumpMap = mp.GetFloatTexture("bumpmap", 0.f);

  return new CarPaint(Kd, Ks1, Ks2, Ks3, R1, R2, R3, M1, M2, M3, bumpMap);
}
