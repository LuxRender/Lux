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

void IORFromName(const string name, SPD* &n, SPD* &k) {

  float *wl;
  float *sn;
  float *sk;
  int ns = 0;

  float sopra_wl[] = {298.7570554, 302.4004341, 306.1337728, 309.960445, 313.8839949, 317.9081487, 
    322.036826, 326.2741526, 330.6244747, 335.092373, 339.6826795, 344.4004944, 349.2512056, 
    354.2405086, 359.374429, 364.6593471, 370.1020239, 375.7096303, 381.4897785, 387.4505563, 
    393.6005651, 399.9489613, 406.5055016, 413.2805933, 420.2853492, 427.5316483, 435.0322035, 
    442.8006357, 450.8515564, 459.2006593, 467.8648226, 476.8622231, 486.2124627, 495.936712, 
    506.0578694, 516.6007417, 527.5922468, 539.0616435, 551.0407911, 563.5644455, 576.6705953, 
    590.4008476, 604.8008683, 619.92089, 635.8162974, 652.5483053, 670.1847459, 688.8009889, 
    708.4810171, 729.3186941, 751.4192606, 774.9011125, 799.8979226, 826.5611867, 855.0632966, 
    885.6012714 };

  if (name == "amorphous carbon") {
    float _wl[] = { 247.96, 309.95, 326.263, 344.389, 364.647, 387.438, 
      413.267, 442.786, 476.846, 516.583, 563.545, 619.9, 688.778, 774.875, 885.571 };
    float _sn[] = {1.73, 1.84, 1.9, 1.94, 2, 2.06, 2.11, 2.17, 2.24, 2.3, 2.38, 2.43, 2.43, 2.33, 2.24};
    float _sk[] = {0.712, 0.808, 0.91, 0.92, 0.92, 0.91, 0.9, 0.89, 0.88, 0.87, 0.82, 0.75, 0.7, 0.71};
    
    ns = 15;
    wl = _wl;
    sn = _sn;
    sk = _sk;
  } else if (name == "silver") {
    float _sn[] = { 1.519, 1.496, 1.4325, 1.323, 1.142062, 0.932, 0.719062, 0.526, 0.388125, 0.294, 0.253313, 
      0.238, 0.221438, 0.209, 0.194813, 0.186, 0.192063, 0.2, 0.198063, 0.192, 0.182, 0.173, 0.172625, 
      0.173, 0.166688, 0.16, 0.1585, 0.157, 0.151063, 0.144, 0.137313, 0.132, 0.13025, 0.13, 0.129938, 
      0.13, 0.130063, 0.129, 0.124375, 0.12, 0.119313, 0.121, 0.1255, 0.131, 0.136125, 0.14, 0.140063, 
      0.14, 0.144313, 0.148, 0.145875, 0.143, 0.142563, 0.145, 0.151938, 0.163 };
    float _sk[] = { 1.08, 0.882, 0.761063, 0.647, 0.550875, 0.504, 0.554375, 0.663, 0.818563, 0.986, 1.120687, 
      1.24, 1.34525, 1.44, 1.53375, 1.61, 1.641875, 1.67, 1.735, 1.81, 1.87875, 1.95, 2.029375, 2.11, 
      2.18625, 2.26, 2.329375, 2.4, 2.47875, 2.56, 2.64, 2.72, 2.798125, 2.88, 2.97375, 3.07, 3.159375, 
      3.25, 3.348125, 3.45, 3.55375, 3.66, 3.76625, 3.88, 4.010625, 4.15, 4.293125, 4.44, 4.58625, 4.74, 
      4.908125, 5.09, 5.28875, 5.5, 5.720624, 5.95 };
    
    ns = 56;
    wl = sopra_wl;
    sn = _sn;
    sk = _sk;
  } else if (name == "gold") {
    float _sn[] = { 1.795, 1.812, 1.822625, 1.83, 1.837125, 1.84, 1.83425, 1.824, 1.812, 1.798, 1.782, 
      1.766, 1.7525, 1.74, 1.727625, 1.716, 1.705875, 1.696, 1.68475, 1.674, 1.666, 1.658, 1.64725, 
      1.636, 1.628, 1.616, 1.59625, 1.562, 1.502125, 1.426, 1.345875, 1.242, 1.08675, 0.916, 0.7545, 
      0.608, 0.49175, 0.402, 0.3455, 0.306, 0.267625, 0.236, 0.212375, 0.194, 0.17775, 0.166, 0.161, 
      0.16, 0.160875, 0.164, 0.1695, 0.176, 0.181375, 0.188, 0.198125, 0.21 };
    float _sk[] = { 1.920375, 1.92, 1.918875, 1.916, 1.911375, 1.904, 1.891375, 1.878, 1.86825, 1.86, 
      1.85175, 1.846, 1.84525, 1.848, 1.852375, 1.862, 1.883, 1.906, 1.9225, 1.936, 1.94775, 1.956, 
      1.959375, 1.958, 1.951375, 1.94, 1.9245, 1.904, 1.875875, 1.846, 1.814625, 1.796, 1.797375, 
      1.84, 1.9565, 2.12, 2.32625, 2.54, 2.730625, 2.88, 2.940625, 2.97, 3.015, 3.06, 3.07, 3.15, 
      3.445812, 3.8, 4.087687, 4.357, 4.610188, 4.86, 5.125813, 5.39, 5.63125, 5.88 };

    ns = 56;
    wl = sopra_wl;
    sn = _sn;
    sk = _sk;
  } else if (name == "copper") {
    float _sn[] = {1.400313, 1.38, 1.358438, 1.34, 1.329063, 1.325, 1.3325, 1.34, 1.334375, 1.325, 
      1.317812, 1.31, 1.300313, 1.29, 1.281563, 1.27, 1.249062, 1.225, 1.2, 1.18, 1.174375, 1.175, 
      1.1775, 1.18, 1.178125, 1.175, 1.172812, 1.17, 1.165312, 1.16, 1.155312, 1.15, 1.142812, 1.135, 
      1.131562, 1.12, 1.092437, 1.04, 0.950375, 0.826, 0.645875, 0.468, 0.35125, 0.272, 0.230813, 0.214, 
      0.20925, 0.213, 0.21625, 0.223, 0.2365, 0.25, 0.254188, 0.26, 0.28, 0.3 };
    float _sk[] = {1.662125, 1.687, 1.703313, 1.72, 1.744563, 1.77, 1.791625, 1.81, 1.822125, 1.834, 
      1.85175, 1.872, 1.89425, 1.916, 1.931688, 1.95, 1.972438, 2.015, 2.121562, 2.21, 2.177188, 2.13, 
      2.160063, 2.21, 2.249938, 2.289, 2.326, 2.362, 2.397625, 2.433, 2.469187, 2.504, 2.535875, 2.564, 
      2.589625, 2.605, 2.595562, 2.583, 2.5765, 2.599, 2.678062, 2.809, 3.01075, 3.24, 3.458187, 3.67, 
      3.863125, 4.05, 4.239563, 4.43, 4.619563, 4.817, 5.034125, 5.26, 5.485625, 5.717 };

    ns = 56;
    wl = sopra_wl;
    sn = _sn;
    sk = _sk;
  } else {
    if (name != "aluminium") {
      // NOTE - lordcrc - added warning
      string msg = "Metal '" + name + "' not found, using default (" + DEFAULT_METAL + ").";
      luxError(LUX_NOERROR, LUX_WARNING, msg.c_str());
    }

    float _sn[] = { 0.273375, 0.28, 0.286813, 0.294, 0.301875, 0.31, 0.317875, 0.326, 0.33475, 0.344, 
      0.353813, 0.364, 0.374375, 0.385, 0.39575, 0.407, 0.419125, 0.432, 0.445688, 0.46, 0.474688, 0.49, 
      0.506188, 0.523, 0.540063, 0.558, 0.577313, 0.598, 0.620313, 0.644, 0.668625, 0.695, 0.72375, 0.755, 
      0.789, 0.826, 0.867, 0.912, 0.963, 1.02, 1.08, 1.15, 1.22, 1.3, 1.39, 1.49, 1.6, 1.74, 1.91, 2.14, 
      2.41, 2.63, 2.8, 2.74, 2.58, 2.24 };

    float _sk[] = { 3.59375, 3.64, 3.689375, 3.74, 3.789375, 3.84, 3.894375, 3.95, 4.005, 4.06, 4.11375, 
      4.17, 4.23375, 4.3, 4.365, 4.43, 4.49375, 4.56, 4.63375, 4.71, 4.784375, 4.86, 4.938125, 5.02, 
      5.10875, 5.2, 5.29, 5.38, 5.48, 5.58, 5.69, 5.8, 5.915, 6.03, 6.15, 6.28, 6.42, 6.55, 6.7, 6.85, 
      7, 7.15, 7.31, 7.48, 7.65, 7.82, 8.01, 8.21, 8.39, 8.57, 8.62, 8.6, 8.45, 8.31, 8.21, 8.21 };

    ns = 56;
    wl = sopra_wl;
    sn = _sn;
    sk = _sk;
  }
  
  n = new IrregularSPD(wl, sn, ns);
  k = new IrregularSPD(wl, sk, ns);
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

  if (metalname == "")
    metalname = DEFAULT_METAL;

  SPD* s_n;
  SPD* s_k;

  // NOTE - lordcrc - added loading nk data from file
// try name as a filename first, else use internal db
  int result = IORFromFile(metalname, s_n, s_k);
  switch (result) {
    case 0: {
      string msg = "Error loading data file '" + metalname + "'. Using default (" + DEFAULT_METAL + ").";
      luxError(LUX_NOERROR, LUX_WARNING, msg.c_str());
      metalname = DEFAULT_METAL;
    }
    case -1:
      IORFromName(metalname, s_n, s_k);
      break;
    case 1:
      break;
  }

  boost::shared_ptr<Texture<SPD*> > n (new ConstantTexture<SPD*>(s_n));
  boost::shared_ptr<Texture<SPD*> > k (new ConstantTexture<SPD*>(s_k));

  boost::shared_ptr<Texture<float> > roughness = tp.GetFloatTexture("roughness", .1f);
  boost::shared_ptr<Texture<float> > bumpMap = tp.GetFloatTexture("bumpmap", 0.f);

  return new Metal(n, k, roughness, bumpMap);
}
