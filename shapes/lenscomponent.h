/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
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

#include "shape.h"

namespace lux
{

class LensComponent : public Shape {
public:
    LensComponent(const Transform &o2w, bool ro, float rad,
        float zmin, float zmax, float phiMax, float aperture);
    virtual ~LensComponent() { }
    // LensComponent public data
    virtual BBox ObjectBound() const;
    virtual bool Intersect(const Ray &ray, float *tHit,
        DifferentialGeometry *dg, bool null_shp_isect = false) const;
    virtual bool IntersectP(const Ray &ray, bool null_shp_isect = false) const;
    virtual float Area() const;
    virtual Point Sample(float u1, float u2, float u3, Normal *n) const;

	static Shape* CreateShape(const Transform &o2w,
                   bool reverseOrientation,
                   const ParamSet &params);

private:
    // LensComponent Private Data
    float apRadius;
    float radius;
    float phiMax;
    float zmin, zmax;
    float thetaMin, thetaMax;
};

}//namespace lux

