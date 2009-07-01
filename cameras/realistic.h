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

// realistic.cpp*
#include "camera.h"
#include "dynload.h"

namespace lux
{

struct Lens {
    Lens(const bool ent, const float n, const float ap,
        boost::shared_ptr<Shape> shape) 
        : entering(ent), eta(n), aperture(ap), shape(shape) {}
    bool entering;
    float eta;
    float aperture;
    boost::shared_ptr<Shape> shape;
};

class RealisticCamera : public Camera {
public:
  RealisticCamera(const Transform &world2camStart, const Transform &world2camEnd, 
		  const float Screen[4],
		  float hither, float yon, float sopen,
		  float sclose, int sdist, float filmdistance, float aperture_diameter, string specfile,
		  float filmdiag, Film *film);
  virtual ~RealisticCamera(void);
  virtual float GenerateRay(const Sample &sample, Ray *) const;

  virtual RealisticCamera* Clone() const {
	  return new RealisticCamera(*this);
  }

  static Camera *CreateCamera(const Transform &world2cam, const Transform &world2camEnd, const ParamSet &params, Film *film);
  
private:
    float ParseLensData(const string& specfile);

    float filmDistance, filmDist2, filmDiag;
    float apertureDiameter, distToBack, backAperture;
 
    vector<boost::shared_ptr<Lens> > lenses;

    Transform RasterToFilm, RasterToCamera, FilmToCamera;
};

}//namespace lux

