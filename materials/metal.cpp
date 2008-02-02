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

const int RGBstart = 390;
const int RGBend = 730;
const int RGBinc = 5;
const int nRGB = 69;

const float CIE_R[nRGB] = {  
  1.83970e-003f,  4.61530e-003f,  9.62640e-003f,
  1.89790e-002f,  3.08030e-002f,  4.24590e-002f,
  5.16620e-002f,  5.28370e-002f,  4.42870e-002f,
  3.22200e-002f,  1.47630e-002f,  -2.33920e-003f,
  -2.91300e-002f,  -6.06770e-002f,  -9.62240e-002f,
  -1.37590e-001f,  -1.74860e-001f,  -2.12600e-001f,
  -2.37800e-001f,  -2.56740e-001f,  -2.77270e-001f,
  -2.91250e-001f,  -2.95000e-001f,  -2.97060e-001f,
  -2.67590e-001f,  -2.17250e-001f,  -1.47680e-001f,
  -3.51840e-002f,  1.06140e-001f,  2.59810e-001f,
  4.19760e-001f,  5.92590e-001f,  7.90040e-001f,
  1.00780e+000f,  1.22830e+000f,  1.47270e+000f,
  1.74760e+000f,  2.02140e+000f,  2.27240e+000f,
  2.48960e+000f,  2.67250e+000f,  2.80930e+000f,
  2.87170e+000f,  2.85250e+000f,  2.76010e+000f,
  2.59890e+000f,  2.37430e+000f,  2.10540e+000f,
  1.81450e+000f,  1.52470e+000f,  1.25430e+000f,
  1.00760e+000f,  7.86420e-001f,  5.96590e-001f,
  4.43200e-001f,  3.24100e-001f,  2.34550e-001f,
  1.68840e-001f,  1.20860e-001f,  8.58110e-002f,
  6.02600e-002f,  4.14800e-002f,  2.81140e-002f,
  1.91170e-002f,  1.33050e-002f,  9.40920e-003f,
  6.51770e-003f,  4.53770e-003f,  3.17420e-003f,
};

const float CIE_G[nRGB] = {
  -4.53930e-004f, -1.04640e-003f, -2.16890e-003f,
  -4.43040e-003f, -7.20480e-003f, -1.25790e-002f,
  -1.66510e-002f, -2.12400e-002f, -1.99360e-002f,
  -1.60970e-002f, -7.34570e-003f, 1.36900e-003f,
  1.96100e-002f, 4.34640e-002f, 7.09540e-002f,
  1.10220e-001f, 1.50880e-001f, 1.97940e-001f,
  2.40420e-001f, 2.79930e-001f, 3.33530e-001f,
  4.05210e-001f, 4.90600e-001f, 5.96730e-001f,
  7.01840e-001f, 8.08520e-001f, 9.10760e-001f,
  9.84820e-001f, 1.03390e+000f, 1.05380e+000f,
  1.05120e+000f, 1.04980e+000f, 1.03680e+000f,
  9.98260e-001f, 9.37830e-001f, 8.80390e-001f,
  8.28350e-001f, 7.46860e-001f, 6.49300e-001f,
  5.63170e-001f, 4.76750e-001f, 3.84840e-001f,
  3.00690e-001f, 2.28530e-001f, 1.65750e-001f,
  1.13730e-001f, 7.46820e-002f, 4.65040e-002f,
  2.63330e-002f, 1.27240e-002f, 4.50330e-003f,
  9.66110e-005f, -1.96450e-003f, -2.63270e-003f,
  -2.62620e-003f, -2.30270e-003f, -1.87000e-003f,
  -1.44240e-003f, -1.07550e-003f, -7.90040e-004f,
  -5.67650e-004f, -3.92740e-004f, -2.62310e-004f,
  -1.75120e-004f, -1.21400e-004f, -8.57600e-005f,
  -5.76770e-005f, -3.90030e-005f, -2.65110e-005f,
};

