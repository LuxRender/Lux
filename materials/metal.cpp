/*
 * pbrt source code Copyright(c) 1998-2005 Matt Pharr and Greg Humphreys
 *
 * All Rights Reserved.
 * For educational use only; commercial use expressly forbidden.
 * NO WARRANTY, express or implied, for this software.
 * (See file License.txt for complete license) 
 */

// metal.cpp*
#include "metal.h"

Metal::Metal(boost::shared_ptr<Texture<Spectrum> > n, boost::shared_ptr<Texture<Spectrum> > k, boost::shared_ptr<Texture<float> > rough, boost::shared_ptr<Texture<float> > bump) {
  N = n;
  K = k;
  roughness = rough;
  bumpMap = bump;
}

BSDF *Metal::GetBSDF(MemoryArena &arena, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const {
  // Allocate _BSDF_, possibly doing bump-mapping with _bumpMap_
  DifferentialGeometry dgs;
  if (bumpMap)
    Bump(bumpMap, dgGeom, dgShading, &dgs);
  else
    dgs = dgShading;

  BSDF *bsdf = BSDF_ALLOC(arena, BSDF)(dgs, dgGeom.nn);
  Spectrum n = N->Evaluate(dgs);
  Spectrum k = K->Evaluate(dgs);
  float rough = roughness->Evaluate(dgs);

  MicrofacetDistribution *md = BSDF_ALLOC(arena, Blinn)(1.f / rough);

  Fresnel *fresnel = BSDF_ALLOC(arena, FresnelConductor)(n, k);
  bsdf->Add(BSDF_ALLOC(arena, Microfacet)(1., fresnel, md));

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
