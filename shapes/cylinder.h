/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

// cylinder.cpp*
#include "shape.h"
// Cylinder Declarations

namespace lux
{

class Cylinder: public Shape {
public:
	// Cylinder Public Methods
	Cylinder(const Transform &o2w, bool ro, const string &name, 
	         float rad, float zmin, float zmax, float phiMax);
	virtual ~Cylinder() { }
	virtual BBox ObjectBound() const;
	virtual bool Intersect(const Ray &ray, float *tHit,
	               DifferentialGeometry *dg) const;
	virtual bool IntersectP(const Ray &ray) const;
	virtual float Area() const;
	virtual Point Sample(float u1, float u2, float u3, 
			Normal *Ns) const {
		float z = luxrays::Lerp(u1, zmin, zmax);
		float t = u2 * phiMax;
		Point p = Point(radius * cosf(t), radius * sinf(t), z);
		*Ns = Normalize(ObjectToWorld * Normal(p.x, p.y, 0.));
		if (reverseOrientation)
			*Ns *= -1.f;
		return ObjectToWorld * p;
	}
	
	static Shape* CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);
protected:
	// Cylinder Private Data
	float radius;
	float zmin, zmax;
	float phiMax;
};

}//namespace lux
