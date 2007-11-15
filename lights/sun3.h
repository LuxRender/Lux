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

#ifndef SUN3_H
#define SUN3_H

// sun3.cpp*
#include "lux.h"
#include "light.h"
#include "shape.h"
#include "primitive.h"

class Sun3Light : public Light {
public:
  // Sun3Light Interface
  Sun3Light(const Transform &light2world, const Spectrum &power, int ns, Vector sundir, float turb);

  virtual Spectrum L(const Point &p, const Normal &n, const Vector &w) const;

  Spectrum Power(const Scene *) const;

  bool IsDeltaLight() const;

  float Pdf(const Point &, const Vector &) const;
  float Pdf(const Point &, const Normal &, const Vector &) const;
  Spectrum Sample_L(const Point &P, Vector *w, VisibilityTester *visibility) const;

  virtual Spectrum Sample_L(const Point &P, const Normal &N, float u1, float u2, Vector *wo, float *pdf, VisibilityTester *visibility) const;
  virtual Spectrum Sample_L(const Point &P, float u1, float u2, Vector *wo, float *pdf, VisibilityTester *visibility) const;
  Spectrum Sample_L(const Scene *scene, float u1, float u2, float u3, float u4, Ray *ray, float *pdf) const;

  static Sun3Light *CreateLight(const Transform &light2world, const ParamSet &paramSet);

protected:
  // Sun3Light Protected Data

  Spectrum computeAttenuatedSunlight(float theta, float turbidity);

  Spectrum Lemit;
  ShapePtr shape;
  float area;
  Vector sunDir;
  float turbidity;
};

#endif
