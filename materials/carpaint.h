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

// carpaint.cpp*
#include "lux.h"
#include "material.h"

namespace lux
{

// CarPaint Class Declarations

class CarPaint : public Material {
public:
  // CarPaint Public Methods
  CarPaint(boost::shared_ptr<Texture<SWCSpectrum> > kd,
           boost::shared_ptr<Texture<SWCSpectrum> > ka,
           boost::shared_ptr<Texture<float> > d,
           boost::shared_ptr<Texture<SWCSpectrum> > ks1, boost::shared_ptr<Texture<SWCSpectrum> > ks2, boost::shared_ptr<Texture<SWCSpectrum> > ks3,
           boost::shared_ptr<Texture<float> > r1, boost::shared_ptr<Texture<float> > r2, boost::shared_ptr<Texture<float> > r3,
           boost::shared_ptr<Texture<float> > m1, boost::shared_ptr<Texture<float> > m2, boost::shared_ptr<Texture<float> > m3,
           boost::shared_ptr<Texture<float> > bump, CompositingParams cp);
  virtual ~CarPaint() { }

  virtual BSDF *GetBSDF(const TsPack *tspack, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading, float u) const;

  static Material * CreateMaterial(const Transform &xform, const TextureParams &mp);
private:
  // CarPaint Private Data
  boost::shared_ptr<Texture<SWCSpectrum> > Kd, Ka, Ks1, Ks2, Ks3;
  boost::shared_ptr<Texture<float> > depth, R1, R2, R3, M1, M2, M3;
  boost::shared_ptr<Texture<float> > bumpMap;
};

struct CarPaintData {
  string name;
  float kd[COLOR_SAMPLES];
  float ks1[COLOR_SAMPLES];
  float ks2[COLOR_SAMPLES];
  float ks3[COLOR_SAMPLES];
  float r1, r2, r3;
  float m1, m2, m3;
};

static CarPaintData carpaintdata[] = {
  {"ford f8",
   {0.0012f, 0.0015f, 0.0018f},
   {0.0049f, 0.0076f, 0.0120f},
   {0.0100f, 0.0130f, 0.0180f},
   {0.0070f, 0.0065f, 0.0077f},
    0.1500f, 0.0870f, 0.9000f,
    0.3200f, 0.1100f, 0.0130f},
  {"polaris silber",
   {0.0550f, 0.0630f, 0.0710f},
   {0.0650f, 0.0820f, 0.0880f},
   {0.1100f, 0.1100f, 0.1300f},
   {0.0080f, 0.0130f, 0.0150f},
    1.0000f, 0.9200f, 0.9000f,
    0.3800f, 0.1700f, 0.0130f},
  {"opel titan",
   {0.0110f, 0.0130f, 0.0150f},
   {0.0570f, 0.0660f, 0.0780f},
   {0.1100f, 0.1200f, 0.1300f},
   {0.0095f, 0.0140f, 0.0160f},
    0.8500f, 0.8600f, 0.9000f,
    0.3800f, 0.1700f, 0.0140f},
  {"bmw339",
   {0.0120f, 0.0150f, 0.0160f},
   {0.0620f, 0.0760f, 0.0800f},
   {0.1100f, 0.1200f, 0.1200f},
   {0.0083f, 0.0150f, 0.0160f},
    0.9200f, 0.8700f, 0.9000f,
    0.3900f, 0.1700f, 0.0130f},
  {"2k acrylack",
   {0.4200f, 0.3200f, 0.1000f},
   {0.0000f, 0.0000f, 0.0000f},
   {0.0280f, 0.0260f, 0.0060f},
   {0.0170f, 0.0075f, 0.0041f},
    1.0000f, 0.9000f, 0.1700f,
    0.8800f, 0.8000f, 0.0150f},
  {"white",
   {0.6100f, 0.6300f, 0.5500f},
   {2.6e-06f, 0.00031f, 3.1e-08f},
   {0.0130f, 0.0110f, 0.0083f},
   {0.0490f, 0.0420f, 0.0370f},
    0.0490f, 0.4500f, 0.1700f,
    1.0000f, 0.1500f, 0.0150f},
  {"blue",
   {0.0079f, 0.0230f, 0.1000f},
   {0.0011f, 0.0015f, 0.0019f},
   {0.0250f, 0.0300f, 0.0430f},
   {0.0590f, 0.0740f, 0.0820f},
    1.0000f, 0.0940f, 0.1700f,
    0.1500f, 0.0430f, 0.0200f},
  {"blue matte",
   {0.0099f, 0.0360f, 0.1200f},
   {0.0032f, 0.0045f, 0.0059f},
   {0.1800f, 0.2300f, 0.2800f},
   {0.0400f, 0.0490f, 0.0510f},
    1.0000f, 0.0460f, 0.1700f,
    0.1600f, 0.0750f, 0.0340f}
};

}//namespace lux

