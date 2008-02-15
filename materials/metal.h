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
#include "lux.h"
#include "spd.h"
#include "material.h"

namespace lux
{

// Metal Class Declarations

class Metal : public Material {
public:
  // Metal Public Methods
  Metal(boost::shared_ptr<Texture<SPD*> > n, boost::shared_ptr<Texture<SPD*> > k,
    boost::shared_ptr<Texture<float> > rough, boost::shared_ptr<Texture<float> > bump);

  BSDF *GetBSDF(const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading) const;

  static Material * CreateMaterial(const Transform &xform, const TextureParams &mp);

private:
  // Metal Private Data
  boost::shared_ptr<Texture<SPD*> > N, K;
  boost::shared_ptr<Texture<float> > roughness;
  boost::shared_ptr<Texture<float> > bumpMap;
};

struct MetalIOR {
  string name;
  float n[COLOR_SAMPLES];
  float k[COLOR_SAMPLES];
};

static MetalIOR metalIORs[] = {
  // default material is at index 0
  { "aluminium", {1.226973f, 0.930951f, 0.600745f}, {7.284448f, 6.535680f, 5.363405f} },
  { "amorphous carbon", {2.411985f, 2.342105f, 2.181310f}, {0.764496f, 0.830813f, 0.889424f} },
  { "silver", {0.127651f, 0.126269f, 0.150659f}, {3.815297f, 3.285348f, 2.449588f} },
  { "gold", {0.194434f, 0.474661f, 1.460432f}, {3.094132f, 2.558459f, 1.869885f} },
  { "cobalt", {1.892247f, 1.686180f, 1.380801f}, {4.034256f, 3.657249f, 3.204640f} },
  { "copper", {0.458325f, 0.919766f, 1.164602f}, {3.023786f, 2.623051f, 2.373409f} },
  { "chromium", {3.532071f, 3.011534f, 1.922369f}, {4.374618f, 4.417932f, 4.126147f} },
  { "lithium", {0.218518f, 0.215410f, 0.249187f}, {2.785056f, 2.451268f, 1.887220f} },
  { "mercury", {1.897530f, 1.536893f, 1.057312f}, {5.113554f, 4.619182f, 3.846061f} },
  { "nickel", {1.900880f, 1.764200f, 1.632063f}, {3.578712f, 3.213744f, 2.644016f} },
  { "potassium", {0.050897f, 0.048574f, 0.043116f}, {1.675808f, 1.422659f, 1.051493f} },
  { "platinum", {2.282318f, 2.106779f, 1.841160f}, {4.024749f, 3.677716f, 3.133220f} },
  { "iridium", {2.455205f, 2.196805f, 1.825194f}, {4.547873f, 4.261930f, 3.659119f} },
  { "silicon", {3.926954f, 4.120908f, 4.741204f}, {0.023591f, 0.046621f, 0.162779f} },
  { "amorphous silicon", {4.261942f, 4.383820f, 4.448197f}, {0.513950f, 0.816898f, 1.611943f} },
  { "sodium", {0.052690f, 0.058955f, 0.066723f}, {2.549482f, 2.233368f, 1.800470f} },
  { "rhodium", {2.098300f, 1.960583f, 1.726040f}, {5.444757f, 4.978817f, 4.439692f} },
  { "tungsten", {3.587545f, 3.476418f, 3.321998f}, {2.861609f, 2.740650f, 2.515569f} },
  { "vanadium", {3.603907f, 3.623065f, 3.076576f}, {2.946438f, 3.069547f, 3.378471f} } 
};

}//namespace lux
