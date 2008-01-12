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

// metal.* - adapted to Lux from code by Asbjørn Heid
#include "metal.h"

using namespace lux;

Metal::Metal(boost::shared_ptr<Texture<Spectrum> > n, boost::shared_ptr<Texture<Spectrum> > k, boost::shared_ptr<Texture<float> > rough, boost::shared_ptr<Texture<float> > bump) {
  N = n;
  K = k;
  roughness = rough;
  bumpMap = bump;
}

BSDF *Metal::GetBSDF(const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const {
  // Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
  DifferentialGeometry dgs;
  if (bumpMap)
    Bump(bumpMap, dgGeom, dgShading, &dgs);
  else
    dgs = dgShading;

  BSDF *bsdf = BSDF_ALLOC( BSDF)(dgs, dgGeom.nn);
  Spectrum n = N->Evaluate(dgs);
  Spectrum k = K->Evaluate(dgs);
  float rough = roughness->Evaluate(dgs);

  MicrofacetDistribution *md = BSDF_ALLOC( Blinn)(1.f / rough);

  Fresnel *fresnel = BSDF_ALLOC( FresnelConductor)(n, k);
  bsdf->Add(BSDF_ALLOC( Microfacet)(1., fresnel, md));

  return bsdf;
}


void IORFromName(const string name, Spectrum *n, Spectrum *k) {
  int numMetals = sizeof(metalIORs) / sizeof(MetalIOR);

  for (int i = 0; i < numMetals; i++) {
    if (name.compare(metalIORs[i].name) == 0) {
      *n = Spectrum(metalIORs[i].n);
      *k = Spectrum(metalIORs[i].k);
      return;
    }
  }

  // default
  *n = Spectrum(metalIORs[0].n);
  *k = Spectrum(metalIORs[0].k);
}

Material *Metal::CreateMaterial(const Transform &xform, const TextureParams &tp) {

  string metalname = tp.FindString("name");

  boost::shared_ptr<Texture<Spectrum> > n;
  boost::shared_ptr<Texture<Spectrum> > k;

  if (metalname.length() < 1) {
    // we got no name, so try to read IOR directly
    n = tp.GetSpectrumTexture("n", Spectrum(metalIORs[0].n));
    k = tp.GetSpectrumTexture("k", Spectrum(metalIORs[0].k));
  }
  else {
    // we got a name, try to look it up
    Spectrum s_n;
    Spectrum s_k;

    IORFromName(metalname, &s_n, &s_k);
    boost::shared_ptr<Texture<Spectrum> > n_n (new ConstantTexture<Spectrum>(s_n));
    boost::shared_ptr<Texture<Spectrum> > n_k (new ConstantTexture<Spectrum>(s_k));
    n = n_n;
    k = n_k;
  }

  boost::shared_ptr<Texture<float> > roughness = tp.GetFloatTexture("roughness", .1f);
  boost::shared_ptr<Texture<float> > bumpMap = tp.GetFloatTexture("bumpmap", 0.f);

  return new Metal(n, k, roughness, bumpMap);
}
