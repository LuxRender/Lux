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
#include "mc.h"
#include "quadrilateral.h"

namespace lux
{

// Quad Declarations
class Quad : public Shape {
public:
	// Quad Public Methods
	Quad(const Transform &o2w, bool ro, int nq, int nv, 
		const int *vi, const Point *P);
	virtual ~Quad();
	virtual BBox ObjectBound() const;
	virtual BBox WorldBound() const;
	virtual bool Intersect(const Ray &ray, float *tHit,
	               DifferentialGeometry *dg) const;
	virtual bool IntersectP(const Ray &ray) const;
	virtual float Area() const;
	virtual Point Sample(float u1, float u2, float u3, Normal *Ns) const {
		return quad->Sample(u1, u2, u3, Ns);
	}
	
	static Shape* CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);
private:
	// Quad Private Data	
	QuadMesh *mesh;
	Quadrilateral *quad;
};

}//namespace lux
