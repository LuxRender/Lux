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
#include "boost/lexical_cast.hpp"

using namespace lux;

Metal::Metal(boost::shared_ptr<Texture<SPD*> > n, boost::shared_ptr<Texture<SPD*> > k, boost::shared_ptr<Texture<float> > rough, boost::shared_ptr<Texture<float> > bump) {
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
  SWCSpectrum n(N->Evaluate(dgs));
  SWCSpectrum k(K->Evaluate(dgs));
  float rough = roughness->Evaluate(dgs);

  MicrofacetDistribution *md = BSDF_ALLOC( Blinn)(1.f / rough);

  Fresnel *fresnel = BSDF_ALLOC( FresnelConductor)(n, k);
  bsdf->Add(BSDF_ALLOC( Microfacet)(1., fresnel, md));

  return bsdf;
}

// converts photon energy (in eV) to wavelength (in nm)
float eVtolambda(float eV) {
  // lambda = hc / E, where 
  //  h is planck's constant in eV*s
  //  c is speed of light in m/s
  return (4.135667e-15 * 299792458 * 1e9) / eV;
}

// converts wavelength (in micrometer) to wavelength (in nm)
float umtolambda(float um) {
  return um * 1000;
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

  // NOTE - lordcrc - added warning
  string msg = "Metal '" + name + "' not found, using default (" + metalIORs[0].name + ").";
  luxError(LUX_NOERROR, LUX_WARNING, msg.c_str());

  // default
  *n = Spectrum(metalIORs[0].n);
  *k = Spectrum(metalIORs[0].k);
}

struct IOR {
  IOR() : n(0), k(0) {}
  IOR(float nn, float kk) : n(nn), k(kk) {}
  const IOR operator+(const IOR &other) const {
    return IOR(n + other.n, k + other.k);
  }
  const IOR operator*(float s) const {
    return IOR(n * s, k * s);
  }

  float n;
  float k;
};

struct IORSample {
  float lambda; // in nm
  IOR ior;
};

// computes the linearly interpolated IOR from the data samples
// returns false if the requested wavelength is outside of the data set
bool InterpolatedIOR(float lambda, const vector<struct IORSample> &data, IOR *ior) {

  int prev, cur;

  prev = -1;

  for (size_t i = 0; i < data.size(); i++)
  {
    if (data[i].lambda > lambda) {
      prev = i - 1;
      cur = i;
      break;
    }
  }

  // outside data range
  if (prev < 0)
    return false;

  // linear interpolation
  float t = (lambda - data[prev].lambda) / (data[cur].lambda - data[prev].lambda);
  *ior = data[prev].ior * (1 - t) + data[cur].ior * t;

  return true;
}

bool ReadSOPRAData(std::ifstream &fs, vector<struct IORSample> &data) {

  string line;

  if (!getline(fs, line).good())
    return false;

  boost::smatch m;

  // read initial line, containing metadata
  boost::regex header_expr("(\\d+)\\s+(\\d*\\.?\\d+)\\s+(\\d*\\.?\\d+)\\s+(\\d+)");

  if (!boost::regex_search(line, m, header_expr))
    return false;

  // used to convert file units to wavelength in nm
  float (*tolambda)(float) = NULL;

  float lambda_first, lambda_last;

  if (m[1] == "1") {
    // lambda in eV
    // low eV -> high lambda
    lambda_last = boost::lexical_cast<float>(m[2]);
    lambda_first = boost::lexical_cast<float>(m[3]);
    tolambda = &eVtolambda;
  } else if (m[1] == "2") {
    // lambda in um
    lambda_first = boost::lexical_cast<float>(m[2]);
    lambda_last = boost::lexical_cast<float>(m[3]);
    tolambda = &umtolambda;
  } else
    return false;

  // number of lines of nk data
  int count = boost::lexical_cast<int>(m[4]);  
  data.resize(count);

  // read nk data
  boost::regex sample_expr("(\\d*\\.?\\d+)\\s+(\\d*\\.?\\d+)");

  // we want lambda to go from low to high
  // so reverse direction
  for (int i = count-1; i >= 0; i--) {
    
    if (!getline(fs, line).good())
      return false;

    if (!boost::regex_search(line, m, sample_expr))
      return false;
    
    struct IORSample sample;
    
    // linearly interpolate units in file
    // then convert to wavelength in nm
    sample.lambda = tolambda(lambda_first + (lambda_last - lambda_first) * i / (float)count);
    string s;
    sample.ior.n = boost::lexical_cast<float>(m[1]);
    sample.ior.k = boost::lexical_cast<float>(m[2]);

    data[i] = sample;
  }

  return true;
}

