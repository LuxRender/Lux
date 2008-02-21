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

// realistic.cpp*
#include "camera.h"
#include "film.h"
#include "paramset.h"

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
  RealisticCamera(const Transform &world2cam, const float Screen[4],
		  float hither, float yon, float sopen,
		  float sclose, float filmdistance, float aperture_diameter, string specfile,
		  float filmdiag, Film *film);
  ~RealisticCamera(void);
  float GenerateRay(const Sample &sample, Ray *) const;

  static Camera *CreateCamera(const ParamSet &params, const Transform &world2cam, Film *film);
  
private:
    float ParseLensData(const string& specfile);

    float filmDistance, filmDist2, filmDiag;
    float apertureDiameter, distToBack, backAperture;
 
    vector<boost::shared_ptr<Lens> > lenses;

    Transform RasterToFilm, RasterToCamera, FilmToCamera;
};

}//namespace lux