const float CIE_B[nRGB] = {  
  1.21520e-002f, 3.11100e-002f, 6.23710e-002f,
  1.31610e-001f, 2.27500e-001f, 3.58970e-001f,
  5.23960e-001f, 6.85860e-001f, 7.96040e-001f,
  8.94590e-001f, 9.63950e-001f, 9.98140e-001f,
  9.18750e-001f, 8.24870e-001f, 7.85540e-001f,
  6.67230e-001f, 6.10980e-001f, 4.88290e-001f,
  3.61950e-001f, 2.66340e-001f, 1.95930e-001f,
  1.47300e-001f, 1.07490e-001f, 7.67140e-002f,
  5.02480e-002f, 2.87810e-002f, 1.33090e-002f,
  2.11700e-003f, -4.15740e-003f, -8.30320e-003f,
  -1.21910e-002f, -1.40390e-002f, -1.46810e-002f,
  -1.49470e-002f, -1.46130e-002f, -1.37820e-002f,
  -1.26500e-002f, -1.13560e-002f, -9.93170e-003f,
  -8.41480e-003f, -7.02100e-003f, -5.74370e-003f,
  -4.27430e-003f, -2.91320e-003f, -2.26930e-003f,
  -1.99660e-003f, -1.50690e-003f, -9.38220e-004f,
  -5.53160e-004f, -3.16680e-004f, -1.43190e-004f,
  -4.08310e-006f, 1.10810e-004f, 1.91750e-004f,
  2.26560e-004f, 2.15200e-004f, 1.63610e-004f,
  9.71640e-005f, 5.10330e-005f, 3.52710e-005f,
  3.12110e-005f, 2.45080e-005f, 1.65210e-005f,
  1.11240e-005f, 8.69650e-006f, 7.43510e-006f,
  6.10570e-006f, 5.02770e-006f, 4.12510e-006f,
};

// returns < 0 if file not found, 0 if error, 1 if ok
int IORFromFile(const string filename, Spectrum *n, Spectrum *k) {

  std::ifstream f;

  f.open(filename.c_str());

  if (!f.is_open())
    return -1;

  vector<struct IORSample>data;

  // read file, by trying each parser in turn
  if (!ReadSOPRAData(f, data))
    if (!ReadLuxpopData(f, data))
      return 0;

  // integrate nk data to create a spectrum
  // rgb color matching functions give better results, it seems
  float wr, wg, wb;
  wr = wg = wb = 0;
  
  IOR r, g, b;

  int usedsamples = 0;

  for (int i = 0; i < nRGB; i++) {

    float lambda = RGBstart + RGBinc * i;
    IOR ior;
    
    if (InterpolatedIOR(lambda, data, &ior)) {

      r = r + ior * CIE_R[i];
      wr += CIE_R[i];

      g = g + ior * CIE_G[i];
      wg += CIE_G[i];

      b = b + ior * CIE_B[i];
      wb += CIE_B[i];

      usedsamples++;
    }
  }

  if (usedsamples == 0) {
    string msg = "Metal IOR data file " + filename + " might not contain data in visible range. Using default (" + metalIORs[0].name + ").";

    luxError(LUX_NOERROR, LUX_WARNING, msg.c_str());

    *n = Spectrum(metalIORs[0].n);
    *k = Spectrum(metalIORs[0].k);
  }
  else {

    if (usedsamples < nRGB) {
      string msg = "Metal IOR data file " + filename + " doesn't contain data for the entire visible range. This may lead to strange results.";
      luxError(LUX_NOERROR, LUX_WARNING, msg.c_str());
    }

    r = r * (1 / wr);
    g = g * (1 / wg);
    b = b * (1 / wb);

    // convert RGB data to XYZ
    IOR x = r * 0.41245329f  + g * 0.35757986f + b * 0.18042259f;
    IOR y = r * 0.21267128f  + g * 0.71515971f + b * 0.072168820f;
    IOR z = r * 0.019333841f + g * 0.11919361f + b * 0.95022678f;

    // create spectrum from XYZ data
    *n = FromXYZ(x.n, y.n, z.n);
    *k = FromXYZ(x.k, y.k, z.k);
  }

  f.close();

  return 1;
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

    // NOTE - lordcrc - added loading nk data from file
	// try name as a filename first, else use internal db
    int result = IORFromFile(metalname, &s_n, &s_k);
    switch (result) {
      case 0: {
        string msg = "Error loading data file '" + metalname + "'. Using default (" + metalIORs[0].name + ").";
        luxError(LUX_NOERROR, LUX_WARNING, msg.c_str());
        metalname = metalIORs[0].name;
      }
      case -1:
        IORFromName(metalname, &s_n, &s_k);
        break;
      case 1:
        break;
    }

    boost::shared_ptr<Texture<Spectrum> > n_n (new ConstantTexture<Spectrum>(s_n));
    boost::shared_ptr<Texture<Spectrum> > n_k (new ConstantTexture<Spectrum>(s_k));
    n = n_n;
    k = n_k;
  }

  boost::shared_ptr<Texture<float> > roughness = tp.GetFloatTexture("roughness", .1f);
  boost::shared_ptr<Texture<float> > bumpMap = tp.GetFloatTexture("bumpmap", 0.f);

  return new Metal(n, k, roughness, bumpMap);
}
