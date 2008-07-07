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

// torus.cpp*
#include "shape.h"
#include "mc.h"

namespace lux
{

// Torus Declarations
class Torus: public Shape {
public:
	// Sphere Public Methods
	Torus(const Transform &o2w, bool ro, float marad, float mirad,
	       float zmin, float zmax, float phiMax);
	BBox ObjectBound() const;
	bool Intersect(const Ray &ray, float *tHit,
	               DifferentialGeometry *dg) const;
	bool IntersectP(const Ray &ray) const;
	float Area() const;
	Point Sample(float u1, float u2, Normal *ns) const {

		float phi = u1 * phiMax;
		float theta;
		// select inner or outer half based on u2
		theta = thetaMin + u2 * (thetaMax - thetaMin);

		float cosphi = cosf(phi);
		float sinphi = sinf(phi);
		float sintheta = sinf(theta);

		Point p = Point((majorRadius+minorRadius*sintheta)*cosphi,
			(majorRadius+minorRadius*sintheta)*sinphi, minorRadius*cosf(theta));

		Point cp = Point(majorRadius*cosphi, majorRadius*sinphi, 0);

		*ns = Normalize(ObjectToWorld(Normal(p - cp)));
		if (reverseOrientation) *ns *= -1.f;
		return ObjectToWorld(p);
	}

	static Shape* CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);
private:
	bool FindIntersection(const Ray &ray, float *tHit, Point *pHit, float *phiHit, float *thetaHit) const;

	// Sphere Private Data
	float majorRadius, minorRadius;
	float phiMax;
	float thetaMin, thetaMax;
	// lowest and highest point on torus, used for bounding
	float zmin, zmax;
};

}//namespace lux