bool ReadLuxpopData(std::ifstream &fs, vector<struct IORSample> &data) {

  string line;

  boost::smatch m;

  // read nk data
  boost::regex sample_expr("(\\d*\\.?\\d+|\\d+\\.)\\s+(\\d*\\.?\\d+|\\d+\\.?)\\s+(\\d*\\.?\\d+|\\d+\\.)");

  // we want lambda to go from low to high
  while (getline(fs, line).good()) {

    // skip comments
    if (line.length() > 0 && line[0] == ';')
      continue;

    if (!boost::regex_search(line, m, sample_expr))
      return false;
    
    struct IORSample sample;

    // wavelength in data file is in Angstroms, we want nm
    sample.lambda = boost::lexical_cast<float>(m[1]) * 0.1;
    sample.ior.n = boost::lexical_cast<float>(m[2]);
    sample.ior.k = boost::lexical_cast<float>(m[3]);

    data.push_back(sample);
  }

  return fs.eof();
}

// returns < 0 if file not found, 0 if error, 1 if ok
int IORFromFile(const string filename, SPD* &n, SPD* &k) {

  std::ifstream f;

  f.open(filename.c_str());

  if (!f.is_open())
    return -1;

  vector<struct IORSample>data;

  // read file, by trying each parser in turn
  if (!ReadSOPRAData(f, data))
    if (!ReadLuxpopData(f, data))
      return 0;

  f.close();

  // copy data into arrays for SPD creation
  // can probably be avoided with pointer voodoo
  int sn = data.size();
  float *wl = new float[sn];
  float *an = new float[sn];
  float *ak = new float[sn];

  for (int i = 0; i < sn; i++) {
    wl[i] = data[i].lambda;
    an[i] = data[i].ior.n;
    ak[i] = data[i].ior.k;
  }

  n = new IrregularSPD(wl, an, sn);
  k = new IrregularSPD(wl, ak, sn);

  delete[] wl;
  delete[] an;
  delete[] ak;

  return 1;
}

Material *Metal::CreateMaterial(const Transform &xform, const TextureParams &tp) {

  string metalname = tp.FindString("name");

  boost::shared_ptr<Texture<SPD*> > n;
  boost::shared_ptr<Texture<SPD*> > k;

  if (metalname.length() < 1) {
    // we got no name, so try to read IOR directly
    //n = tp.GetSpectrumTexture("n", Spectrum(metalIORs[0].n));
    //k = tp.GetSpectrumTexture("k", Spectrum(metalIORs[0].k));
    float w[1] = { 0.f };
    float an[1] = { 1.f };
    float ak[1] = { 0.f };

    SPD* s_n = new IrregularSPD(w, an, 1);
    SPD* s_k = new IrregularSPD(w, ak, 1);

    boost::shared_ptr<Texture<SPD*> > n_n (new ConstantTexture<SPD*>(s_n));
    boost::shared_ptr<Texture<SPD*> > n_k (new ConstantTexture<SPD*>(s_k));
    n = n_n;
    k = n_k;
  }
  else {
    // we got a name, try to look it up
    SPD* s_n;
    SPD* s_k;

    // NOTE - lordcrc - added loading nk data from file
	// try name as a filename first, else use internal db
    int result = IORFromFile(metalname, s_n, s_k);
    switch (result) {
      case 0: {
        string msg = "Error loading data file '" + metalname + "'. Using default (" + metalIORs[0].name + ").";
        luxError(LUX_NOERROR, LUX_WARNING, msg.c_str());
        metalname = metalIORs[0].name;
      }
      case -1:
        luxError(LUX_UNIMPLEMENT, LUX_SEVERE, "Only .nk files supported for now.");        
        //IORFromName(metalname, &s_n, &s_k);
        break;
      case 1:
        break;
    }

    boost::shared_ptr<Texture<SPD*> > n_n (new ConstantTexture<SPD*>(s_n));
    boost::shared_ptr<Texture<SPD*> > n_k (new ConstantTexture<SPD*>(s_k));
    n = n_n;
    k = n_k;
  }

  boost::shared_ptr<Texture<float> > roughness = tp.GetFloatTexture("roughness", .1f);
  boost::shared_ptr<Texture<float> > bumpMap = tp.GetFloatTexture("bumpmap", 0.f);

  return new Metal(n, k, roughness, bumpMap);
}
