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

// hyperboloid.cpp*
#include "shape.h"

namespace lux
{

// Hyperboloid Declarations
class Hyperboloid: public Shape {
public:
	// Hyperboloid Public Methods
	Hyperboloid(const Transform &o2w, bool ro,
	            const Point &point1, const Point &point2,
	            float tm);
	virtual ~Hyperboloid() { }
	virtual BBox ObjectBound() const;
	virtual bool Intersect(const TsPack *tspack, const Ray &ray, float *tHit,
	               DifferentialGeometry *dg) const;
	virtual bool IntersectP(const TsPack *tspack, const Ray &ray) const;
	virtual float Area() const;
	
	static Shape* CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);
protected:
	// Hyperboloid Data
	Point p1, p2;
	float zmin, zmax;
	float phiMax;
	float rmax;
	float a, c;
};

}//namespace lux
